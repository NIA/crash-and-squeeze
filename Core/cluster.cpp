#include "Core/cluster.h"
#include <cstring>
#include <cstdio>

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;
    using Math::equal;
    using Math::less_or_equal;
    using Math::greater_or_equal;
    using Math::sign;
    using Math::cube_root;
    using Math::Vector;
    using Math::Matrix;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
    using Math::TriVector;
    using Math::TriMatrix;
    using Math::NineMatrix;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

    
    namespace Core
    {
        // a constant, determining how fast points are pulled to
        // their position, i.e. how rigid the body is:
        // 0 means no constraint at all, 1 means that current position = goal position
        const Real Cluster::DEFAULT_GOAL_SPEED_CONSTANT = 0.8;
        
        // a constant, determining how rigid body is:
        // if it equals `b`, then optimal deformation for goal positions
        // is calculated as (1 - b)*A + b*R, where R is optimal rotation
        // and A is optimal linear transformation.
        // Thus 1 matches only rotated and shifted shape (rigid),
        // 0 allows any linear transformation
        const Real Cluster::DEFAULT_LINEAR_ELASTICITY_CONSTANT = 0.1;
        
        // plasticity parameter: a threshold of strain, after
        // which deformation becomes non-reversible
        const Real Cluster::DEFAULT_YIELD_CONSTANT = 0.1;

        // plasticity paramter: a coefficient determining how fast
        // plasticity_state will be changed on large deformation
        const Real Cluster::DEFAULT_CREEP_CONSTANT = 60;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
        // another plasticity paramter: a coefficient determining how fast
        // quadratic part of plasticity_state will be changed on large deformation
        const Real Cluster::DEFAULT_QX_CREEP_CONSTANT = 10;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED

        // plasticity paramter: a threshold of maximum allowed strain
        const Real Cluster::DEFAULT_MAX_DEFORMATION_CONSTANT = 1.5;
        
        void PhysicalVertexMappingInfo::setup_initial_values(const Vector & center_of_mass)
        {
            initial_offset_pos = vertex->get_pos() - center_of_mass;
            equilibrium_offset_pos = initial_offset_pos;
            equilibrium_pos = vertex->get_pos();
        }

        void GraphicalVertexMappingInfo::setup_initial_values(const Vector & center_of_mass)
        {
            initial_offset_state = *vertex;
            for(int i = 0; i < initial_offset_state.get_points_num(); ++i)
            {
                Vector offset = vertex->get_point(i) - center_of_mass;
                initial_offset_state.set_point(i, offset);
            }
            deformed_offset_state = initial_offset_state;
            previous_state = *vertex;
        }

        // -- Cluster methods --

        Cluster::Cluster()
            : physical_vertex_infos(INITIAL_ALLOCATED_VERTICES_NUM),
              initial_characteristics_computed(true),

              total_mass(0),
              valid(false),

              goal_speed_constant(DEFAULT_GOAL_SPEED_CONSTANT),
              linear_elasticity_constant(DEFAULT_LINEAR_ELASTICITY_CONSTANT),
              yield_constant(DEFAULT_YIELD_CONSTANT),
              creep_constant(DEFAULT_CREEP_CONSTANT),
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
              qx_creep_constant(DEFAULT_QX_CREEP_CONSTANT),
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
              max_deformation_constant(DEFAULT_MAX_DEFORMATION_CONSTANT),

              center_of_mass(Vector::ZERO),
              rotation(Matrix::IDENTITY),
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
              total_deformation(Matrix::IDENTITY, Matrix::ZERO, Matrix::ZERO),
#else
              total_deformation(Matrix::IDENTITY),
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
              plasticity_state(Matrix::IDENTITY, Matrix::ZERO, Matrix::ZERO),
#else
              plasticity_state(Matrix::IDENTITY),
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
              plasticity_state_inv_trans(Matrix::IDENTITY),
              plastic_deformation_measure(0)
        {
        }

        void Cluster::add_physical_vertex(PhysicalVertex &vertex)
        {
            // update mass
            total_mass += vertex.get_mass();

            // add new vertex
            PhysicalVertexMappingInfo & info = physical_vertex_infos.create_item();
            info.vertex = &vertex;
            
            // set addition index
            info.addition_index = vertex.get_next_addition_index();

            // invalidate initial characteristics
            initial_characteristics_computed = false;
        }
        
        void Cluster::add_graphical_vertex(GraphicalVertex &vertex)
        {
            // add new vertex
            GraphicalVertexMappingInfo & mapping_info = graphical_vertex_infos.create_item();
            mapping_info.vertex = &vertex;
            mapping_info.setup_initial_values(center_of_mass);
        }

        void Cluster::compute_initial_characteristics()
        {
            physical_vertex_infos.freeze();

            update_center_of_mass();
            initial_center_of_mass = center_of_mass;

            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                physical_vertex_infos[i].setup_initial_values(center_of_mass);
            }

            for(int i = 0; i < get_graphical_vertices_num(); ++i)
            {
                graphical_vertex_infos[i].setup_initial_values(center_of_mass);
            }

            initial_characteristics_computed = true;

            compute_symmetric_term();
        }

        void Cluster::match_shape(Real dt)
        {
            if(0 == get_physical_vertices_num())
                return;

            update_center_of_mass();
            compute_transformations();
            update_equilibrium_positions(false);
            apply_goal_positions(dt);
            update_plasticity_state(dt);
            update_graphical_transformations();
        }            

        void Cluster::update_center_of_mass()
        {
            center_of_mass = Vector::ZERO;
            if( 0 != total_mass )
            {
                for(int i = 0; i < get_physical_vertices_num(); ++i)
                {
                    PhysicalVertex &v = get_physical_vertex(i);
                    center_of_mass += v.get_mass()*v.get_pos()/total_mass;
                }
            }
        }

        void Cluster::update_equilibrium_positions(bool plasticity_state_changed)
        {
            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                Vector & equil_pos           = physical_vertex_infos[i].equilibrium_pos;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                TriVector & equil_offset_pos = physical_vertex_infos[i].equilibrium_offset_pos;
                if(plasticity_state_changed)
                {
                    equil_offset_pos.set_vector(plasticity_state * physical_vertex_infos[i].initial_offset_pos);
                }
                Vector new_equil_pos = center_of_mass + rotation * equil_offset_pos.to_vector();
#else
                Vector & equil_offset_pos    = physical_vertex_infos[i].equilibrium_offset_pos;
                if(plasticity_state_changed)
                {
                    equil_offset_pos = plasticity_state * physical_vertex_infos[i].initial_offset_pos;
                }
                Vector new_equil_pos = center_of_mass + rotation * equil_offset_pos;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

                get_physical_vertex(i).change_equilibrium_pos(new_equil_pos - equil_pos);
                equil_pos = new_equil_pos;
            }
        }

        void Cluster::update_graphical_transformations()
        {
#if CAS_GRAPHICAL_TRANSFORM_TOTAL
    #if CAS_QUADRATIC_EXTENSIONS_ENABLED
            graphical_pos_transform = total_deformation;
        #if CAS_QUADRATIC_PLASTICITY_ENABLED
            // TODO: implement proper superposition of tri-matrices graphical_pos_transform and plasticity_state (if it is possible...)
            graphical_pos_transform.as_matrix() = graphical_pos_transform.to_matrix()*plasticity_state.to_matrix();
        #else
            graphical_pos_transform.as_matrix() = graphical_pos_transform.to_matrix()*plasticity_state;
        #endif // CAS_QUADRATIC_PLASTICITY_ENABLED
            // TODO: use quadratic transformation for graphical vertices normals as well
            graphical_nrm_transform = graphical_pos_transform.to_matrix().inverted().transposed();
    #else
            graphical_pos_transform = total_deformation*plasticity_state;
            graphical_nrm_transform = graphical_pos_transform.inverted().transposed();
    #endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
#else
    #if CAS_QUADRATIC_EXTENSIONS_ENABLED
        #if CAS_QUADRATIC_PLASTICITY_ENABLED
            // TODO: implement proper superposition of tri-matrices graphical_pos_transform and plasticity_state (if it is possible...)
            graphical_pos_transform = TriMatrix(rotation*plasticity_state.to_matrix(), Matrix::ZERO, Matrix::ZERO);
        #else
            graphical_pos_transform = TriMatrix(rotation*plasticity_state, Matrix::ZERO, Matrix::ZERO);
        #endif // CAS_QUADRATIC_PLASTICITY_ENABLED
            // TODO: use quadratic transformation for graphical vertices normals as well
            graphical_nrm_transform = rotation*plasticity_state_inv_trans;
    #else
            graphical_pos_transform = rotation*plasticity_state;
            graphical_nrm_transform = rotation*plasticity_state_inv_trans;
    #endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
#endif // CAS_GRAPHICAL_TRANSFORM_TOTAL
        }

        void Cluster::compute_asymmetric_term()
        {
            asymmetric_term.set_all(0);
            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_physical_vertex(i);

#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                asymmetric_term += TriMatrix( v.get_mass()*(v.get_pos() - center_of_mass), get_equilibrium_offset_pos(i) );
#else
                asymmetric_term += Matrix( v.get_mass()*(v.get_pos() - center_of_mass), get_equilibrium_offset_pos(i) );
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            }
        }

        void Cluster::compute_symmetric_term()
        {
            symmetric_term.set_all(0);
            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_physical_vertex(i);
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                const TriVector & equilibrium_pos = get_equilibrium_offset_pos(i);
                symmetric_term += NineMatrix( equilibrium_pos*v.get_mass(), equilibrium_pos );
#else
                const Vector & equilibrium_pos = get_equilibrium_offset_pos(i);
                symmetric_term += Matrix( v.get_mass()*equilibrium_pos, equilibrium_pos );
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            }
            // Try to invert matrix, if not possible => mark cluster as not valid
            valid = symmetric_term.invert_sym();

            if (!valid)
                Logger::warning("in Cluster::compute_symmetric_term: symmetric term is not invertible, cluster marked as invalid", __FILE__, __LINE__);
            // TODO: try to fix invalid clusters (e.g., remove them and add vertices to the nearest other cluster)
        }

        void Cluster::compute_transformations()
        {
            compute_optimal_transformation();

#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            optimal_transformation.to_matrix().do_polar_decomposition(rotation, scale);
            // (1-b)*[A Q M] + b*[R 0 0]
            total_deformation = optimal_transformation;
            total_deformation *= (1 - linear_elasticity_constant);
            total_deformation.as_matrix() += linear_elasticity_constant*rotation;
#else
            optimal_transformation.do_polar_decomposition(rotation, scale);
            // (1-b)*A + b*R
            total_deformation = linear_elasticity_constant*rotation + (1 - linear_elasticity_constant)*optimal_transformation;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

        }

        void Cluster::compute_optimal_transformation()
        {
            // check that symmetric_term has been precomputed...
            if( ! check_initial_characteristics() )
                return;
            // ...and that cluster is valid...
            if ( ! valid )
            {
                // if invalid, the best we can assume is no deformation - identity matrix
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                optimal_transformation = TriMatrix(Matrix::IDENTITY, Matrix::ZERO, Matrix::ZERO);
#else
                optimal_transformation = Matrix::IDENTITY;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
                return;
            }
            // ...and compute a brand new assymetric_term
            compute_asymmetric_term();

            // A = Apq * Aqq
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            symmetric_term.left_mult_by(asymmetric_term, optimal_transformation);
#else
            optimal_transformation = asymmetric_term*symmetric_term;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

            // -- enforce volume conservation --

#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            Real det = optimal_transformation.to_matrix().determinant();
#else
            Real det = optimal_transformation.determinant();
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            if( ! equal(0, det) )
            {
                if( det < 0 )
                {
                    Logger::warning("in Cluster::compute_optimal_transformation: optimal_transformation.determinant() is less than 0, inverted state detected!", __FILE__, __LINE__);
                }
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                optimal_transformation.as_matrix() /= cube_root(det);
#else
                optimal_transformation /= cube_root(det);
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            }
            else
            {
                Logger::warning("in Cluster::compute_optimal_transformation: optimal_transformation is singular, so volume-preserving constraint cannot be enforced", __FILE__, __LINE__);
                // but now, while polar decomposition is only for invertible matrix - it's very, very bad...
            }
        }

        void Cluster::apply_goal_positions(Real dt)
        {
            // -- find and apply velocity_addition --

            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                PhysicalVertex &vertex = get_physical_vertex(i);

                Vector goal_position = total_deformation*get_equilibrium_offset_pos(i) + center_of_mass;

                Vector velocity_addition = goal_speed_constant*(goal_position - vertex.get_pos())/dt;

                vertex.add_to_average_velocity_addition(velocity_addition, get_addition_index(i));
            }
        }

        Real Cluster::get_relative_plastic_deformation() const
        {
            return plastic_deformation_measure/max_deformation_constant;
        }
        
        void Cluster::update_plasticity_state(Real dt)
        {
            if(less_or_equal(max_deformation_constant, 0))
                return;

            Matrix deformation = scale - Matrix::IDENTITY;
            Real deformation_measure = deformation.norm();

            if(deformation_measure > yield_constant)
            {
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
                // Plasticity state [Pa Pq Pm] is changed like this:
                // Pa = (E + dt*c*(S-E))*Pa (just like linear case)
                // Pq = Pq + dt*c*Q, Pm = Pm + dt*c*M
                TriMatrix new_plasticity_state = optimal_transformation;
                new_plasticity_state *= dt*qx_creep_constant;
                new_plasticity_state += plasticity_state;
                new_plasticity_state.as_matrix() = (Matrix::IDENTITY + dt*creep_constant*deformation)*plasticity_state.to_matrix();
                Real det = new_plasticity_state.to_matrix().determinant();
#else
                Matrix new_plasticity_state = (Matrix::IDENTITY + dt*creep_constant*deformation)*plasticity_state;
                Real det = new_plasticity_state.determinant();
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED

                if( ! equal(0, det) )
                {
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
                    // enforce volume conservation
                    new_plasticity_state.as_matrix() /= cube_root(det);

                    Real new_plastic_deform_measure = (new_plasticity_state.to_matrix() - Matrix::IDENTITY).norm();
#else
                    // enforce volume conservation
                    new_plasticity_state /= cube_root(det);

                    Real new_plastic_deform_measure = (new_plasticity_state - Matrix::IDENTITY).norm();
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED

                    // TODO: is condition of (new_plastic_deform_measure > plastic_deformation_measure) useful?
                    if( new_plastic_deform_measure < max_deformation_constant &&
                        new_plastic_deform_measure > plastic_deformation_measure )
                    {
                        plasticity_state = new_plasticity_state;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
                        plasticity_state_inv_trans = plasticity_state.to_matrix().inverted().transposed();
#else
                        plasticity_state_inv_trans = plasticity_state.inverted().transposed();
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
                        plastic_deformation_measure = new_plastic_deform_measure;
                        update_equilibrium_positions(true);
                        compute_symmetric_term();
                    }
                }
            }
        }

        bool Cluster::check_initial_characteristics() const
        {
            if( ! initial_characteristics_computed )
            {
                Logger::error("missed precomputing: Cluster::compute_initial_characteristics must be called after last vertex is added", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        void Cluster::set_simulation_params(const SimulationParams & params)
        {
            goal_speed_constant        = params.goal_speed_fraction;
            linear_elasticity_constant = params.linear_elasticity_fraction;

            yield_constant           = params.yield_threshold;
            creep_constant           = params.creep_speed;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            qx_creep_constant        = params.quadratic_creep_speed;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            max_deformation_constant = params.max_deformation;
        }

        void Cluster::get_simulation_params(SimulationParams /*out*/ & params) const
        {
            params.goal_speed_fraction        = goal_speed_constant;
            params.linear_elasticity_fraction = linear_elasticity_constant;
            

            params.yield_threshold       = yield_constant;
            params.creep_speed           = creep_constant;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            params.quadratic_creep_speed = qx_creep_constant;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            params.max_deformation       = max_deformation_constant;
        }

        const PhysicalVertex & Cluster::get_physical_vertex(int index) const
        {
            return *physical_vertex_infos[index].vertex;
        }

        PhysicalVertex & Cluster::get_physical_vertex(int index)
        {
            return *physical_vertex_infos[index].vertex;
        }

        // returns equilibrium position of vertex
        // (measured off the center of mass of the cluster)
        // taking into account plasticity_state
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
        const TriVector & Cluster::get_equilibrium_offset_pos(int index) const
#else
        const Vector & Cluster::get_equilibrium_offset_pos(int index) const
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
        {
            check_initial_characteristics();
            return physical_vertex_infos[index].equilibrium_offset_pos;
        }

        const int Cluster::get_addition_index(int index) const
        {
            return physical_vertex_infos[index].addition_index;
        }

        void Cluster::log_properties(int id)
        {
            static char buffer[1024];
            sprintf_s(buffer, "cluster #%2d, physical vertices: %4d, graphical vertices: %4d",
                              id, get_physical_vertices_num(), get_graphical_vertices_num());
            Logger::log(buffer);
        }

        Cluster::~Cluster()
        {
        }
    }
}

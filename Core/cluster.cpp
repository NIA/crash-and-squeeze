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
    
    namespace Core
    {
        // a constant, determining how fast points are pulled to
        // their position, i.e. how rigid the body is:
        // 0 means no constraint at all, 1 means absolutely rigid
        const Real Cluster::DEFAULT_GOAL_SPEED_CONSTANT = 1;
        
        // a constant, determining how rigid body is:
        // if it equals `b`, then optimal deformation for goal positions
        // is calculated as (1 - b)*A + b*R, where R is optimal rotation
        // and A is optimal linear transformation.
        // Thus 1 matches only rotated and shifted shape,
        // 0 allows any linear transformation
        const Real Cluster::DEFAULT_LINEAR_ELASTICITY_CONSTANT = 0.7;
        
        // plasticity parameter: a threshold of strain, after
        // which deformation becomes non-reversible
        const Real Cluster::DEFAULT_YIELD_CONSTANT = 0.1;

        // plasticity paramter: a coefficient determining how fast
        // plasticity_state will be changed on large deformation
        const Real Cluster::DEFAULT_CREEP_CONSTANT = 60;

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
              max_deformation_constant(DEFAULT_MAX_DEFORMATION_CONSTANT),

              center_of_mass(Vector::ZERO),
              rotation(Matrix::IDENTITY),
              total_deformation(Matrix::IDENTITY),
              plasticity_state(Matrix::IDENTITY),
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
                Vector & equil_offset_pos = physical_vertex_infos[i].equilibrium_offset_pos;
                Vector & equil_pos        = physical_vertex_infos[i].equilibrium_pos;
                if(plasticity_state_changed)
                {
                    equil_offset_pos = plasticity_state * physical_vertex_infos[i].initial_offset_pos;
                }

                Vector new_equil_pos = center_of_mass + rotation * equil_offset_pos;

                get_physical_vertex(i).change_equilibrium_pos(new_equil_pos - equil_pos);
                equil_pos = new_equil_pos;
            }
        }

        void Cluster::update_graphical_transformations()
        {
#if CAS_GRAPHICAL_TRANSFORM_TOTAL
            graphical_pos_transform = total_deformation;
            graphical_nrm_transform = total_deformation.inverted().transposed();
#else
            graphical_pos_transform = rotation*plasticity_state;
            graphical_nrm_transform = rotation*plasticity_state_inv_trans;
#endif // CAS_GRAPHICAL_TRANSFORM_TOTAL
        }

        void Cluster::compute_asymmetric_term()
        {
            asymmetric_term = Matrix::ZERO;
            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_physical_vertex(i);

                asymmetric_term += v.get_mass()*Matrix( v.get_pos() - center_of_mass, get_equilibrium_offset_pos(i) );
            }
        }

        void Cluster::compute_symmetric_term()
        {
            symmetric_term = Matrix::ZERO;
            for(int i = 0; i < get_physical_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_physical_vertex(i);
                Vector equilibrium_pos = get_equilibrium_offset_pos(i);


                symmetric_term += v.get_mass()*Matrix( equilibrium_pos, equilibrium_pos );
            }
            if( symmetric_term.is_invertible() )
            {
                valid = true;
                symmetric_term = symmetric_term.inverted();
            }
            else
            {
                valid = false;
                symmetric_term = Matrix::ZERO;
            }
        }

        void Cluster::compute_transformations()
        {
            compute_optimal_transformation();

            optimal_transformation.do_polar_decomposition(rotation, scale);

            total_deformation = linear_elasticity_constant*rotation + (1 - linear_elasticity_constant)*optimal_transformation;
        }

        void Cluster::compute_optimal_transformation()
        {
            // check that symmetric_term has been precomputed...
            if( ! check_initial_characteristics() )
                return;
            // ...and compute a brand new assymetric_term
            compute_asymmetric_term();

            optimal_transformation = asymmetric_term*symmetric_term;

            // -- enforce volume conservation --

            Real det = optimal_transformation.determinant();
            if( ! equal(0, det) )
            {
                if( det < 0 )
                {
                    Logger::warning("in Cluster::compute_optimal_transformation: optimal_transformation.determinant() is less than 0, inverted state detected!", __FILE__, __LINE__);
                }
                optimal_transformation /= cube_root(det);
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
                Vector equilibrium_pos = get_equilibrium_offset_pos(i);

                Vector goal_position = total_deformation*equilibrium_pos + center_of_mass;

                Vector velocity_addition = goal_speed_constant*(goal_position - vertex.get_pos())/dt;

                vertex.add_to_average_velocity_addition(velocity_addition, get_addition_index(i));
            }
        }

        Math::Real Cluster::get_relative_plastic_deformation() const
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
                Matrix new_plasticity_state = (Matrix::IDENTITY + dt*creep_constant*deformation)*plasticity_state;
                Real det = new_plasticity_state.determinant();
                if( ! equal(0, det) )
                {
                    // enforce volume conservation
                    new_plasticity_state /= cube_root(det);

                    Real new_plastic_deform_measure = (new_plasticity_state - Matrix::IDENTITY).norm();

                    // TODO: is condition of (new_plastic_deform_measure > plastic_deformation_measure) useful?
                    if( new_plastic_deform_measure < max_deformation_constant &&
                        new_plastic_deform_measure > plastic_deformation_measure )
                    {
                        plasticity_state = new_plasticity_state;
                        plasticity_state_inv_trans = plasticity_state.inverted().transposed();
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
        const Vector & Cluster::get_equilibrium_offset_pos(int index) const
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

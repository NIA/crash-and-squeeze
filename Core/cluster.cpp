#include "Core/cluster.h"
#include <cstring>

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
        // Thus 0 means freely (but only linearly) deformable body,
        // 1 means absolutely rigid
        const Real Cluster::DEFAULT_LINEAR_ELASTICITY_CONSTANT = 0.7;
        
        // plasticity parameter: a threshold of strain, after
        // which deformation becomes non-reversible
        const Real Cluster::DEFAULT_YIELD_CONSTANT = 0.4;

        // plasticity paramter: a coefficient determining how fast
        // plasticity_state will be changed on large deformation
        const Real Cluster::DEFAULT_CREEP_CONSTANT = 60;

        // plasticity paramter: a threshold of maximum allowed strain
        const Real Cluster::DEFAULT_MAX_DEFORMATION_CONSTANT = 1.5;
        
        // -- Cluster methods --

        Cluster::Cluster()
            : vertex_infos(INITIAL_ALLOCATED_VERTICES_NUM),
              initial_characteristics_computed(true),

              total_mass(0),
              valid(false),

              pos(Vector::ZERO),
              size(Vector::ZERO),

              goal_speed_constant(DEFAULT_GOAL_SPEED_CONSTANT),
              linear_elasticity_constant(DEFAULT_LINEAR_ELASTICITY_CONSTANT),
              yield_constant(DEFAULT_YIELD_CONSTANT),
              creep_constant(DEFAULT_CREEP_CONSTANT),
              max_deformation_constant(DEFAULT_MAX_DEFORMATION_CONSTANT),

              initial_center_of_mass(Vector::ZERO),
              center_of_mass(Vector::ZERO),
              rotation(Matrix::IDENTITY),
              total_deformation(Matrix::IDENTITY),
              plasticity_state(Matrix::IDENTITY)
        {
        }

        void Cluster::add_vertex(PhysicalVertex &vertex)
        {
            // update mass
            total_mass += vertex.get_mass();

            // add new vertex
            vertex_infos.create_item().vertex = &vertex;

            // increment vertex's cluster counter
            vertex.include_to_one_more_cluster();

            // invalidate initial characteristics
            initial_characteristics_computed = false;
        }

        void Cluster::compute_initial_characteristics()
        {
            vertex_infos.freeze();

            update_center_of_mass();
            initial_center_of_mass = center_of_mass;

            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                
                vertex_infos[i].initial_offset_position = v.get_pos() - initial_center_of_mass;
                vertex_infos[i].equilibrium_offset_position = vertex_infos[i].initial_offset_position;
            }
            
            initial_characteristics_computed = true;

            compute_symmetric_term();
        }

        void Cluster::match_shape(Real dt)
        {
            if(0 == get_vertices_num())
                return;

            update_center_of_mass();
            compute_transformations();
            apply_goal_positions(dt);
            update_plasticity_state(dt);
        }
            

        void Cluster::update_center_of_mass()
        {
            center_of_mass = Vector::ZERO;
            if( 0 != total_mass )
            {
                for(int i = 0; i < get_vertices_num(); ++i)
                {
                    PhysicalVertex &v = get_vertex(i);
                    center_of_mass += v.get_mass()*v.get_pos()/total_mass;
                }
            }
        }
        void Cluster::update_equilibrium_positions()
        {
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                vertex_infos[i].equilibrium_offset_position = plasticity_state * vertex_infos[i].initial_offset_position;
            }
        }

        void Cluster::compute_asymmetric_term()
        {
            asymmetric_term = Matrix::ZERO;
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_vertex(i);

                asymmetric_term += v.get_mass()*Matrix( v.get_pos() - center_of_mass, get_equilibrium_position(i) );
            }
        }

        void Cluster::compute_symmetric_term()
        {
            symmetric_term = Matrix::ZERO;
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_position(i);

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
            compute_linear_transformation();

            linear_transformation.do_polar_decomposition(rotation, scale);
            
            total_deformation = linear_elasticity_constant*rotation + (1 - linear_elasticity_constant)*linear_transformation;
        }

        void Cluster::compute_linear_transformation()
        {
            // check that symmetric_term has been precomputed...
            if( ! check_initial_characteristics() )
                return;
            // ...and compute a brand new assymetric_term
            compute_asymmetric_term();

            linear_transformation = asymmetric_term*symmetric_term;

            // -- enforce volume conservation --

            Real det = linear_transformation.determinant();
            if( ! equal(0, det) )
            {
                if( det < 0 )
                {
                    Logger::warning("in Cluster::compute_linear_transformation: linear_transformation.determinant() is less than 0, inverted state detected!", __FILE__, __LINE__);
                }
                linear_transformation /= cube_root(det);
            }
            else
            {
                Logger::warning("in Cluster::compute_linear_transformation: linear_transformation is singular, so volume-preserving constraint cannot be enforced", __FILE__, __LINE__);
                // but now, while polar decomposition is only for invertible matrix - it's very, very bad...
            }
        }

        void Cluster::apply_goal_positions(Real dt)
        {
            // -- find and apply velocity_addition --

            Vector linear_momentum_addition = Vector::ZERO;
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &vertex = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_position(i);
                
                Vector goal_position = total_deformation*equilibrium_pos + center_of_mass;
                
                Vector velocity_addition = goal_speed_constant*(goal_position - vertex.get_pos())/dt;

                // velocity_addition is corrected inside this function...
                vertex.add_to_velocity_addition(velocity_addition);
                // ...and after that momentum delta is added to linear_momentum_addition
                linear_momentum_addition += vertex.get_mass()*velocity_addition;
            }

            // -- enforce total momentum conservation --

            if( 0 != total_mass )
            {
                // specific (per a unity of mass) velocity correction
                Vector specific_velocity_correction = - linear_momentum_addition / total_mass;
                
                for(int i = 0; i < get_vertices_num(); ++i)
                    get_vertex(i).correct_velocity_addition(specific_velocity_correction);
            }
        }

        Math::Real Cluster::get_relative_plastic_deformation() const
        {
            // TODO: store it
            return (plasticity_state - Matrix::IDENTITY).norm()/max_deformation_constant;
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
                    
                    Matrix plastic_deformation = new_plasticity_state - Matrix::IDENTITY;
                    Real plastic_deform_measure = plastic_deformation.norm();
                    
                    if( plastic_deform_measure < max_deformation_constant )
                    {
                        plasticity_state = new_plasticity_state;
                        update_equilibrium_positions();
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

        const PhysicalVertex & Cluster::get_vertex(int index) const
        {
            return *vertex_infos[index].vertex;
        }

        PhysicalVertex & Cluster::get_vertex(int index)
        {
            return *vertex_infos[index].vertex;
        }

        const Math::Vector & Cluster::get_initial_center_of_mass() const
        {
            check_initial_characteristics();
            return initial_center_of_mass;
        }
        
        // returns equilibrium position of vertex
        // (measured off the center of mass of the cluster)
        // taking into account plasticity_state
        const Vector & Cluster::get_equilibrium_position(int index) const
        {
            check_initial_characteristics();
            return vertex_infos[index].equilibrium_offset_position;
        }

        Cluster::~Cluster()
        {
        }
    }
}

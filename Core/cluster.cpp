#include "Core/cluster.h"

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
        
        // -- Cluster methods --

        Cluster::Cluster()
            : goal_speed_constant(DEFAULT_GOAL_SPEED_CONSTANT),
              linear_elasticity_constant(DEFAULT_LINEAR_ELASTICITY_CONSTANT),
              yield_constant(DEFAULT_YIELD_CONSTANT),
              creep_constant(DEFAULT_CREEP_CONSTANT),
              max_deformation_constant(DEFAULT_MAX_DEFORMATION_CONSTANT),

              total_deformation(Matrix::IDENTITY),
              plasticity_state(Matrix::IDENTITY),
              plastic_deformation_measure(0)
        {
        }

        void Cluster::compute_correction(Real dt)
        {
            if(0 == get_vertices_num())
                return;

            shape_matcher.match_shape();
            shape_matcher.update_vertices_equilibrium_positions();
            
            total_deformation = linear_elasticity_constant*get_rotation()
                              + (1 - linear_elasticity_constant)*get_linear_transformation();

            apply_goal_positions(dt);
            update_plasticity_state(dt);
        }

        void Cluster::apply_goal_positions(Real dt)
        {
            // -- find and apply velocity_addition --

            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &vertex = get_vertex(i);

                Vector goal_position = shape_matcher.get_center_of_mass()
                                     + total_deformation*shape_matcher.get_equilibrium_offset_pos(i);

                Vector velocity_addition = goal_speed_constant*(goal_position - vertex.get_pos())/dt;

                vertex.add_to_average_velocity_addition(velocity_addition);
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

            Matrix deformation = shape_matcher.get_scale() - Matrix::IDENTITY;
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
                        plastic_deformation_measure = new_plastic_deform_measure;
                        shape_matcher.update_equilibrium_offset_positions(plasticity_state);
                        shape_matcher.update_vertices_equilibrium_positions();
                    }
                }
            }
        }
    }
}

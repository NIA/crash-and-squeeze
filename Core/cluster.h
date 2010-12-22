#pragma once
#include "Core/core.h"
#include "Core/physical_vertex.h"
#include "Core/shape_matcher.h"
#include "Math/vector.h"
#include "Math/matrix.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class Cluster
        {
        private:
            ShapeMatcher shape_matcher;
            
            // -- parameters --

            // a constant, determining how fast points are pulled to
            // their position, i.e. how rigid the body is:
            // 0 means no constraint at all, 1 means absolutely rigid
            Math::Real goal_speed_constant;

            // a constant, determining how rigid body is:
            // if it equals `b`, then optimal deformation for goal positions
            // is calculated as (1 - b)*A + b*R, where R is optimal rotation
            // and A is optimal linear transformation.
            // Thus 0 means freely (but only linearly) deformable body,
            // 1 means absolutely rigid
            Math::Real linear_elasticity_constant;

            // plasticity parameter: a threshold of strain, after
            // which deformation becomes non-reversible
            Math::Real yield_constant;
            
            // plasticity paramter: a coefficient determining how fast
            // plasticity_state will be changed on large deformation
            Math::Real creep_constant;

            // plasticity paramter: a threshold of maximum allowed strain
            Math::Real max_deformation_constant;

            // -- variable (at run-time) fields --

            // plastic deformation applied to initial shape
            Math::Matrix plasticity_state;
            // measure of plasticity_state
            Math::Real plastic_deformation_measure;
            // total deformation used in goal positions calculation (interpolated from optimal rotation and optimal linear transformation)
            Math::Matrix total_deformation;
            
            // computes goal positions and applies corrections to velocities of vertices
            void apply_goal_positions(Math::Real dt);
            
            // updates plasticity_state due to deformation applied
            void update_plasticity_state(Math::Real dt);
            
            // updates equilibrium position offsets (if plasticity_state changed) and equilibrium positions
            void update_equilibrium_positions(bool plasticity_state_changed);
        
        public:
            Cluster();
            
            // -- methods --

            void add_vertex(PhysicalVertex &vertex);

            // this must be called after last vertex is added
            void compute_initial_characteristics() { shape_matcher.compute_initial_characteristics(); }

            bool is_valid() const { return shape_matcher.is_valid(); }

            void compute_correction(Math::Real dt);

            // -- getters/setters --

            int get_vertices_num() const { return shape_matcher.get_vertices_num(); }

            PhysicalVertex & get_vertex(int index) { return shape_matcher.get_vertex(index); }
            const PhysicalVertex & get_vertex(int index) const { return shape_matcher.get_vertex(index); }

            Math::Real get_goal_speed_constant() const { return goal_speed_constant; }
            Math::Real get_linear_elasticity_constant() const { return linear_elasticity_constant; }
            Math::Real get_yield_constant() const { return yield_constant; }
            Math::Real get_creep_constant() const { return creep_constant; }

            const Math::Vector & get_center_of_mass() const { return shape_matcher.get_center_of_mass(); }
            const Math::Matrix & get_rotation() const { return shape_matcher.get_rotation(); }
            const Math::Matrix & get_linear_transformation() const { return shape_matcher.get_linear_transformation(); }
            const Math::Matrix & get_total_deformation() const { return total_deformation; }
            const Math::Matrix & get_plasticity_state() const { return plasticity_state; }
            Math::Real get_relative_plastic_deformation() const;

            static const Math::Real DEFAULT_GOAL_SPEED_CONSTANT;
            static const Math::Real DEFAULT_LINEAR_ELASTICITY_CONSTANT;
            static const Math::Real DEFAULT_YIELD_CONSTANT;
            static const Math::Real DEFAULT_CREEP_CONSTANT;
            static const Math::Real DEFAULT_MAX_DEFORMATION_CONSTANT;
        private:
            // No copying!
            Cluster(const Cluster &);
            Cluster & operator=(const Cluster &);
        };
    }
}

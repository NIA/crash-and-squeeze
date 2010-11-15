#pragma once
#include "Core/core.h"
#include "Core/physical_vertex.h"
#include "Math/vector.h"
#include "Math/matrix.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class Body
        {
        private:
            Collections::Array<PhysicalVertex *> vertices;

            Math::Real total_mass;
            
            Math::Vector center_of_mass;
            Math::Matrix inertia_tensor;
            
            Math::Vector linear_velocity;
            Math::Vector angular_velocity;
            
            Math::Vector linear_velocity_addition;
            Math::Vector angular_velocity_addition;

            // -- init steps -- 
            
            void set_initial_values();
            void add_vertex(PhysicalVertex &v);
            
            // -- internal helpers --
            
            // self-control check
            bool check_total_mass() const;

            typedef const Math::Vector & (PhysicalVertex::*VelocityFunc)() const;
            // common code for compute_velocities and compute_velocities_additions
            bool abstract_compute_velocities(VelocityFunc velocity_func,
                                             /*out*/ Math::Vector & res_linear_velocity,
                                             /*out*/ Math::Vector & res_angular_velocity);

        public:
            // creates rigid body from all given vertices
            Body(PhysicalVertexArray &all_vertices);
            // creates rigid body only from vertices defined by body_indices
            Body(PhysicalVertexArray &all_vertices, const IndexArray &body_indices);

            // computes center of mass and inertia tensor
            bool compute_properties();
            // computes velocity of center of mass and angular velocity
            bool compute_velocities();
            // computes additions of velocity of center of mass and angular velocity
            bool compute_velocity_additions();
            
            // compensates given linear and angular velocity 
            void compensate_velocities(const Math::Vector &linear, const Math::Vector &angular);

            static const Math::Real MAX_RIGIDITY_COEFF;
            // Makes velocities of vertices close to those they would have if they were
            // moving as a part of given rigid body, with its linear velocity and
            // with its angular velocity around its center of mass.
            // Argument `coeff' specifies how close to those velocities they will be:
            // 1 - equal, absolutely rigid body; 0 - no change.
            void set_rigid_motion(const Body & body, Math::Real coeff = MAX_RIGIDITY_COEFF);
            // enforces rigid motion of body itself
            void set_rigid_motion(Math::Real coeff = MAX_RIGIDITY_COEFF) { set_rigid_motion(*this, coeff); }

            Math::Real get_total_mass() const { return total_mass; }
            const Math::Vector & get_center_of_mass() const { return center_of_mass; }
            const Math::Matrix & get_inertia_tensor() const { return inertia_tensor; }
            const Math::Vector & get_linear_velocity() const { return linear_velocity; }
            const Math::Vector & get_angular_velocity() const { return angular_velocity; }
            const Math::Vector & get_linear_velocity_addition() const { return linear_velocity_addition; }
            const Math::Vector & get_angular_velocity_addition() const { return angular_velocity_addition; }
        };
    }
}

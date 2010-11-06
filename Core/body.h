#pragma once
#include "Core/core.h"
#include "Core/physical_vertex.h"
#include "Collections/array.h"
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

            // init steps
            void set_initial_values();
            void add_vertex(PhysicalVertex &v, bool rigid);
            
            // self-control check
            bool check_total_mass() const;

            typedef const Math::Vector & (PhysicalVertex::*VelocityFunc)() const;
            // common code for compute_velocities and compute_velocities_additions
            bool abstract_compute_velocities(VelocityFunc velocity_func,
                                             /*out*/ Math::Vector res_linear_velocity,
                                             /*out*/ Math::Vector res_angular_velocity);

        public:
            typedef Collections::Array<PhysicalVertex> PhysicalVertexArray;

            // creates rigid body from all given vertices.
            // if `rigid' is true, PhysicalVertex::fix is called for each vertex
            Body(PhysicalVertexArray &all_vertices, bool rigid = false);
            // creates rigid body only from vertices defined by body_indices
            Body(PhysicalVertexArray &all_vertices, const IndexArray &body_indices, bool rigid = false);

            // computes center of mass and inertia tensor
            bool compute_properties();
            // computes velocity of center of mass and angular velocity
            bool compute_velocities();
            // computes additions of velocity of center of mass and angular velocity
            bool compute_velocity_additions();
            
            // compensates given linear and angular velocity 
            void compensate_velocities(const Math::Vector &linear, const Math::Vector &angular);

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

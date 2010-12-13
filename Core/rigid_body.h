#pragma once
#include "Math/floating_point.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // A kinematic model of rigid body
        class RigidBody
        {
        private:
            Math::Vector position;
            Math::Vector rotation;
            Math::Vector velocity;
            Math::Vector angular_velocity;

            Math::Matrix rotation_matrix;

            bool rotation_matrix_outdated;
            void recompute_rotation_matrix();

            void set_rotation(const Math::Vector & value);

        public:
            RigidBody(const Math::Vector & position,
                      const Math::Vector & rotation,
                      const Math::Vector & velocity = Math::Vector::ZERO,
                      const Math::Vector & angular_velocity = Math::Vector::ZERO);

            const Math::Vector & get_position() const { return position; }
            const Math::Vector & get_rotation() const { return rotation; }
            const Math::Vector & get_velocity() const { return velocity; }
            const Math::Vector & get_angular_velocity() const { return angular_velocity; }

            void add_to_velocity(const Math::Vector & addition) { velocity += addition; }
            void add_to_angular_velocity(const Math::Vector & addition) { angular_velocity += addition; }

            void integrate(Math::Real dt,
                           const Math::Vector & acceleration = Math::Vector::ZERO,
                           const Math::Vector & angular_acceleration = Math::Vector::ZERO);

            const Math::Matrix & get_rotation_matrix();
        };
    }
}

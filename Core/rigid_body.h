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
            Math::Matrix orientation;
            Math::Vector velocity;
            Math::Vector angular_velocity;

        public:
            RigidBody(const Math::Vector & position,
                      const Math::Matrix & orientation = Math::Matrix::IDENTITY,
                      const Math::Vector & velocity = Math::Vector::ZERO,
                      const Math::Vector & angular_velocity = Math::Vector::ZERO);

            const Math::Vector & get_position() const { return position; }
            const Math::Matrix & get_orientation() const { return orientation; }
            const Math::Vector & get_velocity() const { return velocity; }
            const Math::Vector & get_angular_velocity() const { return angular_velocity; }

            void add_to_velocity(const Math::Vector & addition) { velocity += addition; }
            void add_to_angular_velocity(const Math::Vector & addition) { angular_velocity += addition; }

            void integrate(Math::Real dt,
                           const Math::Vector & acceleration = Math::Vector::ZERO,
                           const Math::Vector & angular_acceleration = Math::Vector::ZERO);

            // static helpers for matrices
            static Math::Matrix x_rotation_matrix(Math::Real angle);
            static Math::Matrix y_rotation_matrix(Math::Real angle);
            static Math::Matrix z_rotation_matrix(Math::Real angle);

            // for vector V returns such matrix A that for each vector U
            // A*U == cross_product(V, U)
            static Math::Matrix cross_product_matrix(Math::Vector v);
        };
    }
}

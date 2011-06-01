#pragma once
#include "Math/floating_point.h"
#include "Math/vector.h"
#include "Math/matrix.h"
#include "Core/ibody.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // A kinematic model of rigid body
        class RigidBody : public IBody
        {
        private:
            Math::Vector position;
            Math::Matrix orientation;
            Math::Vector linear_velocity;
            Math::Vector angular_velocity;

        public:
            RigidBody(const Math::Vector & position = Math::Vector::ZERO,
                      const Math::Matrix & orientation = Math::Matrix::IDENTITY,
                      const Math::Vector & linear_velocity = Math::Vector::ZERO,
                      const Math::Vector & angular_velocity = Math::Vector::ZERO);

            void setup(const Math::Vector & position,
                       const Math::Matrix & orientation,
                       const Math::Vector & linear_velocity,
                       const Math::Vector & angular_velocity);

            virtual /*override*/ const Math::Vector & get_position() const { return position; }
            const Math::Matrix & get_orientation() const { return orientation; }
            virtual /*override*/ const Math::Vector & get_linear_velocity() const { return linear_velocity; }
            virtual /*override*/ const Math::Vector & get_angular_velocity() const { return angular_velocity; }

            void set_linear_velocity(const Math::Vector & value) { linear_velocity = value; }
            void set_angular_velocity(const Math::Vector & value) { angular_velocity = value; }
            void add_to_linear_velocity(const Math::Vector & value) { linear_velocity += value; }
            void add_to_angular_velocity(const Math::Vector & value) { angular_velocity += value; }
            void set_motion(const IBody &body);

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

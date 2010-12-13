#include "rigid_body.h"
#include <cmath>

namespace CrashAndSqueeze
{
    using Math::Real;
    using Math::Vector;
    using Math::Matrix;
    using Math::MATRIX_ELEMENTS_NUM;

    namespace Core
    {
        RigidBody::RigidBody(const Vector & position, const Vector & rotation,
                  const Vector & velocity, const Vector & angular_velocity)
            : position(position),
              velocity(velocity),
              angular_velocity(angular_velocity)
        {
            set_rotation(rotation);
        }

        void RigidBody::set_rotation(const Vector & value)
        {
            rotation = value;
            rotation_matrix_outdated = true;
        }
        
        void RigidBody::integrate(Real dt, const Vector & acceleration, const Vector & angular_acceleration)
        {
            velocity += acceleration*dt;
            angular_velocity += angular_acceleration*dt;

            position += velocity*dt;
            set_rotation(rotation + angular_velocity*dt);
        }

        void RigidBody::recompute_rotation_matrix()
        {
            const Real x_angle = rotation[0];
            const Real y_angle = rotation[1];
            const Real z_angle = rotation[2];

            const Real x_values[MATRIX_ELEMENTS_NUM] = 
            {
                1,            0,             0,
                0, cos(x_angle), -sin(x_angle),
                0, sin(x_angle),  cos(x_angle),
            };

            const Real y_values[MATRIX_ELEMENTS_NUM] = 
            {
                 cos(y_angle), 0, sin(y_angle),
                            0, 1,            0,
                -sin(y_angle), 0, cos(y_angle),
            };

            const Real z_values[MATRIX_ELEMENTS_NUM] = 
            {
                cos(z_angle), -sin(z_angle), 0, 
                sin(z_angle),  cos(z_angle), 0, 
                           0,             0, 1, 
            };
            
            rotation_matrix = Matrix(z_values)*Matrix(y_values)*Matrix(x_values);
        }

        const Matrix & RigidBody::get_rotation_matrix()
        {
            if(rotation_matrix_outdated)
            {
                recompute_rotation_matrix();
                rotation_matrix_outdated = false;
            }
            return rotation_matrix;
        }
    }
}

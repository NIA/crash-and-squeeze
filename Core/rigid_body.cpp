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
        RigidBody::RigidBody(const Vector & position, const Matrix & orientation,
                             const Vector & linear_velocity, const Vector & angular_velocity)
            : position(position),
              orientation(orientation),
              linear_velocity(linear_velocity),
              angular_velocity(angular_velocity)
        {}

        void RigidBody::set_motion(const IBody &body)
        {
            set_linear_velocity( body.get_linear_velocity() +
                                 cross_product(body.get_angular_velocity(), -body.get_position()) );
            set_angular_velocity( body.get_angular_velocity() );
        }
        
        void RigidBody::integrate(Real dt, const Vector & acceleration, const Vector & angular_acceleration)
        {
            linear_velocity += acceleration*dt;
            angular_velocity += angular_acceleration*dt;

            position += linear_velocity*dt;
            orientation += cross_product_matrix(angular_velocity*dt)*orientation;
            inverted_orientation = orientation.inverted();
        }

        Matrix RigidBody::cross_product_matrix(Vector v)
        {
            const Real values[MATRIX_ELEMENTS_NUM] =
            {
                    0, -v[2],  v[1],
                 v[2],     0, -v[0],
                -v[1],  v[0],     0,
            };
            return Matrix(values);
        }

        Matrix RigidBody::x_rotation_matrix(Real angle)
        {
            const Real values[MATRIX_ELEMENTS_NUM] = 
            {
                1,          0,           0,
                0, cos(angle), -sin(angle),
                0, sin(angle),  cos(angle),
            };
            return Matrix(values);
        }

        Matrix RigidBody::y_rotation_matrix(Real angle)
        {
            const Real values[MATRIX_ELEMENTS_NUM] = 
            {
                 cos(angle), 0, sin(angle),
                          0, 1,          0,
                -sin(angle), 0, cos(angle),
            };
            return Matrix(values);
        }

        Matrix RigidBody::z_rotation_matrix(Real angle)
        {
            const Real values[MATRIX_ELEMENTS_NUM] = 
            {
                cos(angle), -sin(angle), 0, 
                sin(angle),  cos(angle), 0, 
                         0,           0, 1, 
            };
            return Matrix(values);
        }
    }
}

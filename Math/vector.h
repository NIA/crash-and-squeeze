#pragma once
#include <iostream>
#include <cmath>
#include "floating_point.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        // A Vector in 3D space.
        // For matrix operations assumed to be a column-vector
        class Vector
        {
        public:
            double x, y, z;

            Vector() : x(0), y(0), z(0) {}
            Vector(double x, double y, double z) : x(x), y(y), z(z) {}

            // unary operators
            Vector operator-() const
            {
                return Vector( -x, -y, -z );
            }
            Vector operator+() const
            {
                return *this;
            }

            // assignment operators
            Vector & operator+=(const Vector &another)
            {
                x += another.x;
                y += another.y;
                z += another.z;
                return *this;
            }
            Vector & operator-=(const Vector &another)
            {
                x -= another.x;
                y -= another.y;
                z -= another.z;
                return *this;
            }

            Vector & operator*=(const double &scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
                return *this;
            }
            Vector & operator/=(const double &scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
                return *this;
            }

            // binary operators
            Vector operator+(const Vector &another) const
            {
                Vector result = *this;
                return result += another;
            }
            Vector operator-(const Vector &another) const
            {
                Vector result = *this;
                return result -= another;
            }

            Vector operator*(const double &scalar) const
            {
                Vector result = *this;
                return result *= scalar;
            }
            Vector operator/(const double &scalar) const
            {
                Vector result = *this;
                return result /= scalar;
            }

            bool operator==(const Vector &another) const
            {
                return equal( x, another.x ) && equal( y, another.y ) && equal( z, another.z );
            }
            bool operator!=(const Vector &another) const
            {
                return !( *this == another );
            }
            
            // scalar multiplication
            double operator*(const Vector &another) const
            {
                return x*another.x + y*another.y + z*another.z;
            }
            
            // methods
            double sqared_norm() const
            {
                return (*this)*(*this);
            }
            double norm() const
            {
                return sqrt( sqared_norm() );
            }
            Vector & normalize() // normalizes given point/vector in place (!), returns itself
            {
                if( norm() != 0 )
                {
                    (*this) /= norm();
                }
                return *this;
            }
            Vector normalized() const  // returns normalized point/vector
            {
                Vector result = *this;
                return result.normalize();
            }

            bool is_zero() const
            {
                return equal( 0, x ) && equal( 0, y ) && equal( 0, z );
            }
            bool is_collinear_to(const Vector &another) const;
            bool is_orthogonal_to(const Vector &another) const
            {
                return equal( 0, (*this)*another );
            }
        };

        typedef Vector Point; // define an alias

        // more operators
        inline Vector operator*(const double &scalar, const Vector &vector)
        {
            return vector * scalar;
        }
        inline std::ostream &operator<<(std::ostream &stream, const Vector &vector)
        {
            return stream << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
        }

        // functions
        inline double distance(const Point &A, const Point &B)
        {
            return (A - B).norm();
        }
        inline Vector cross_product(const Vector &a, const Vector &b)
        {
            return Vector( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x );
        }

        inline bool Vector::is_collinear_to(const Vector &another) const
        {
            return cross_product( *this, another ).is_zero();
        }
    };
};

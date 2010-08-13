#pragma once
#include <cmath>
#include "floating_point.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        const int VECTOR_SIZE = 3;
        // A Vector in 3D space.
        // For matrix operations assumed to be a column-vector
        class Vector
        {
        private:
            double values[VECTOR_SIZE];
        public:
            Vector() { values[0] = 0; values[1] = 0; values[2] = 0; }
            Vector(double x, double y, double z) { values[0] = x; values[1] = y; values[2] = z; }

            static const Vector ZERO;

            // -- getters/setters --

            // indices in vector: 0 to 2
            double operator[](int index) const
            {
                assert(index >= 0);
                assert(index < VECTOR_SIZE);
                return values[index];
            }
            double & operator[](int index)
            {
                assert(index >= 0);
                assert(index < VECTOR_SIZE);
                return values[index];
            }

            // -- assignment operators --

            Vector & operator+=(const Vector &another)
            {
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    values[i] += another[i];
                return *this;
            }
            Vector & operator-=(const Vector &another)
            {
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    values[i] -= another[i];
                return *this;
            }

            Vector & operator*=(const double &scalar)
            {
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    values[i] *= scalar;
                return *this;
            }
            Vector & operator/=(const double &scalar)
            {
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    values[i] /= scalar;
                return *this;
            }

            // -- binary arithmetic operators --

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
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    if( ! equal(values[i], another[i]) )
                        return false;
                return true;
            }
            bool operator!=(const Vector &another) const
            {
                return !( *this == another );
            }

            // -- unary operators --

            Vector operator-() const
            {
                return (*this)*(-1);
            }
            Vector operator+() const
            {
                return *this;
            }
            
            // scalar multiplication
            double operator*(const Vector &another) const
            {
                double result = 0;
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    result += values[i]*another[i];
                return result;
            }
            
            // -- methods --

            double sqared_norm() const
            {
                return (*this)*(*this);
            }
            double norm() const
            {
                return sqrt( sqared_norm() );
            }
            // normalizes given point/vector in place (!), returns itself
            Vector & normalize()
            {
                if( norm() != 0 )
                {
                    (*this) /= norm();
                }
                return *this;
            }
            // returns normalized point/vector
            Vector normalized() const
            {
                Vector result = *this;
                return result.normalize();
            }

            bool is_zero() const
            {
                return *this == Vector::ZERO;
            }
            bool is_collinear_to(const Vector &another) const;
            bool is_orthogonal_to(const Vector &another) const
            {
                return equal( 0, (*this)*another );
            }
        };

        // define an alias
        typedef Vector Point;

        // -- more operators --
        
        inline Vector operator*(const double &scalar, const Vector &vector)
        {
            return vector * scalar;
        }
        
        // -- functions --
        
        inline double distance(const Point &A, const Point &B)
        {
            return (A - B).norm();
        }
        inline Vector cross_product(const Vector &a, const Vector &b)
        {
            return Vector( a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0] );
        }

        inline bool Vector::is_collinear_to(const Vector &another) const
        {
            return cross_product( *this, another ).is_zero();
        }
    };
};

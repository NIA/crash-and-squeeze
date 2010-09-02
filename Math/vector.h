#pragma once
#include <cmath>
#include "Logging/logger.h"
#include "Math/floating_point.h"

namespace CrashAndSqueeze
{
    using Logging::logger;

    namespace Math
    {
        const int VECTOR_SIZE = 3;
        // A Vector in 3D space.
        // For matrix operations assumed to be a column-vector
        class Vector
        {
        private:
            Real values[VECTOR_SIZE];
            bool check_index(int index) const
            {
                if(index < 0 || index >= VECTOR_SIZE)
                {
                    logger.error("Vector index out of range", __FILE__, __LINE__);
                    return false;
                }
                return true;
            }
        public:
            Vector() { values[0] = 0; values[1] = 0; values[2] = 0; }
            Vector(Real x, Real y, Real z) { values[0] = x; values[1] = y; values[2] = z; }

            static const Vector ZERO;

            // -- getters/setters --

            // indices in vector: 0 to 2
            Real operator[](int index) const
            {
                if(check_index(index))
                    return values[index];
                else
                    return values[0];
            }
            Real & operator[](int index)
            {
                if(check_index(index))
                    return values[index];
                else
                    return values[0];
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

            Vector & operator*=(const Real &scalar)
            {
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    values[i] *= scalar;
                return *this;
            }
            Vector & operator/=(const Real &scalar)
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

            Vector operator*(const Real &scalar) const
            {
                Vector result = *this;
                return result *= scalar;
            }
            Vector operator/(const Real &scalar) const
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
            Real operator*(const Vector &another) const
            {
                Real result = 0;
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    result += values[i]*another[i];
                return result;
            }
            
            // -- methods --

            Real squared_norm() const
            {
                return (*this)*(*this);
            }
            Real norm() const
            {
                return sqrt( squared_norm() );
            }
            // normalizes given point/vector in place (!), returns itself
            Vector & normalize()
            {
                if( norm() != 0 )
                    (*this) /= norm();
                else
                    logger.warning("attempting to normalize zero Vector, the Vector left unchanged", __FILE__, __LINE__);
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
        
        inline Vector operator*(const Real &scalar, const Vector &vector)
        {
            return vector * scalar;
        }
        
        // -- functions --
        
        inline Real distance(const Point &A, const Point &B)
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

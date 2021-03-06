#pragma once
#include <cmath>
#include <cfloat>

// Helpers for 'proper' comparing floating point numbers: assuming equal those ones,
// whose difference is less than given epsilon

namespace CrashAndSqueeze
{
    namespace Math
    {
        // floating-point type of internal calculations
        typedef double Real;
        const Real MAX_REAL = DBL_MAX; 
        
        const Real DEFAULT_REAL_PRECISION = 1e-9;

        inline bool equal(Real a, Real b, Real epsilon = DEFAULT_REAL_PRECISION)
        {
            return fabs(a - b) <= epsilon;
        }

        inline bool less_or_equal(Real a, Real b, Real epsilon = DEFAULT_REAL_PRECISION)
        {
            return (a < b) || equal( a, b, epsilon );
        }

        inline bool greater_or_equal(Real a, Real b, Real epsilon = DEFAULT_REAL_PRECISION)
        {
            return (a > b) || equal( a, b, epsilon );
        }

        inline int sign(Real x)
        {
            return (x > 0) - (x < 0);
        }

        template <typename T> /* requires LessThanComparable<T> */
        inline T minimum(T a, T b)
        {
            return (a < b) ? a : b;
        }

        inline Real cube_root(Real value)
        {
            return pow( abs(value), 1.0/3)*sign(value);
        }
    };
};

#pragma once
#include <assert.h>
#include <cstdlib>
#include <cmath>

// Helpers for 'proper' comparing floating point numbers: assuming equal those ones,
// whose difference is less than given max_ulps Units in the Last Place
// (or whose difference is less than given epsilon - for comparison near zero)

namespace CrashAndSqueeze
{
    namespace Math
    {
        union DoubleAndInt
        {
            double double_;
            long long int_;
        };

        const long long DEFAULT_MAX_ULPS = 50;
        const double DEFAULT_EPSILON = 1e-12;

#ifdef _MSC_VER
        inline long long llabs(long long x) { return _abs64(x); }
#endif //#ifdef _MSC_VER

        // TODO: tests for these functions

        //
        // the idea from
        // 'Comparing floating point numbers' by Bruce Dawson
        // http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
        //
        inline bool equal(double a, double b, double epsilon = DEFAULT_EPSILON, long long max_ulps = DEFAULT_MAX_ULPS)
        {
            assert( sizeof(long long) == sizeof(double) );
            // Make sure max_ulps is non-negative and small enough that the
            // default NAN won't compare as equal to anything.
            //TODO: errors
            assert( max_ulps > 0 && max_ulps < 4 * 1024 * 1024 ); // this is maximum ULPS for floats, for doubles it might be greater, but for what?
            
            // epsilon-comparison: needed near zero
            if( fabs(a - b) <= epsilon )
            {
                return true;
            }
            
            DoubleAndInt a_union = { a };
            DoubleAndInt b_union = { b };
            
            long long a_int = a_union.int_;
            long long b_int = b_union.int_;

            DoubleAndInt minus_null_union = { -0.0 };
            long long minus_null_int = minus_null_union.int_;
            // Make a_int lexicographically ordered as a twos-complement int
            if (a_int < 0)
                a_int = minus_null_int - a_int;
            // Make b_int lexicographically ordered as a twos-complement int
            if (b_int < 0)
                b_int = minus_null_int - b_int;
           
            long long int_diff = llabs(a_int - b_int);
            if (int_diff <= max_ulps)
                return true;

            return false;
        }

        inline bool less_or_equal(double a, double b, double epsilon = DEFAULT_EPSILON, long long max_ulps = DEFAULT_MAX_ULPS)
        {
            return (a < b) || equal( a, b, epsilon, max_ulps );
        }

        inline bool greater_or_equal(double a, double b, double epsilon = DEFAULT_EPSILON, long long max_ulps = DEFAULT_MAX_ULPS)
        {
            return (a > b) || equal( a, b, epsilon, max_ulps );
        }
        inline int sign(double x)
        {
            return (x > 0) - (x < 0);
        }
    };
};

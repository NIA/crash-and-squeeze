#pragma once
#include "Math/matrix.h"

// This file contains different classes and methods that can be used
// in addition to usual Math::Vector and Math::Matrix in order to implement
// quadratic extensions (QX) of shape matching algorithm.
// In contrast to rather complete classes Math::Vector and Math::Matrix,
// only operations required for QX are implemented.

namespace CrashAndSqueeze
{
    namespace Math
    {
        // For many entities here we have three components:
        // linear (like x), pure quadratic (like x*x), mixed (like x*y)
        const int COMPONENTS_NUM = 3;
        
        // A generic triple (can be matrices or vectors)
        template <class T>
        class Triple
        {
        private:
            T components[COMPONENTS_NUM];
            bool check_index(int index) const
            {
                if(index < 0 || index >= COMPONENTS_NUM)
                {
                    Logging::Logger::error("Triple component index out of range", __FILE__, __LINE__);
                    return false;
                }
                return true;
            }
        public:
            Triple() {}
            Triple(const T & lin, const T & quad, const T & mix)
            {
                components[0] = lin;
                components[1] = quad;
                components[2] = mix;
            }

            // return i'th component of triple (i: 0 to 2)
            const T & operator[](int index) const
            {
            #ifndef NDEBUG
                if(false == check_index(index))
                    return components[0];
                else
            #endif // #ifndef NDEBUG
                    return components[index];
            }

            T & operator[](int index)
            {
            #ifndef NDEBUG
                if(false == check_index(index))
                    return components[0];
                else
            #endif // #ifndef NDEBUG
                    return components[index];
            }
        };

        // A 9-component vector. When used in QX,
        //  - first 3 components are usual coordinates (x, y, z),
        //  - other 3 components are pure quadratic terms (x*x, y*y, z*z),
        //  - last  3 components are mixed terms (x*y, y*z, z*y).
        typedef Triple<Vector> TriVector;

        // A 3x9 matrix. When used in QX, it can be represented as
        // three matrices [A Q M]:
        //  - A is usual linear transformation
        //  - Q is transformation of pure quadratic terms
        //  - M is transformation of mixed terms.
        typedef Triple<Matrix> TriMatrix;

        // A 9x9 matrix (that is, 3x3=9 normal matrices)
        // It is a triple of "columns", each of which is a triple of matrices.
        // If m is a NineMatrix, then m[j] is the j'th "column" of matrices,
        // it is a TriMatrix, so m[j][i] is the Matrix in i'th row and j'th column
        // WARNING: this means that 
        typedef Triple<TriMatrix> NineMatrix;

        void vec2triple(/*in*/ const Vector & v, /*out*/ TriVector & res);

        // Multiplication of TriMatrix and TriVector: yields Vector.
        Vector operator*(const TriMatrix & m, const TriVector & v);

        // Multiplication of TriMatrix and NineMatrix: yields TriMatrix
        // Implemented as a function (instead of operator*) because
        // returning big TriMatrix would require much copying
        void qx_mult(/*in*/  const TriMatrix  & m3,
                     /*in*/  const NineMatrix & m9,
                     /*out*/ TriMatrix & res);
    }
}
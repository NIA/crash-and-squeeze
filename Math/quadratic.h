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
        
        // A generic triple (can be matrices or vectors or etc)
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
        class TriVector {
        public:
            Triple<Vector> vectors;

            TriVector() {}
            // Initialize all three parts given first (linear) part
            TriVector(const Vector & v);
            // Initialize each part directly
            TriVector(const Vector & lin, const Vector & quad, const Vector & mix)
                : vectors(lin, quad, mix)
            {}
        };

        // A 3x9 matrix. When used in QX, it can be represented as
        // three matrices [A Q M]:
        //  - A is usual linear transformation
        //  - Q is transformation of pure quadratic terms
        //  - M is transformation of mixed terms.
        class TriMatrix {
        public:
            Triple<Matrix> matrices;

            TriMatrix() {}
            // construct trimatrix as outer product of vector and trivector
            TriMatrix(const Vector &left_vector, const TriVector &right_vector);
            // inititalize each part directly
            TriMatrix(const Matrix & lin, const Matrix & quad, const Matrix & mix)
                : matrices(lin, quad, mix)
            {}

            TriMatrix & operator+=(const TriMatrix & another);
            TriMatrix & operator*=(const Real & scalar);
            
            // Multiplication of TriMatrix and TriVector: yields Vector.
            Vector operator*(const TriVector & v) const;
        };

        // A 9x9 matrix (that is, 3x3=9 normal matrices)
        // It is a triple of "columns", each of which is a triple of matrices.
        // If m is a NineMatrix, then m[j] is the j'th "column" of matrices,
        // it is a TriMatrix, so m[j][i] is the Matrix in i'th row and j'th column
        // WARNING: this means that order of indexes is swapped compared to normal matrix
        class NineMatrix {
        public:
            Triple<TriMatrix> columns;

            NineMatrix() {}
            // construct ninematrix as outer product of two trivectors
            NineMatrix(const TriVector &left_vector, const TriVector &right_vector);

            NineMatrix & operator+=(const NineMatrix &another);

            // Multiplication of TriMatrix and NineMatrix: yields TriMatrix
            // Implemented as a function (instead of operator*) because
            // returning big TriMatrix would require much copying
            void left_mult_by(/*in*/  const TriMatrix  & m3,
                              /*out*/ TriMatrix & res) const;

            // Inverts matrix (in place). Returns true if successfully
            // inverted, false if singular
            bool invert();
        };

    }
}

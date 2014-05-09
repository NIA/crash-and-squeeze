#include "Math/quadratic.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        void vec2triple(/*in*/ const Vector & v, /*out*/ TriVector & res)
        {
            res[0] = v;
            Vector & vq = res[1]; // pure quadratic:
            for (int i = 0; i < VECTOR_SIZE; ++i)
            {
                vq[i] = v[i]*v[i]; // x*x, y*y, z*z
            }
            Vector & vm = res[2]; // mixed:
            vm[0] = v[0]*v[1]; // x*y
            vm[1] = v[1]*v[2]; // y*z
            vm[2] = v[2]*v[0]; // z*x
        }

        Vector operator*(const TriMatrix & m, const TriVector & v)
        {
            // If m = [A Q M] and v = [va; vq; vm], then
            // m * v = A*va + Q*vq + M*vm
            return m[0]*v[0] + m[1]*v[1] + m[2]*v[2];
        }

        void qx_mult(/*in*/  const TriMatrix  & m3,
                     /*in*/  const NineMatrix & m9,
                     /*out*/ TriMatrix & res)
        {
            for (int j = 0; j < COMPONENTS_NUM; ++j) {
                const TriMatrix & column = m9[j];
                res[j] = m3[0]*column[0] + m3[1]*column[1] + m3[2]*column[2];
            }
        }
    }
}
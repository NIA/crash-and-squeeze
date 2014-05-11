#include "Math/quadratic.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        TriVector::TriVector(const Vector & v)
        {
            vectors[0] = v;
            Vector & vq = vectors[1]; // pure quadratic:
            for (int i = 0; i < VECTOR_SIZE; ++i)
            {
                vq[i] = v[i]*v[i]; // x*x, y*y, z*z
            }
            Vector & vm = vectors[2]; // mixed:
            vm[0] = v[0]*v[1]; // x*y
            vm[1] = v[1]*v[2]; // y*z
            vm[2] = v[2]*v[0]; // z*x
        }

        TriMatrix::TriMatrix(const Vector &left_vector, const TriVector &right_vector)
            // construct trimatrix as outer product of vector and trivector (do this for each part)
            : matrices(Matrix(left_vector, right_vector.vectors[0]),
                       Matrix(left_vector, right_vector.vectors[1]),
                       Matrix(left_vector, right_vector.vectors[2]))
        {}

        TriMatrix & TriMatrix::operator+=(const TriMatrix & another)
        {
            for (int i = 0; i < COMPONENTS_NUM; ++i)
            {
                matrices[i] += another.matrices[i];
            }
            return *this;
        }

        TriMatrix & TriMatrix::operator*=(const Real & scalar)
        {
            for (int i = 0; i < COMPONENTS_NUM; ++i)
            {
                matrices[i] *= scalar;
            }
            return *this;
        }

        Vector TriMatrix::operator*(const TriVector & v) const
        {
            // If m = [A Q M] and v = [va; vq; vm], then
            // m * v = A*va + Q*vq + M*vm
            return matrices[0]*v.vectors[0] + matrices[1]*v.vectors[1] + matrices[2]*v.vectors[2];
        }

        NineMatrix::NineMatrix(const TriVector &left_vector, const TriVector &right_vector)
        {
            for (int i = 0; i < COMPONENTS_NUM; ++i)
            {
                for (int j = 0; j < COMPONENTS_NUM; ++j)
                {
                    // TODO: make setter to avoid copying?
                    columns[j].matrices[i] = Matrix(left_vector.vectors[i], right_vector.vectors[j]);
                }
            }
        }

        NineMatrix & NineMatrix::operator+=(const NineMatrix &another)
        {
            for (int j = 0; j < COMPONENTS_NUM; ++j)
            {
                columns[j] += another.columns[j];
            }
            return *this;
        }

        void NineMatrix::left_mult_by(/*in*/  const TriMatrix  & m3,
                                      /*out*/ TriMatrix & res) const
        {
            for (int j = 0; j < COMPONENTS_NUM; ++j)
            {
                const TriMatrix & col = columns[j];
                res.matrices[j] = m3.matrices[0]*col.matrices[0]
                                + m3.matrices[1]*col.matrices[1]
                                + m3.matrices[2]*col.matrices[2];
            }
        }

        bool NineMatrix::invert() {
            Logging::Logger::error("NineMatrix::invert not yet implemented!", __FILE__, __LINE__);
            return false;
        }
    }
}

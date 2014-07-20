#include "Math/quadratic.h"
// TODO: avoid external dependency!
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

namespace CrashAndSqueeze
{
    using Logging::Logger;
    namespace Math
    {
        TriVector::TriVector(const Vector & v)
        {
            set_vector(v);
        }

        void TriVector::set_vector(const Vector &v)
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

        CrashAndSqueeze::Math::TriVector TriVector::operator*(const Real &scalar) const
        {
            return TriVector(vectors[0]*scalar, vectors[1]*scalar, vectors[2]*scalar);
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

        void TriMatrix::set_all(const Real &value)
        {
            for (int i = 0; i < COMPONENTS_NUM; ++i)
            {
                matrices[i].set_all(value);
            }
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

        NineMatrix::NineMatrix(const Real values[SIZE][SIZE])
        {
            for (int i = 0; i < SIZE; ++i)
            {
                for (int j = 0; j < SIZE; ++j)
                {
                    set_at(i, j, values[i][j]);
                }
            }
        }

        Real NineMatrix::get_at(int i, int j) const
        {
            // TODO: check boundaries
            return columns[j / 3].matrices[i / 3].get_at(i % 3, j % 3);
        }

        void NineMatrix::set_at(int i, int j, Real value)
        {
            // TODO: check boundaries
            return columns[j / 3].matrices[i / 3].set_at(i % 3, j % 3, value);
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

        bool NineMatrix::invert()
        {
            // Create GSL matrix
            gsl_matrix * m = gsl_matrix_alloc(SIZE, SIZE);
            // TODO: avoid such copying! Change internal representation of NineMatrix to array[81]
            for (int i = 0; i < SIZE; ++i)
            {
                for (int j = 0; j < SIZE; ++j)
                {
                    gsl_matrix_set(m, i, j, get_at(i, j));
                }
            }
            // TODO: avoid allocation! (Use gsl_matrix_view_array ?)
            gsl_matrix * inverse = gsl_matrix_alloc (SIZE, SIZE);
            gsl_permutation * perm = gsl_permutation_alloc (SIZE);
            int signum; // sign of permutation - used for determinant

            // Make LU decomposition of matrix m (both L and U are encoded in matrix m)
            gsl_linalg_LU_decomp(m, perm, &signum);
            // Find determinant from LU decomposition 
            double det = gsl_linalg_LU_det(m, signum);
            if (equal(0, det))
            {
                Logger::error("inverting singular NineMatrix (determinant == 0)", __FILE__, __LINE__);
                return false;
            }
            // Invert the matrix `m`, result into `inverse`
            gsl_linalg_LU_invert(m, perm, inverse);
            // TODO: avoid such copying! Change internal representation of NineMatrix to array[81]
            for (int i = 0; i < SIZE; ++i)
            {
                for (int j = 0; j < SIZE; ++j)
                {
                    set_at(i, j, gsl_matrix_get(inverse, i, j));
                }
            }
            gsl_matrix_free(m);
            gsl_matrix_free(inverse);
            gsl_permutation_free(perm);
            return true;
        }

        void NineMatrix::set_all(const Real &value)
        {
            for (int j = 0; j < COMPONENTS_NUM; ++j)
            {
                columns[j].set_all(value);
            }
        }

    }
}

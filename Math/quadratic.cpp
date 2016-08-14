#include "Math/quadratic.h"
#ifndef NDEBUG
#include <iostream>// for NineMatrix::print
#include <iomanip> // for NineMatrix::print
#endif // !NDEBUG

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

        bool TriVector::operator==(const TriVector &another) const
        {
            return vectors[0] == another.vectors[0] &&
                   vectors[1] == another.vectors[1] &&
                   vectors[2] == another.vectors[2];
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

        bool TriMatrix::operator==(const TriMatrix &another) const
        {
            return matrices[0] == another.matrices[0] &&
                   matrices[1] == another.matrices[1] &&
                   matrices[2] == another.matrices[2];
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
                    columns[j].matrices[i].set_outer_product(left_vector.vectors[i], right_vector.vectors[j]);
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

        void NineMatrix::make_unit()
        {
            set_all(0);
            for (int i = 0; i < SIZE; ++i)
                set_at(i, i, 1);
        }

        void NineMatrix::make_rotation(int p, int q, Real sine, Real cosine)
        {
            make_unit();
            set_at(p, p, cosine);
            set_at(q, q, cosine);
            set_at(p, q, -sine);
            set_at(q, p, sine);
        }

        Real NineMatrix::get_at(int i, int j) const
        {
            // TODO: check boundaries
            return columns[j / 3].matrices[i / 3].get_at(i % 3, j % 3);
        }

        void NineMatrix::set_at(int i, int j, Real value)
        {
            // TODO: check boundaries
            columns[j / 3].matrices[i / 3].set_at(i % 3, j % 3, value);
        }

        void NineMatrix::add_at(int i, int j, Real value)
        {
            columns[j / 3].matrices[i / 3].add_at(i % 3, j % 3, value);
        }

        NineMatrix & NineMatrix::operator+=(const NineMatrix &another)
        {
            for (int j = 0; j < COMPONENTS_NUM; ++j)
            {
                columns[j] += another.columns[j];
            }
            return *this;
        }

        bool NineMatrix::operator==(const NineMatrix &another) const
        {
            return columns[0] == another.columns[0] &&
                   columns[1] == another.columns[1] &&
                   columns[2] == another.columns[2];
        }

        NineMatrix & NineMatrix::operator*=(const NineMatrix &another)
        {
            NineMatrix first = *this;
            set_all(0);
            for (int i = 0; i < COMPONENTS_NUM; ++i)
            {
                for (int j = 0; j < COMPONENTS_NUM; ++j)
                {
                    for (int k = 0; k < COMPONENTS_NUM; ++k)
                    {
                        columns[j].matrices[i] += first.submatrix(i, k)*another.submatrix(k, j);
                    }
                }
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

        void NineMatrix::do_jacobi_rotation(int p, int q, NineMatrix & current_transformation)
        {
            // TODO: avoid copy-paste from Matrix::do_jacobi_rotation?
            if (p == q)
            {
                Logger::warning("incorrect Jacobi rotation: `p` and `q` must be different indices", __FILE__, __LINE__);
                return;
            }
            if (equal(0, get_at(p, q))) // already zeroed, no need to rotate
                return;

            // theta is cotangent of Real rotation angle
            Real theta = (get_at(q, q) - get_at(p, p)) / get_at(p, q) / 2;
            // t is sin/cos of rotation angle
            // it is determined from equation t^2 + 2*t*theta - 1 = 0,
            // which implies from definition of theta as (cos^2 - sin^2)/(2*sin*cos)
            Real t = (0 != theta) ? sign(theta) / (abs(theta) + sqrt(theta*theta + 1)) : 1;
            Real cosine = 1 / sqrt(t*t + 1);
            Real sine = t*cosine;
            // tau is tangent of half of rotation angle
            Real tau = sine / (1 + cosine);

            // transform A
            add_at(p, p, -t*get_at(p, q));
            add_at(q, q, +t*get_at(p, q));
            set_at(p, q, 0);
            set_at(q, p, 0);
            for (int r = 0; r < SIZE; ++r)
            {
                if (r != p && r != q)
                {
                    // correction to element at (r,p)
                    Real rp_correction = -sine*(get_at(r, q) + tau*get_at(r, p));
                    // correction to element at (r,q)
                    Real rq_correction = +sine*(get_at(r, p) - tau*get_at(r, q));
                    add_at(r, p, rp_correction);
                    add_at(p, r, rp_correction);
                    add_at(r, q, rq_correction);
                    add_at(q, r, rq_correction);
                }
            }

            // store rotation in R
            for (int r = 0; r < SIZE; ++r)
            {
                Real Rkp = cosine*current_transformation.get_at(r, p) - sine*current_transformation.get_at(r, q);
                Real Rkq = sine*current_transformation.get_at(r, p) + cosine*current_transformation.get_at(r, q);
                current_transformation.set_at(r, p, Rkp);
                current_transformation.set_at(r, q, Rkq);
            }
        }

        void NineMatrix::diagonalize(int rotations_count, NineMatrix & transformation)
        {
            // Start with unit matrix
            transformation.make_unit();

            // TODO: avoid copy-paste from Matrix::diagonalize?
            // Repeat rotations_count times
            for (int iter = 0; iter < rotations_count; ++iter)
            {
                // Find max non-diagonal element
                int p = -1, q = -1;
                Real max = -1;
                for (int i = 0; i < SIZE - 1; ++i)
                {
                    for (int j = i + 1; j < SIZE; ++j)
                    {
                        Real a = fabs(get_at(i, j));
                        if (max < 0 || a > max)
                        {
                            p = i;
                            q = j;
                            max = a;
                        }
                    }
                }
                // If all elements are small enough => done
                if (less_or_equal(max, 0))
                    return;
                // Else nullify the found element
                do_jacobi_rotation(p, q, transformation);
            }
        }

        bool NineMatrix::invert_sym(int diag_rotations)
        {
            // diagonalize
            NineMatrix R;
            NineMatrix diagonalized = *this;
            diagonalized.diagonalize(diag_rotations, R);

            // invert eigenvalues
            Real egnv[SIZE];
            for (int i = 0; i < SIZE; ++i)
            {
                egnv[i] = diagonalized.get_at(i, i);
                if ( ! equal(0, egnv[i]) )
                    egnv[i] = 1 / egnv[i];
            }

            // transform back
            for (int i = 0; i < SIZE; ++i)
            {
                for (int j = 0; j < SIZE ; ++j)
                {
                    set_at(i, j, 0);
                    for (int k = 0; k < SIZE; ++k)
                        add_at(i, j, egnv[k] * R.get_at(i, k) * R.get_at(j, k));
                }
            }

            return true;
        }

        void NineMatrix::set_all(const Real &value)
        {
            for (int j = 0; j < COMPONENTS_NUM; ++j)
            {
                columns[j].set_all(value);
            }
        }

#ifndef NDEBUG
        void NineMatrix::print(char * header /*= ""*/) const
        {
            std::ostream & stream = std::cout;
            stream << std::fixed << header << std::endl;
            for (int i = 0; i < SIZE; ++i)
            {
                for (int j = 0; j < SIZE; ++j)
                {
                    Real val = get_at(i, j);
                    int int_val = (int)round(val);
                    if (equal(val, int_val, 0.001))
                        stream << std::setw(4) << int_val;
                    else
                        stream << std::setprecision(2) << val;
                    stream << ' ';
                }
                stream << std::endl;
            }
        }
#endif // !NDEBUG
    }
}

#include "Math/matrix.h"
#include <cstring>

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Math
    {
        // -- constructors --

        Matrix::Matrix(const Real values[MATRIX_ELEMENTS_NUM])
        {
            memcpy(this->values, values, sizeof(this->values));
        }

        Matrix::Matrix(const Vector &left_vector, const Vector &right_vector)
        {
            for(int i = 0; i < VECTOR_SIZE; ++i)
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    set_at(i, j, left_vector[i]*right_vector[j]);
        }
        
        // -- getters/setters --
        Real Matrix::get_at_index(int index) const
        {
            #ifndef NDEBUG
                if(index < 0 || index >= MATRIX_ELEMENTS_NUM)
                {
                    Logging::Logger::error("Matrix index out of range", __FILE__, __LINE__);
                    return 0;
                }
                else
            #endif //ifndef NDEBUG
                    return values[index];
        }

        Vector Matrix::get_row(int row) const
        {
            Vector result;
            for(int j = 0; j < VECTOR_SIZE; ++j)
                result[j] = get_at(row, j);

            return result;
        }

        Vector Matrix::get_column(int column) const
        {
            Vector result;
            for(int i = 0; i < VECTOR_SIZE; ++i)
                result[i] = get_at(i, column);

            return result;
        }

        // -- assignment operators --

        Matrix & Matrix::operator +=(const Matrix &another)
        {
            for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
                values[i] += another.get_at_index(i);

            return *this;
        }
        
        Matrix & Matrix::operator -=(const Matrix &another)
        {
            for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
                values[i] -= another.get_at_index(i);

            return *this;
        }
        
        Matrix & Matrix::operator *=(const Real &scalar)
        {
            for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
                values[i] *= scalar;

            return *this;
        }
        
        Matrix & Matrix::operator /=(const Real &scalar)
        {
            for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
                values[i] /= scalar;

            return *this;
        }

        // -- binary arithmetic operators --

        bool Matrix::operator==(const Matrix &another) const
        {
            for(int i = 0; i < VECTOR_SIZE; ++i)
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    if( ! equal( get_at(i, j), another.get_at(i, j) ) )
                        return false;
            return true;
        }
        // -- multiplications --
        
        Matrix Matrix::operator*(const Matrix &another) const
        {
            Matrix result;
            Real value;

            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                for(int j = 0; j < VECTOR_SIZE; ++j)
                {
                    value = 0;
                    for(int k =0; k < VECTOR_SIZE; ++k)
                    {
                        value += get_at(i, k)*another.get_at(k, j);
                    }
                    result.set_at(i, j, value);
                }
            }
            return result;
        }
        
        Vector Matrix::operator*(const Vector &vector) const
        {
            Vector result;
            Real value;
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                value = 0;
                for(int k = 0; k < VECTOR_SIZE; ++k)
                {
                    value += get_at(i, k)*vector[k];
                }
                result[i] = value;
            }
            return result;
        }

        Matrix & Matrix::transpose()
        {
            for(int i = 0; i < VECTOR_SIZE - 1; ++i)
                for(int j = i + 1; j < VECTOR_SIZE; ++j)
                    swap( values[ element_index(i,j) ], values[ element_index(j,i)] );
            return *this;
        }

        Matrix Matrix::inverted() const
        {
            Real det = determinant();
            if(equal(0, det))
            {
                Logger::error("inverting singular matrix (determinant == 0)", __FILE__, __LINE__);
                return Matrix::ZERO;
            }

            Matrix cofactors;
            Real value;
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                for(int j = 0; j < VECTOR_SIZE; ++j)
                {
                    value = get_at((i+1)%3, (j+1)%3)*get_at((i+2)%3, (j+2)%3)
                          - get_at((i+1)%3, (j+2)%3)*get_at((i+2)%3, (j+1)%3);
                    cofactors.set_at(j, i, value); // already transposed
                }
            }
            return cofactors /= det;
        }

        Real Matrix::squared_norm() const
        {
            Real result = 0;
            Real value;
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                for(int j = 0; j < VECTOR_SIZE; ++j)
                {
                    value = get_at(i,j);
                    result += value*value;
                }
            }
            return result;
        }

        void Matrix::do_jacobi_rotation(int p, int q, /*out*/ Matrix & current_transformation)
        {
            check_index(p);
            check_index(q);
            
            // check for invalid rotation
            if(p == q)
            {
                Logger::warning("incorrect jacobi rotation: `p` and `q` must be different indices", __FILE__, __LINE__);
                return;
            }
            
            // if element is already zeroed
            if(0 == get_at(p,q))
                return;

            // r is the remaining index: not p and not q
            int r = 3 - p - q;

            // theta is cotangent of Real rotation angle
            Real theta = (get_at(q,q) - get_at(p,p))/get_at(p,q)/2;
            // t is sin/cos of rotation angle
            // it is determined from equation t^2 + 2*t*theta - 1 = 0,
            // which implies from definition of theta as (cos^2 - sin^2)/(2*sin*cos)
            Real t = (0 != theta) ? sign(theta)/(abs(theta) + sqrt(theta*theta + 1)) : 1;
            Real cosine = 1/sqrt(t*t + 1);
            Real sine = t*cosine;
            // tau is tangent of half of rotation angle
            Real tau = sine/(1 + cosine);

            add_at(p, p, - t*get_at(p,q));
            add_at(q, q, + t*get_at(p,q));
            
            // correction to element at (r,p)
            Real rp_correction = - sine*(get_at(r,q) + tau*get_at(r,p));
            // correction to element at (r,q)
            Real rq_correction = + sine*(get_at(r,p) - tau*get_at(r,q));
            add_at(r, p, rp_correction);
            add_at(p, r, rp_correction);
            add_at(r, q, rq_correction);
            add_at(q, r, rq_correction);
            set_at(p, q, 0);
            set_at(q, p, 0);

            // construct matrix of applied jacobi rotation
            Matrix rotation = Matrix::IDENTITY;
            rotation.set_at(p, p, cosine);
            rotation.set_at(q, q, cosine);
            rotation.set_at(p, q, sine);
            rotation.set_at(q, p, - sine);
            current_transformation = current_transformation*rotation;
        }

        Matrix & Matrix::diagonalize(int rotations_count, /*out*/ Matrix & transformation)
        {
            transformation = Matrix::IDENTITY;
            for(int i = 0; i < rotations_count; ++i)
                do_jacobi_rotation((i+1)%3, (i+2)%3, transformation);
            return *this;
        }

        Matrix Matrix::compute_function(Function function, int diagonalization_rotations_count) const
        {
            if(NULL == function)
            {
                Logger::error("null pointer `function` in Matrix::compute_function", __FILE__, __LINE__);
                return Matrix::ZERO;
            }
            
            Matrix transformation;
            Matrix diagonalized = this->diagonalized(diagonalization_rotations_count, transformation);
            
            Real value;
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                value = function( diagonalized.get_at(i, i) );
                diagonalized.set_at(i, i, value);
            }

            return transformation*diagonalized*transformation.transposed();
        }

        namespace
        {
            Real cautious_sqrt(Real value)
            {
                return value < 0 ? 0 : sqrt(value);
            }
        }

        void Matrix::do_polar_decomposition(/*out*/ Matrix &orthogonal_part,
                                            /*out*/ Matrix &symmetric_part,
                                            int diagonalization_rotations_count) const
        {
            symmetric_part = (this->transposed()*(*this)).compute_function(cautious_sqrt, diagonalization_rotations_count);
            // FIXME: what if symmetric_part.determinant() == 0?
            if( ! symmetric_part.is_invertible() )
            {
                Logger::error("in Matrix::do_polar_decomposition: symmetric_part is singular, cannot invert", __FILE__, __LINE__);
                return;
            }

            orthogonal_part = (*this)*symmetric_part.inverted();
        }

        // -- identity matrix --

        namespace 
        {
            const Real IDENTITY_VALUES[MATRIX_ELEMENTS_NUM] =
                { 1, 0, 0,
                  0, 1, 0,
                  0, 0, 1 };
            const Real ZERO_VALUES[MATRIX_ELEMENTS_NUM] =
                { 0, 0, 0,
                  0, 0, 0,
                  0, 0, 0 };
        }
        const Matrix Matrix::IDENTITY(IDENTITY_VALUES);
        const Matrix Matrix::ZERO(ZERO_VALUES);
    };
};
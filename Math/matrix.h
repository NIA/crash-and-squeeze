#pragma once
#include "Logging/logger.h"
#include "Math/vector.h"
#include <cmath>

namespace CrashAndSqueeze
{
    namespace Math
    {
        const int MATRIX_ELEMENTS_NUM = VECTOR_SIZE*VECTOR_SIZE;
        
        typedef Real (*Operation)(Real self_element, Real another_element);
        typedef Real (*Function)(Real value);
        
        const int DEFAULT_JACOBI_ROTATIONS_COUNT = 6;

        class Matrix
        {
        private:
            // matrix represented in memory in row-major order ("C-like arrays")
            Real values[MATRIX_ELEMENTS_NUM];
            
            bool check_index(int index) const
            {
                if(index < 0 || index >= VECTOR_SIZE)
                {
                    Logging::Logger::error("Matrix row/column index out of range", __FILE__, __LINE__);
                    return false;
                }
                return true;
            }

            int element_index(int row, int column) const
            {
            #ifndef NDEBUG
                if(false == check_index(row) || false == check_index(column))
                    return 0;
                else
            #endif //ifndef NDEBUG
                    return VECTOR_SIZE*row + column;
            }

        public:
            // -- constructors --

            Matrix() {}
            
            Matrix(const Real values[MATRIX_ELEMENTS_NUM]);

            Matrix(Real x00, Real x01, Real x02,
                   Real x10, Real x11, Real x12,
                   Real x20, Real x21, Real x22);
            
            // construct matrix as outer product of two vectors: matrix product of left_vector to transposed right_vector
            Matrix(const Vector &left_vector, const Vector &right_vector);

            void set_all(Real value);

            static const Matrix IDENTITY;
            static const Matrix ZERO;

            // -- getters/setters --

            // indices in matrix: 0,0 to 2,2
            Real Matrix::get_at(int row, int column) const
            {
                return values[ element_index(row, column) ];
            }

            // index: 0 to 8
            Real Matrix::get_at_index(int index) const;

            // index: 0 to 8
            void Matrix::set_at_index(int index, Real value);
            
            // indices in matrix: 0,0 to 2,2
            void Matrix::set_at(int row, int column, Real value)
            {
                values[ element_index(row, column) ] = value;
            }

            // indices in matrix: 0,0 to 2,2
            void Matrix::add_at(int row, int column, Real value)
            {
                values[ element_index(row, column) ] += value;
            }

            Vector get_row(int row) const;
            Vector get_column(int column) const;

            // -- assignment operators --

            Matrix & operator+=(const Matrix &another);
            Matrix & operator-=(const Matrix &another);
            Matrix & operator*=(const Real &scalar);
            Matrix & operator/=(const Real &scalar);

            // -- binary arithmetic operators --

            Matrix operator+(const Matrix &another) const
            {
                Matrix result = *this;
                return result += another;
            }
            Matrix operator-(const Matrix &another) const
            {
                Matrix result = *this;
                return result -= another;
            }

            Matrix operator*(const Real &scalar) const
            {
                Matrix result = *this;
                return result *= scalar;
            }
            Matrix operator/(const Real &scalar) const
            {
                Matrix result = *this;
                return result /= scalar;
            }

            bool operator==(const Matrix &another) const;
            bool operator!=(const Matrix &another) const
            {
                return !( *this == another );
            }

            
            // -- unary operators --

            Matrix operator-() const
            {
                return (*this)*(-1);
            }
            Matrix operator+() const
            {
                return *this;
            }

            
            // -- multiplications --
            
            Matrix operator*(const Matrix &another) const;
            Vector operator*(const Vector &vector) const;

            // -- methods --

            // transposes matrix in place (!), returns itself
            Matrix & transpose();
            
            // returns transposed matrix
            Matrix transposed() const
            {
                Matrix matrix = *this;
                return matrix.transpose();
            }

            Real determinant() const
            {
                return get_at(0,0)*get_at(1,1)*get_at(2,2) - get_at(0,0)*get_at(1,2)*get_at(2,1)
                     - get_at(0,1)*get_at(1,0)*get_at(2,2) + get_at(0,1)*get_at(1,2)*get_at(2,0)
                     + get_at(0,2)*get_at(1,0)*get_at(2,1) - get_at(0,2)*get_at(1,1)*get_at(2,0);
            }
            
            bool is_invertible() const
            {
                return ! equal(0, determinant());
            }

            // Returns inverted matrixs. Reports error if determinant == 0
            Matrix inverted() const;

            // Inverts matrix (in place). Returns false if determinant == 0 and does not report error
            bool invert();

            // Inverts symmetric (!) matrix in place (!) by diagonalizing it using diag_rotations Jacobi rotations
            bool invert_sym(int diag_rotations = 2*DEFAULT_JACOBI_ROTATIONS_COUNT);

            // squared Frobenius norm of matrix
            Real squared_norm() const;
            
            // Frobenius norm of matrix
            Real norm() const
            {
                return sqrt( squared_norm() );
            }

            // Does Jacobi rotation to nullify A(p, q).
            // Modifies matrix in place (!), multiplies given matrix
            // current_transformation by rotation matrix in place (!).
            // The matrix is assumed to be symmetric (!), and this is NOT checked.
            void do_jacobi_rotation(int p, int q, /*out*/ Matrix & current_transformation);
            
            // Diagonalizes matrix in place(!) using rotations_count Jacobi rotations.
            // Writes diagonalizing transformation matrix into transformation
            // (transformation matrix V is such that D = V.transposed()*A*V,
            //  where A is given matrix and D is diagonalized result).
            // Returns itself.
            // The matrix is assumed to be symmetric (!), and this is NOT checked.
            Matrix & diagonalize(int rotations_count, /*out*/ Matrix & transformation);
            
            // Returns matrix, diagonalized using rotations_count Jacobi rotations.
            // Writes diagonalizing transformation matrix into transformation
            // (transformation matrix V is such that D = V.transposed()*A*V,
            //  where A is given matrix and D is diagonalized result).
            // The matrix is assumed to be symmetric (!), and this is NOT checked.
            Matrix diagonalized(int rotations_count, /*out*/ Matrix & transformation) const
            {
                Matrix result = *this;
                return result.diagonalize(rotations_count, transformation);
            }

            // Computes scalar functions of matrix by diagonalizing it using
            // diagonalization_rotations_count Jacobi rotations and computing
            // function of each diagonal element, and than transforming
            // ("un-diagonalizing") it back.
            // The matrix is assumed to be symmetric (!), and this is NOT checked.
            Matrix compute_function(Function function,
                                    int diagonalization_rotations_count = DEFAULT_JACOBI_ROTATIONS_COUNT) const;

            // Does polar decomposition of matrix, writing orthogonal term
            // ("rotation part") into orthogonal_part, and symmetric term
            // ("scale part") into symmetric_part
            void do_polar_decomposition(/*out*/ Matrix &orthogonal_part,
                                        /*out*/ Matrix &symmetric_part,
                                        int diagonalization_rotations_count = DEFAULT_JACOBI_ROTATIONS_COUNT) const;
        };

        inline Matrix operator*(const Real &scalar, const Matrix &matrix)
        {
            return matrix*scalar;
        }
    };
};

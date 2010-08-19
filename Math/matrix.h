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
            
            Matrix & assigment_operation(const Matrix &another, Operation operation);
            Matrix & assigment_operation(const Real &scalar, Operation operation);

            bool check_index(int index) const
            {
                if(index < 0 || index >= VECTOR_SIZE)
                {
                    logger.error("Matrix index out of range", __FILE__, __LINE__);
                    return false;
                }
                return true;
            }

            int element_index(int line, int column) const
            {
                if(check_index(line) || check_index(column))
                    return VECTOR_SIZE*line + column;
                else
                    return 0;
            }

        public:
            // -- constructors --

            Matrix();
            
            Matrix(const Real values[MATRIX_ELEMENTS_NUM]);
            
            // construct matrix as multiplication of left_vector to transposed right_vector
            Matrix(const Vector &left_vector, const Vector &right_vector);

            static const Matrix IDENTITY;

            // -- getters/setters --

            // indices in matrix: 0,0 to 2,2
            Real Matrix::get_at(int line, int column) const
            {
                return values[ element_index(line, column) ];
            }
            
            // indices in matrix: 0,0 to 2,2
            void Matrix::set_at(int line, int column, Real value)
            {
                values[ element_index(line, column) ] = value;
            }

            // indices in matrix: 0,0 to 2,2
            void Matrix::add_at(int line, int column, Real value)
            {
                values[ element_index(line, column) ] += value;
            }


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
            
            Matrix inverted() const;

            // squared Frobenius norm of matrix
            Real squared_norm() const;
            
            // Frobenius norm of matrix
            Real norm() const
            {
                return sqrt( squared_norm() );
            }

            // Does Jacobi rotation for p and q rows and columns.
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

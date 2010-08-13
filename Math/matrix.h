#pragma once

#include "vector.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        const int MATRIX_ELEMENTS_NUM = VECTOR_SIZE*VECTOR_SIZE;
        typedef double (*Operation)(double self_element, double another_element);

        class Matrix
        {
        private:
            // matrix represented in memory in row-major order ("C-like arrays")
            double values[MATRIX_ELEMENTS_NUM];
            
            Matrix & assigment_operation(const Matrix &another, Operation operation);
            Matrix & assigment_operation(const double &scalar, Operation operation);

        public:
            // -- constructors --

            Matrix();
            Matrix(const double *values);
            // construct matrix as multiplication of left_vector to transposed right_vector
            Matrix(const Vector &left_vector, const Vector &right_vector);

            // -- getters/setters --

            // indices in matrix: 0,0 to 2,2
            static int _element_index(int line, int column)
            {
                assert(line >= 0);
                assert(line < VECTOR_SIZE);
                assert(column >= 0);
                assert(column < VECTOR_SIZE);
                return VECTOR_SIZE*line + column;
            }

            double Matrix::get_at(int line, int column) const
            {
                return values[ _element_index(line, column) ];
            }
            
            void Matrix::set_at(int line, int column, double value)
            {
                values[ _element_index(line, column) ] = value;
            }

            // -- assignment operators --

            Matrix & operator+=(const Matrix &another);
            Matrix & operator-=(const Matrix &another);
            Matrix & operator*=(const double &scalar);
            Matrix & operator/=(const double &scalar);

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

            Matrix operator*(const double &scalar) const
            {
                Matrix result = *this;
                return result *= scalar;
            }
            Matrix operator/(const double &scalar) const
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

            Matrix operator-() const;
            Matrix operator+() const;

            // -- multiplications --
            
            Matrix operator*(const Matrix &another);
            Vector operator*(const Vector &vector);

            // -- methods --

            Matrix transpose();
            Matrix transposed();

            Matrix inverted();

            // squared Frobenius norm of matrix
            Matrix squared_norm();
            // Frobenius norm of matrix
            Matrix norm();
        };
    };
};

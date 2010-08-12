#pragma once

#include "vector.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        const int MATRIX_SIZE = 3;
        const int MATRIX_ELEMENTS_NUM = MATRIX_SIZE*MATRIX_SIZE;

        class Matrix
        {
        private:
            // matrix represented in memory as concatenated lines
            double values[MATRIX_ELEMENTS_NUM];

        public:
            Matrix();
            Matrix(const double *values);

            // indices in matrix: 1,1 to 3,3
            double get_at(int line, int column) const;
            void set_at(int line, int column, double value);

            // unary operators
            Matrix operator-() const;
            Matrix operator+() const;

            // assignment operators
            Matrix & operator+=(const Matrix &another);
            Matrix & operator-=(const Matrix &another);
            Matrix & operator*=(const double &scalar);
            Matrix & operator/=(const double &scalar);

            // binary operators
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

            Matrix operator*(const Matrix &another);
            Vector operator*(const Vector &vector);

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

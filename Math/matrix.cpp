#include "assert.h"
#include "matrix.h"
#include <cstring>

namespace CrashAndSqueeze
{
    namespace Math
    {
        // -- constructors --

        Matrix::Matrix()
        {
            for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
                values[i] = 0;
        }

        Matrix::Matrix(const double *values)
        {
            memcpy(this->values, values, sizeof(this->values));
        }

        Matrix::Matrix(const Vector &left_vector, const Vector &right_vector)
        {
            for(int i = 0; i < VECTOR_SIZE; ++i)
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    set_at(i, j, left_vector[i]*right_vector[j]);
        }
        
        // -- assignment operators --

        Matrix & Matrix::assigment_operation(const Matrix &another, Operation operation)
        {
            assert(operation != NULL);
            double result;
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                for(int j = 0; j < VECTOR_SIZE; ++j)
                {
                    result = operation( get_at(i, j), another.get_at(i, j) );
                    set_at(i, j, result);
                }
            }
                    
            return *this;
        }

        Matrix & Matrix::assigment_operation(const double &scalar, Operation operation)
        {
            assert(operation != NULL);
            double result;
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                for(int j = 0; j < VECTOR_SIZE; ++j)
                {
                    result = operation( get_at(i, j), scalar );
                    set_at(i, j, result);
                }
            }
                    
            return *this;
        }

        double add(double a, double b) { return a + b; }
        double sub(double a, double b) { return a - b; }
        double mul(double a, double b) { return a * b; }
        double div(double a, double b) { return a / b; }

        Matrix & Matrix::operator +=(const Matrix &another)
        {
            return assigment_operation(another, add);
        }
        
        Matrix & Matrix::operator -=(const Matrix &another)
        {
            return assigment_operation(another, sub);
        }
        
        Matrix & Matrix::operator *=(const double &scalar)
        {
            return assigment_operation(scalar, mul);
        }
        
        Matrix & Matrix::operator /=(const double &scalar)
        {
            return assigment_operation(scalar, div);
        }

        // -- binary arithmetic operators --

        bool Matrix::operator==(const Matrix &another) const
        {
            for(int i = 0; i < VECTOR_SIZE; ++i)
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    if( get_at(i, j) != another.get_at(i, j) )
                        return false;
            return true;
        }
        // -- multiplications --
        
        Matrix Matrix::operator*(const Matrix &another)
        {
            Matrix result;
            double value;

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
        
        Vector Matrix::operator*(const Vector &vector)
        {
            Vector result;
            double value;
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
        
        // -- identity matrix --

        const double IDENTITY_VALUES[MATRIX_ELEMENTS_NUM] =
            { 1, 0, 0,
              0, 1, 0,
              0, 0, 1 };
        const Matrix Matrix::IDENTITY(IDENTITY_VALUES);
    };
};
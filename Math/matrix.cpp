#include "assert.h"
#include "matrix.h"

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
        
        bool Matrix::operator==(const Matrix &another) const
        {
            for(int i = 0; i < VECTOR_SIZE; ++i)
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    if( get_at(i, j) != another.get_at(i, j) )
                        return false;
            return true;
        }

    };
};
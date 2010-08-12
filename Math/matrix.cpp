#include "floating_point.h"
#include "vector.h"
#include "assert.h"
#include "Matrix.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        Matrix::Matrix()
        {
            for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
                values[i] = 0;
        }

        Matrix::Matrix(const double *values)
        {
            memcpy(this->values, values, sizeof(this->values));
        }

        int _element_index(int line, int column)
        {
            assert(line > 0);
            assert(line <= MATRIX_SIZE);
            assert(column > 0);
            assert(column <= MATRIX_SIZE);
            return MATRIX_SIZE*(line - 1) + column - 1;
        }

        double Matrix::get_at(int line, int column) const
        {
            return values[ _element_index(line, column) ];
        }
        
        void Matrix::set_at(int line, int column, double value)
        {
            values[ _element_index(line, column) ] = value;
        }
    };
};
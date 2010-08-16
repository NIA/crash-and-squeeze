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
                    if( ! equal( get_at(i, j), another.get_at(i, j) ) )
                        return false;
            return true;
        }
        // -- multiplications --
        
        Matrix Matrix::operator*(const Matrix &another) const
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
        
        Vector Matrix::operator*(const Vector &vector) const
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

        inline void swap(double &a, double &b)
        {
            double temp;
            temp = a;
            a = b;
            b = temp;
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
            double det = determinant();
            assert(det != 0); //TODO: errors
            
            Matrix cofactors;
            double value;
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

        double Matrix::squared_norm() const
        {
            double result = 0;
            double value;
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
                return;
            
            // if element is already zeroed
            if(0 == get_at(p,q))
                return;

            // r is the remaining index: not p and not q
            int r = 3 - p - q;

            // theta is cotangent of double rotation angle
            double theta = (get_at(q,q) - get_at(p,p))/get_at(p,q)/2;
            // t is sin/cos of rotation angle
            // it is determined from equation t^2 + 2*t*theta - 1 = 0,
            // which implies from definition of theta as (cos^2 - sin^2)/(2*sin*cos)
            double t = (0 != theta) ? sign(theta)/(abs(theta) + sqrt(theta*theta + 1)) : 1;
            double cosine = 1/sqrt(t*t + 1);
            double sine = t*cosine;
            // tau is tangent of half of rotation angle
            double tau = sine/(1 + cosine);

            add_at(p, p, - t*get_at(p,q));
            add_at(q, q, + t*get_at(p,q));
            
            // correction to element at (r,p)
            double rp_correction = - sine*(get_at(r,q) + tau*get_at(r,p));
            // correction to element at (r,q)
            double rq_correction = + sine*(get_at(r,p) - tau*get_at(r,q));
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


        // -- identity matrix --

        const double IDENTITY_VALUES[MATRIX_ELEMENTS_NUM] =
            { 1, 0, 0,
              0, 1, 0,
              0, 0, 1 };
        const Matrix Matrix::IDENTITY(IDENTITY_VALUES);
    };
};
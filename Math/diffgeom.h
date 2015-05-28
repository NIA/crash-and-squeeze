
#pragma  once
#include "Math/matrix.h"

// This file contains different entities related to differential geometry
// (Riemannian or non-Riemannian) that can be used in addition to usual
// Math::Vector and Math::matrix

namespace CrashAndSqueeze
{
    namespace Math
    {
        // Coefficients of affine connection $\Gamma_{ij}^k$ at some given point: N^3 numbers
        class ConnectionCoeffs
        {
        private:
            // `i` and `j` indices are mapped to rows and columns of matrix, while `k` enumerates items in this array
            Matrix coeff_mxs[VECTOR_SIZE];

            bool check_index(int index) const
            {
                if(index < 0 || index >= VECTOR_SIZE)
                {
                    Logging::Logger::error("Coefficients of  connection index out of range", __FILE__, __LINE__);
                    return false;
                }
                return true;
            }
        public:
            // TODO: add some prefix/postfix for i/j/k arguments to differentiate from `up` and `down` indices? Like i_d, k_u?

            // Return $\Gamma_{ij}^k$ coordinate: `i` and `j` are down indices, `k` is up index 
            Real get_at(int i, int j, int k) const
            {
#ifndef NDEBUG
                if(false == check_index(i) || false == check_index(j) || false == check_index(k))
                    return 0;
                else
#endif //ifndef NDEBUG
                    return coeff_mxs[k].get_at(i,j);
            }

            // Set $\Gamma_{ij}^k$ coordinate: `i` and `j` are down indices, `k` is up index 
            void set_at(int i, int j, int k, Real value)
            {
#ifndef NDEBUG
                if(false == check_index(i) || false == check_index(j) || false == check_index(k))
                    return;
                else
#endif //ifndef NDEBUG
                    coeff_mxs[k].set_at(i,j, value);
            }

            void set_all(Real value)
            {
                for (int k = 0; k < VECTOR_SIZE; ++k)
                    coeff_mxs[k].set_all(value);
            }

            // Returns differential of vector `v` coordinates when moving from `x` (point at which current coeffs are given)
            // to `x` +`dx`: $d v^k = - \Gamma_{ij}^k v^j dx^i$
            Vector d_parallel_transport(const Vector &v, const Vector &dx) const
            {
                Vector res = Vector::ZERO;
                for (int k = 0; k < VECTOR_SIZE; ++k)
                    for (int i = 0; i < VECTOR_SIZE; ++i)
                        for (int j = 0; j < VECTOR_SIZE; ++j)
                            res[k] += - get_at(i, j, k) * v[j] * dx[i];
                return res;
            }

            // Returns addition to partial derivative of vector field that makes it a covariant derivative:
            // $\nabla_k v^i = \frac{\partial v^i}{\partial x^k} + \Gamma_{kp}^i v^p$
            //                                                     ^^^^^^^^^^^^^^^^^
            Vector covariant_addition(const Vector &v, int k) const
            {
                Vector res = Vector::ZERO;
                for (int i = 0; i < VECTOR_SIZE; ++i)
                    for (int p = 0; p < VECTOR_SIZE; ++p)
                        res[i] += get_at(k, p, i) * v[p];
                return res;
            }
        };

        // Interface of connection object: the field of $\Gamma_{ij}^k$
        // Any actual connection object should be made as class implementing this interface
        class IConnectionObject
        {
        public:
            // returns connection coeffs $\Gamma_{ij}^k$ at point `point` by reference via `coeffs` reference
            virtual void get_at(/*in*/ const Vector & point, /*out*/ ConnectionCoeffs & coeffs) const = 0;
        };

        // Metric tensor at given point: N^2 coordinates. (Can be implemented just as matrix)
        typedef Matrix MetricTensor;

        // Interface of metric: the field of metric tensor $g_{ij}$
        // Any actual metric should be made as class implementing this interface
        class IMetric
        {
        public:
            // returns metric tensor $g_{ij}$ at point `point` by reference via `metric_tensor` reference
           virtual void get_at(/*in*/ const Vector & point, /*out*/ MetricTensor & metric_tensor) const = 0;
        };

    }
}

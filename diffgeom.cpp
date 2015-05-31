#define _USE_MATH_DEFINES
#include "Math/diffgeom.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    namespace Math
    {
        ConnectionCoeffs::ConnectionCoeffs(Real all_components)
        {
            set_all(all_components);
        }

        CrashAndSqueeze::Math::Real ConnectionCoeffs::get_at(int i, int j, int k) const
        {
#ifndef NDEBUG
            if(false == check_index(i) || false == check_index(j) || false == check_index(k))
                return 0;
            else
#endif //ifndef NDEBUG
                return coeff_mxs[k].get_at(i,j);
        }

        void ConnectionCoeffs::set_at(int i, int j, int k, Real value)
        {
#ifndef NDEBUG
            if(false == check_index(i) || false == check_index(j) || false == check_index(k))
                return;
            else
#endif //ifndef NDEBUG
                coeff_mxs[k].set_at(i,j, value);
        }

        void ConnectionCoeffs::set_all(Real value)
        {
            for (int k = 0; k < VECTOR_SIZE; ++k)
                coeff_mxs[k].set_all(value);
        }

        CrashAndSqueeze::Math::Vector ConnectionCoeffs::d_parallel_transport(const Vector &v, const Vector &dx) const
        {
            Vector dv = Vector::ZERO;
            // $d v^k = - \Gamma_{ij}^k v^j dx^i$
            for (int k = 0; k < VECTOR_SIZE; ++k)
                for (int i = 0; i < VECTOR_SIZE; ++i)
                    for (int j = 0; j < VECTOR_SIZE; ++j)
                        dv[k] += - get_at(i, j, k) * v[j] * dx[i];
            return dv;
        }

        CrashAndSqueeze::Math::Vector ConnectionCoeffs::covariant_addition(const Vector &v, int k) const
        {
            Vector res = Vector::ZERO;
            // $\nabla_k v^i = \frac{\partial v^i}{\partial x^k} + \Gamma_{kp}^i v^p$
            //                                                     ^^^^^^^^^^^^^^^^^
            for (int i = 0; i < VECTOR_SIZE; ++i)
                for (int p = 0; p < VECTOR_SIZE; ++p)
                    res[i] += get_at(k, p, i) * v[p];
            return res;
        }

        bool ConnectionCoeffs::check_index(int index) const
        {
            if(index < 0 || index >= VECTOR_SIZE)
            {
                Logger::error("Coefficients of  connection index out of range", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        // -- Implementations: for some specific spaces --

        namespace
        {
            inline Real cotan(Real x)
            {
                return tan(M_PI_2 - x);
            }
            inline Real sqr(Real x)
            {
                return x*x;
            }

}

        Real d_line_length(const MetricTensor & metric, const Vector &dv)
        {
            // $ ds^2 = g_{ij} dx^i dx^j $
            Real ds_squared = (metric * dv) * dv;
            if (ds_squared < 0)
            {
                Logger::error("In MetricTensor::d_line_length: ds^2 < 0 - Pseudo-Riemannian metric is not supported", __FILE__, __LINE__);
                return 0;
            }
            return sqrt(ds_squared);
        }

        // -- Implementations: for some specific spaces --

        void Euclidean::Connection::value_at(const Vector & point, ConnectionCoeffs & coeffs) const 
        {
            coeffs.set_all(0);
        }
        
        void Euclidean::Metric::value_at(const Vector & point, MetricTensor & metric) const 
        {
            metric = MetricTensor::IDENTITY;
        }

        void SphericalCoords::Connection::value_at(const Vector & point, ConnectionCoeffs & coeffs) const 
        {
            Real rho = point[0], theta = point[1], phi = point[2];

            if ( equal(0, rho) || equal(0, sin(theta)) )
            {
                Logger::error("In SphericalCoords::Connection::value_at: value is not defined at rho=0 or theta = pi*n");
                return;
            }

            // Values as from http://blog.sciencenet.cn/home.php?mod=attachment&filename=ConnectionExample.nb.pdf&id=48629
            coeffs.set_all(0);
            coeffs.set_at_sym(RHO,  THETA,THETA,  1/rho);
            coeffs.set_at_sym(RHO,  PHI,  PHI,    1/rho);
            coeffs.set_at_sym(THETA,PHI,  PHI,    cotan(theta));
            coeffs.set_at    (THETA,THETA,RHO,    -rho);
            coeffs.set_at    (PHI,  PHI,  RHO,    -rho*sqr(sin(theta)));
            coeffs.set_at    (PHI,  PHI,  THETA,  -cos(theta)*sin(theta));
        }

        void SphericalCoords::Metric::value_at(const Vector & point, MetricTensor & metric) const 
        {
            Real rho = point[0], theta = point[1], phi = point[2];
            metric.set_all(0);
            metric.set_at(RHO,  RHO,   1);
            metric.set_at(THETA,THETA, sqr(rho));
            metric.set_at(PHI,  PHI,   sqr(rho*sin(theta)));
        }

    }
}
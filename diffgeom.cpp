#define _USE_MATH_DEFINES
#include "Math/diffgeom.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    namespace Math
    {
        const Real ICurve::T_START = 0.0;
        const Real ICurve::T_END   = 1.0;

        ConnectionCoeffs::ConnectionCoeffs(Real all_components)
        {
            set_all(all_components);
        }

        Real ConnectionCoeffs::get_at(int i, int j, int k) const
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

        Vector ConnectionCoeffs::d_parallel_transport(const Vector &v, const Vector &dx) const
        {
            Vector dv = Vector::ZERO;
            // $d v^k = - \Gamma_{ij}^k v^j dx^i$
            for (int k = 0; k < VECTOR_SIZE; ++k)
                for (int i = 0; i < VECTOR_SIZE; ++i)
                    for (int j = 0; j < VECTOR_SIZE; ++j)
                        dv[k] += - get_at(i, j, k) * v[j] * dx[i];
            return dv;
        }

        Vector ConnectionCoeffs::covariant_addition(const Vector &v, int k) const
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

        bool CurvatureTensor::check_index(int index) const
        {
            if(index < 0 || index >= VECTOR_SIZE)
            {
                Logger::error("Curvature tensor index out of range", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        Real CurvatureTensor::get_at(int i, int j, int k, int m) const
        {
#ifndef NDEBUG
            if (false == check_index(i) || false == check_index(j) || false == check_index(k) || false == check_index(m) )
                return;
            else
#endif //ifndef NDEBUG
                return coeffs_mxs[i][j].get_at(k, m);
        }

        void CurvatureTensor::set_at(int i, int j, int k, int m, Real value)
        {
#ifndef NDEBUG
            if (false == check_index(i) || false == check_index(j) || false == check_index(k) || false == check_index(m) )
                return;
            else
#endif //ifndef NDEBUG
                coeffs_mxs[i][j].set_at(k, m, value);
        }

        void CurvatureTensor::set_all(Real value)
        {
            for (int k = 0; k < VECTOR_SIZE; ++k)
                for (int m = 0; m < VECTOR_SIZE; ++m)
                    coeffs_mxs[k][m].set_all(value);
        }

        void CurvatureTensor::lower_index(const MetricTensor &g, CurvatureTensor &R_out)
        {
            R_out.set_all(0);
            for (int i = 0; i < VECTOR_SIZE; ++i)
                for (int j = 0; j < VECTOR_SIZE; ++j)
                    for (int k = 0; k < VECTOR_SIZE; ++k)
                        for (int m = 0; m < VECTOR_SIZE; ++m)
                            for (int q = 0; q < VECTOR_SIZE; ++q)
                                // R_ijkm += R^q_jkm * g_qi
                                R_out.set_at(i,j,k,m, R_out.get_at(i,j,k,m) + get_at(q,j,k,m)*g.get_at(q, i));
        }

        CrashAndSqueeze::Math::Vector CurvatureTensor::d_parallel_transport(const Vector &v, const Vector &dx1, const Vector &dx2) const
        {
            Vector res = Vector::ZERO;
            for (int i = 0; i < VECTOR_SIZE; ++i)
                for (int k = 0; k < VECTOR_SIZE; ++k)
                    for (int m = 0; m < VECTOR_SIZE; ++m)
                        for (int j = 0; j < VECTOR_SIZE; ++j)
                            res[i] += get_at(i, m, k, j) * v[j] * (dx1[m]*dx2[k] - dx1[k]*dx2[m]) / 2;
            return res;
        }

        const NoCurvature NoCurvature::instance;
        void NoCurvature::value_at(const Vector & /*point*/, CurvatureTensor & coords) const 
        {
            coords.set_all(0);
        }

        // -- Implementations: for some specific spaces --

        // TODO: there is VERY much copy-paste...

        void Euclidean::Connection::value_at(const Point & /*point*/, ConnectionCoeffs & coeffs) const 
        {
            coeffs.set_all(0);
        }
        
        void Euclidean::Metric::value_at(const Point & /*point*/, MetricTensor & metric) const 
        {
            metric = MetricTensor::IDENTITY;
        }

        const Euclidean::Connection Euclidean::connection;
        const IConnection * Euclidean::get_connection() const 
        {
            return &connection;
        }

        
        bool Euclidean::has_metric() const 
        {
            return true;
        }

        const Euclidean::Metric Euclidean::metric;
        const IMetric * Euclidean::get_metric() const 
        {
            return &metric;
        }

        const ICurvature * Euclidean::get_curvature() const 
        {
            return &NoCurvature::instance;
        }

        Point Euclidean::point_to_cartesian(const Point & point) const 
        {
            throw point;
        }

        Vector Euclidean::vector_to_cartesian(const Vector & vector, const Point & /*point*/) const 
        {
            throw vector;
        }

        const Real SphericalCoords::PI = M_PI;

        void SphericalCoords::Connection::value_at(const Point & point, ConnectionCoeffs & coeffs) const 
        {
            Real rho = point[RHO], theta = point[THETA]/*, phi = point[PHI]*/;

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

        void SphericalCoords::Metric::value_at(const Point & point, MetricTensor & metric) const 
        {
            Real rho = point[RHO], theta = point[THETA]/*, phi = point[PHI]*/;
            metric.set_all(0);
            metric.set_at(RHO,  RHO,   1);
            metric.set_at(THETA,THETA, sqr(rho));
            metric.set_at(PHI,  PHI,   sqr(rho*sin(theta)));
        }

        const SphericalCoords::Connection SphericalCoords::connection;
        const IConnection * SphericalCoords::get_connection() const 
        {
            return &connection;
        }

        bool SphericalCoords::has_metric() const 
        {
            return true;
        }

        const SphericalCoords::Metric SphericalCoords::metric;
        const IMetric * SphericalCoords::get_metric() const 
        {
            return &metric;
        }

        const ICurvature * SphericalCoords::get_curvature() const 
        {
            return &NoCurvature::instance;
        }

        Point SphericalCoords::point_to_cartesian(const Point & point) const 
        {
            Real rho = point[RHO], theta = point[THETA], phi = point[PHI];
            return Vector(rho*sin(theta)*cos(phi), rho*sin(theta)*sin(phi), rho*cos(theta));
        }

        Vector SphericalCoords::vector_to_cartesian(const Vector & vector, const Point & at_point) const 
        {
            Real rho = at_point[RHO], theta = at_point[THETA], phi = at_point[PHI];

            // Cartesian coordinates of spherical basis vectors
            Vector e_rho(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
            Vector e_theta(cos(theta)*cos(phi), cos(theta)*sin(phi), -sin(theta));
            Vector e_phi(-sin(phi), cos(phi), 0);

            // Now make the linear combination (TODO: write this as matrix multiplication?)
            return e_rho*vector[RHO] + e_theta*vector[THETA] + e_phi*vector[PHI];
        }

        void UnitSphere2D::Connection::value_at(const Point & point, ConnectionCoeffs & coeffs) const 
        {
            Real theta = point[THETA]/*, phi = point[PHI]*/;
            if ( equal(0, sin(theta)) )
            {
                Logger::error("In UnitSphere2D::Connection::value_at: value is not defined at theta = pi*n");
                return;
            }
            // Same values as in SphericalCoords::Connection, but without RHO components
            coeffs.set_all(0);
            coeffs.set_at_sym(THETA,PHI,  PHI,    cotan(theta));
            coeffs.set_at    (PHI,  PHI,  THETA,  -cos(theta)*sin(theta));
        }

        void UnitSphere2D::Metric::value_at(const Point & point, MetricTensor & metric) const 
        {
            Real theta = point[THETA]/*, phi = point[PHI]*/;
            metric.set_all(0);
            metric.set_at(THETA,THETA, 1);
            metric.set_at(PHI,  PHI,   sqr(sin(theta)));
        }


        void UnitSphere2D::Curvature::value_at(const Vector & point, CurvatureTensor & coords) const 
        {
            coords.set_all(0);
            Matrix delta = Matrix::IDENTITY; // `delta` = coordinates of Kronecker delta
            MetricTensor g;
            metric.value_at(point, g);       // `g` = coordinates of metric tensor at that point
            // Values as of http://www.physics.usu.edu/Wheeler/GenRel/Lectures/2Sphere.pdf
            for (int i = 0; i < VECTOR_SIZE; ++i)
                for (int j = 0; j < VECTOR_SIZE; ++j)
                    for (int k = 0; k < VECTOR_SIZE; ++k)
                        for (int m = 0; m < VECTOR_SIZE; ++m)
                            coords.set_at(i, j, k, m, delta.get_at(i,k)*g.get_at(j,m) - delta.get_at(i, m)*g.get_at(j,k));
        }
        
        const UnitSphere2D::Connection UnitSphere2D::connection;
        const IConnection * UnitSphere2D::get_connection() const 
        {
            return & connection;
        }

        bool UnitSphere2D::has_metric() const 
        {
            return true;
        }

        const UnitSphere2D::Metric UnitSphere2D::metric;
        const IMetric * UnitSphere2D::get_metric() const 
        {
            return & metric;
        }


        const UnitSphere2D::Curvature UnitSphere2D::curvature;
        const ICurvature * UnitSphere2D::get_curvature() const 
        {
            return &curvature;
        }

        Point UnitSphere2D::point_to_cartesian(const Point & point) const 
        {
            // Reuse the logic of SphericalCoords for rho = 1
            SphericalCoords spherical;
            return spherical.point_to_cartesian(Point(1, point[THETA], point[PHI]));
        }

        CrashAndSqueeze::Math::Vector UnitSphere2D::vector_to_cartesian(const Vector & vector, const Point & at_point) const 
        {
            // Reuse the logic of SphericalCoords for rho = 1 (point) and rho = 0 (vector)
            SphericalCoords spherical;
            return spherical.vector_to_cartesian(Vector(0, vector[THETA], vector[PHI]), Point(1, at_point[THETA], at_point[PHI]));
        }

    }
}
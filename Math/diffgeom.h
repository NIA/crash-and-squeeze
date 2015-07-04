#pragma  once
#include "Math/matrix.h"

// This file contains different entities related to differential geometry
// (Riemannian or non-Riemannian) that can be used in addition to usual
// Math::Vector and Math::Matrix for curved spaces.

namespace CrashAndSqueeze
{
    namespace Math
    {
        // An interface of curve: returns points of curve for parameter `t` ranging from T_START to T_END
        class ICurve
        {
        public:
            virtual Point point_at(Real t) const = 0;

            static const Real T_START; /* = 0 */
            static const Real T_END;   /* = 1 */
        };

        // An interface of surface: returns points of surface for two parameters `u` and `v`, ranging from U_START to U_END and from T_START to T_END, correspondingly
        // TODO: currently surface is a unit square in (u,v), other parameterizations should also be possible
        class ISurface
        {
        public:
            virtual Point point_at(Real u, Real v) const = 0;
            
            static const Real U_START; /* = 0 */
            static const Real U_END;   /* = 1 */
            static const Real V_START; /* = 0 */
            static const Real V_END;   /* = 1 */
        };

        // Coefficients of affine connection $\Gamma_{ij}^k$ at some given point: N^3 numbers
        class ConnectionCoeffs
        {
        private:
            // `i` and `j` indices are mapped to rows and columns of matrix, while `k` enumerates items in this array
            Matrix coeff_mxs[VECTOR_SIZE];

            bool check_index(int index) const;
        public:
            // TODO: add more useful constructors?

            // Do not initialize ConnectionCoeffs
            ConnectionCoeffs() {}

            // Initialize all components with same value all_components
            ConnectionCoeffs(Real all_components);

            // TODO: add some prefix/postfix for i/j/k arguments to differentiate from `up` and `down` indices? Like i_d, k_u?

            // Return $\Gamma_{ij}^k$ coordinate: `i` and `j` are down indices, `k` is up index 
            Real get_at(int i, int j, int k) const;

            // Set $\Gamma_{ij}^k$ coordinate: `i` and `j` are down indices, `k` is up index 
            void set_at(int i, int j, int k, Real value);

            // Sets both $\Gamma_{ij}^k$ and $\Gamma_{ji}^k$ - useful for torsion-free (symmetric) connection
            void set_at_sym(int i, int j, int k, Real value)
            {
                set_at(i,j,k, value);
                set_at(j,i,k, value);
            }

            void set_all(Real value);

            // Returns differential of vector `v` coordinates when moving from `x` (point at which current coeffs are given)
            // to `x` +`dx`: $d v^k = - \Gamma_{ij}^k v^j dx^i$
            Vector d_parallel_transport(const Vector &v, const Vector &dx) const;

            // Returns addition to partial derivative of vector field that makes it a covariant derivative:
            // $\nabla_k v^i = \frac{\partial v^i}{\partial x^k} + \Gamma_{kp}^i v^p$
            //                                                     ^^^^^^^^^^^^^^^^^
            Vector covariant_addition(const Vector &v, int k) const;
        };

        // A template interface that is basic for IConnectionObject and IMetric
        template <class T>
        class ICoordsAtPoint
        {
        public:
            // returns coordinates (of type T) at point `point` by reference
            virtual void value_at(/*in*/ const Vector & point, /*out*/ T & coords) const = 0;
        };

        // Interface of connection object: the field of $\Gamma_{ij}^k$
        // Any actual connection object should be made as class implementing this interface
        typedef ICoordsAtPoint<ConnectionCoeffs> IConnection;

        // Metric tensor at given point: N^2 coordinates. (Can be implemented just as matrix)
        class MetricTensor : public Matrix
        {
        public:
            // Returns scalar product with respect to metric: $ g_{ij} v^i u^j $
            Real dot_product(const Vector &v, const Vector & u) const;

            // Returns differential of line (arc) length `ds` when coordinates are changed by an infinitesimal `dv`
            // $ ds^2 = g_{ij} dx^i dx^j $
            // TODO: this function should probably be a method of class MetricTensor, but currently it is just a `typedef Matrix` and it seems not reasonable to make it a separate class
            Real norm(const Vector &dv) const;

            // Allow assignment of usual matrix
            MetricTensor & operator=(const Matrix &m)
            {
                Matrix::operator=(m);
                return *this;
            }
        };


        // Interface of metric: the field of metric tensor $g_{ij}$
        // Any actual metric should be made as class implementing this interface
        typedef ICoordsAtPoint<MetricTensor> IMetric;

        // Curvature tensor $R^i_jkm$ at some given point: N^4 numbers
        class CurvatureTensor
        {
        private:
            // `i` and `j` enumerate items in this array, while `k` and `m` indices are mapped to rows and columns of matrix
            Matrix coeffs_mxs[VECTOR_SIZE][VECTOR_SIZE];

            bool check_index(int index) const;
        public:
            
            // Return $R^i_jkm$ coordinate: `i` is up index, `j`, `k`, `m` are down indices
            Real get_at(int i, int j, int k, int m) const;

            // Set $R^i_jkm$ coordinate: `i` is up index, `j`, `k`, `m` are down indices
            void set_at(int i, int j, int k, int m, Real value);

            void set_all(Real value);

            // Perform lowering of index `i` into `R_out`, given the value of metric tensor at this point `g`
            void lower_index(const MetricTensor &g, /*out*/ CurvatureTensor &R_out);

            Vector d_parallel_transport(const Vector &v, const Vector &dx1, const Vector &dx2) const;
        };

        typedef ICoordsAtPoint<CurvatureTensor> ICurvature;

        class NoCurvature : public ICurvature
        {
        public:
            virtual void value_at(const Vector & point, CurvatureTensor & coords) const override;

            static const NoCurvature instance;
        };

        class ISpace
        {
        public:
            virtual const IConnection * get_connection() const = 0;
            
            virtual bool has_metric() const = 0;
            virtual const IMetric * get_metric() const = 0;

            virtual const ICurvature * get_curvature() const = 0;

            // Transform coordinates of point used in this space to Cartesian coordinates
            virtual Point point_to_cartesian(const Point & point) const = 0;
            // Transform coordinates of `vector` in local basis at point `at_point` to Cartesian coordinates
            virtual Vector vector_to_cartesian(const Vector & vector, const Point & at_point) const = 0;
        };

        // -- Implementations: for some specific spaces --

        // Trivial implementations for usual Cartesian coordinates in Euclidean space
        class Euclidean : public ISpace
        {
        public:
            // Trivial (zero) connection of Euclidean space
            class Connection : public IConnection
            {
            public:
                virtual void value_at(const Point & point, /*out*/ ConnectionCoeffs & coeffs) const override;
            };
            static const Connection connection;

            // Trivial (identity) metric of Euclidean space
            class Metric: public IMetric
            {
            public:
                virtual void value_at(const Point & point, /*out*/ MetricTensor & metric) const override;
            };
            static const Metric metric;

            // Implement ISpace:

            virtual const IConnection * get_connection() const override;
            virtual bool has_metric() const override;
            virtual const IMetric * get_metric() const override;
            virtual const ICurvature * get_curvature() const override;
            virtual Point point_to_cartesian(const Point & point) const override;
            virtual Vector vector_to_cartesian(const Vector & vector, const Point & at_point) const override;

        };

        // Implementations for usual spherical coordinates in Euclidean space
        //
        // Components of vector (0, 1, 2) here refer to (rho, theta, phi)
        class SphericalCoords : public ISpace
        {
        public:
            enum CoordName
            {
                RHO   = 0,
                THETA = 1,
                PHI   = 2,
            };
            static const Real PI;

            // Spherical coordinates connection coefficients for normal Euclidean space
            class Connection : public IConnection
            {
            public:
                virtual void value_at(const Point & point, /*out*/ ConnectionCoeffs & coeffs) const override;
            };
            static const Connection connection;

            // Spherical coordinates metric for normal Euclidean space
            class Metric: public IMetric
            {
            public:
                virtual void value_at(const Point & point, /*out*/ MetricTensor & metric) const override;
            };
            static const Metric metric;

            // Implement ISpace:

            virtual const IConnection * get_connection() const override;
            virtual bool has_metric() const override;
            virtual const IMetric * get_metric() const override;
            virtual const ICurvature * get_curvature() const override;
            virtual Point point_to_cartesian(const Point & point) const override;
            virtual Vector vector_to_cartesian(const Vector & vector, const Point & at_point) const override;

        };

        // 2D-Surface of 3D unit sphere in as a Riemannian space
        class UnitSphere2D : public ISpace
        {
        public:
            enum CoordName
            {
                // There is no `rho`! first component of Vector is ignored
                THETA = 1,
                PHI = 2,
            };

            // 2D sphere connection coefficients
            class Connection : public IConnection
            {
            public:
                virtual void value_at(const Point & point, /*out*/ ConnectionCoeffs & coeffs) const override;
            };
            static const Connection connection;

            // 2D sphere metric
            class Metric: public IMetric
            {
            public:
                virtual void value_at(const Point & point, /*out*/ MetricTensor & metric) const override;
            };
            static const Metric metric;

            // 2D sphere curvature tensor
            class Curvature: public ICurvature
            {
            public:
                virtual void value_at(const Vector & point, /*out*/ CurvatureTensor & coords) const override;
            };
            static const Curvature curvature;

            // Implement ISpace:

            virtual const IConnection * get_connection() const override;
            virtual bool has_metric() const override;
            virtual const IMetric * get_metric() const override;
            virtual const ICurvature * get_curvature() const override;
            virtual Point point_to_cartesian(const Point & point) const override;
            virtual Vector vector_to_cartesian(const Vector & vector, const Point & at_point) const override;
        };
    }
}

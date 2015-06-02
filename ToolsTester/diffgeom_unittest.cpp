#define _USE_MATH_DEFINES
#include "tools_tester.h"
#include "Math/diffgeom.h"

TEST(DiffGeomTest, GijkGetSet)
{
    ConnectionCoeffs gijk;
    gijk.set_all(0);

    gijk.set_at(1,1,1, 1.2);
    gijk.set_at(2,1,0, -3);

    EXPECT_EQ(1.2, gijk.get_at(1,1,1));
    EXPECT_EQ(-3,  gijk.get_at(2,1,0));
}

TEST(DiffGeomTest, SphericToCartesian)
{
    SphericalCoords coords;
    // point
    EXPECT_EQ(Point(0.5, 0.5, M_SQRT1_2), coords.point_to_cartesian(Point(1, M_PI_4, M_PI_4)));
    // point on equator with phi=pi/2: e_rho = e_y, e_theta is -e_z, e_phi is -e_x
    EXPECT_EQ(Vector(-3, 1, -2), coords.vector_to_cartesian(Vector(1, 2, 3), Point(1, M_PI_2, M_PI_2)));
}

TEST(DiffGeomTest, SphericMetric)
{
    SphericalCoords::Metric metric;

    // take a small arc from phi to phi+dphi at r = 2 and theta = pi/6 - it should have length dphi
    const Real dphi = 0.01;
    const Vector v (2, M_PI/6, M_PI); // rho, theta, phi
    const Vector dv(0, 0, dphi);      // drho, dtheta, dphi

    // obtain metric tensor value at this point
    MetricTensor mt;
    metric.value_at(v, mt);
    // compare length
    EXPECT_DOUBLE_EQ(dphi, d_line_length(mt, dv));
}

TEST(DiffGeomTest, SphericMetric2)
{
    SphericalCoords::Metric metric;

    // take a small arc from theta to theta+dtheta at r = 3. It should have length 3*dtheta at any theta and any phi.
    const Real dtheta = 0.02;
    const Vector v (3, 1, 2); // rho = 3, theta and phi = arbitrary
    const Vector dv(0, dtheta, 0); // drho, dtheta, dphi

    // obtain metric tensor value at this point
    MetricTensor mt;
    metric.value_at(v, mt);
    // compare length
    EXPECT_DOUBLE_EQ(3*dtheta, d_line_length(mt, dv));
}

TEST(DiffGeomTest, SphericTransport)
{
    SphericalCoords::Connection conn;

    // take a vector v || e_r and parallel-transport it along the equator by 90 degrees. I will not be || e_r, but will become || e_phi
    Vector v (1, 0, 0);
    const Vector expected (0, 0, -1);

    // Temporary variables
    ConnectionCoeffs gijk;
    Real dphi = 0.0001;
    Vector x(1, M_PI_2, 0); // rho, theta, phi
    Vector dx(0, 0, dphi); // drho, dtheta, dphi
    // Integration
    for(Real phi = 0; phi < M_PI_2; phi += dphi)
    {
        x[SphericalCoords::PHI] = phi;
        conn.value_at(x, gijk);
        v += gijk.d_parallel_transport(v, dx);
    }
    EXPECT_PRED3(are_near, expected, v, 0.0001);
}

TEST(DiffGeomTest, SphericTransport2)
{
    SphericalCoords::Connection conn;

    // take a vector v || e_r and parallel-transport it along the meridian by 90 degrees. I will not be || e_r, but will become || e_theta
    Vector v (1, 0, 0);
    const Vector expected = Vector(1, -1, 0).normalized();

    // Temporary variables
    ConnectionCoeffs gijk;
    Real dtheta = 0.0001;
    Vector x(1, M_PI_4, 0); // rho, theta, phi
    Vector dx(0, dtheta, 0); // drho, dtheta, dphi
    // Integration
    for(Real theta = M_PI_4; theta < M_PI_2; theta += dtheta)
    {
        x[SphericalCoords::THETA] = theta;
        conn.value_at(x, gijk);
        v += gijk.d_parallel_transport(v, dx);
    }
    EXPECT_PRED3(are_near, expected, v, 0.0001);
}

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
    EXPECT_DOUBLE_EQ(dphi, mt.norm(dv));
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
    EXPECT_DOUBLE_EQ(3*dtheta, mt.norm(dv));
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
    Point x(1, M_PI_4, 0); // rho, theta, phi
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

TEST(DiffGeomTest, U2SphereCurvatureComp)
{
    Real theta = M_PI/6;
    Point x(0, theta, 0);

    UnitSphere2D::Curvature curv;
    CurvatureTensor R;
    curv.value_at(x, R);

    Real expected = sin(theta)*sin(theta);
    EXPECT_DOUBLE_EQ(expected, R.get_at(UnitSphere2D::THETA, UnitSphere2D::PHI, UnitSphere2D::THETA, UnitSphere2D::PHI));

    // print R^i_jkm
    const int theta_and_phi[] = {UnitSphere2D::THETA, UnitSphere2D::PHI};
    for (int i: theta_and_phi)
        for (int j: theta_and_phi)
            for (int k: theta_and_phi)
                for (int m: theta_and_phi)
                    std::cout << "R^" << i << "_" << j << k << m << "=" << R.get_at(i, j, k, m) << std::endl;

    UnitSphere2D::Metric metric;
    MetricTensor g;
    metric.value_at(x, g);
    // Lowering index `i`
    CurvatureTensor R_lower;
    R.lower_index(g, R_lower);
    EXPECT_DOUBLE_EQ(expected,  R_lower.get_at(UnitSphere2D::THETA, UnitSphere2D::PHI, UnitSphere2D::THETA, UnitSphere2D::PHI));
    EXPECT_DOUBLE_EQ(-expected, R_lower.get_at(UnitSphere2D::PHI, UnitSphere2D::THETA, UnitSphere2D::THETA, UnitSphere2D::PHI));

    // print R_ijkm
    std::cout << std::endl;
    for (int i: theta_and_phi) {
        for (int j: theta_and_phi) {
            for (int k: theta_and_phi) {
                for (int m: theta_and_phi) {
                    std::cout << "R_"<< i << j << k << m << "=" << R_lower.get_at(i, j, k, m) << std::endl;
                }
            }
        }
    }
}

// Get change of vector after parallel transport around infinitesimal loop
TEST(DiffGeomTest, U2SphereTransport)
{
    // vector to be transported
    Vector v(0, 1, 2);
    // Point where the loop is located
    Point x(0, M_PI/6, M_PI_2);
    Real dx = 0.001;
    // Vectors forming the loop
    Vector dx1(0, dx, 0);
    Vector dx2(0, 0, dx);


    // First way: using Gijk
    UnitSphere2D::Connection conn;
    Vector dv1 = Vector::ZERO;
    Vector v1 = v;
    Vector loop[] = {dx1, dx2, -dx1, -dx2};
    for (auto &dx: loop)
    {
        ConnectionCoeffs Gijk;
        conn.value_at(x, Gijk);
        dv1 += Gijk.d_parallel_transport(v1, dx);
        v1 = v + dv1;
        x += dx;
    }

    // Second way: using R
    UnitSphere2D::Curvature curv;
    CurvatureTensor R;
    curv.value_at(x, R);
    Vector dv2 = R.d_parallel_transport(v, dx1, dx2);
    EXPECT_PRED3(are_near, dv1, dv2, 1e-7);
}
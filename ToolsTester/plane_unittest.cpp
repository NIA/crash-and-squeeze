#include "tools_tester.h"
#include "Math/plane.h"

using namespace ::CrashAndSqueeze::Math;

TEST(PlaneTest, CreateDefault)
{
    const Plane p;
    EXPECT_EQ( Vector::ZERO, p.get_point() );
    EXPECT_EQ( 1, p.get_normal().norm() );
}

TEST(PlaneTest, CreateBad)
{
    set_tester_err_callback();
    EXPECT_THROW( const Plane p(Vector::ZERO, Vector::ZERO), ToolsTesterException );
    unset_tester_err_callback();
}

TEST(PlaneTest, PointProperty)
{
    const Vector P1(1,2,3);
    const Vector P2(4,5,6);
    const Vector N(7,8,9);
    Plane p(P1, N);

    EXPECT_EQ( P1, p.get_point() );
    p.set_point(P2);
    EXPECT_EQ( P2, p.get_point() );
}

TEST(PlaneTest, NormalProperty)
{
    const Vector P(1,2,3);
    const Vector N1(4,5,6);
    const Vector N2(7,8,9);
    Plane p(P, N1);

    EXPECT_EQ( N1.normalized(), p.get_normal() );
    p.set_normal(N2);
    EXPECT_EQ( N2.normalized(), p.get_normal() );
    set_tester_err_callback();
    EXPECT_THROW( p.set_normal(Vector::ZERO), ToolsTesterException );
    unset_tester_err_callback();
}

TEST(PlaneTest, ProjectionAndDistance)
{
    const Vector P(0,0,0);
    const Vector N(0,0,2);
    const Vector P1(1,2,3);
    const Vector P2(-3,-2,-1);
    const Plane p(P, N);

    EXPECT_EQ( P1[2], p.projection_to_normal(P1) );
    EXPECT_EQ( P1[2], p.distance_to(P1) );
    EXPECT_EQ( P2[2], p.projection_to_normal(P2) );
    EXPECT_EQ( -P2[2], p.distance_to(P2) );
}

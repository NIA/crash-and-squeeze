#include "core_tester.h"
#include "Core/force.h"

TEST(ForceTest, EverywhereForceDefault)
{
    const EverywhereForce f;
    const Vector somewhere(2, 3, 20);
    EXPECT_TRUE(f.is_active());
    EXPECT_TRUE(f.is_applied_to(somewhere));
    EXPECT_EQ(Vector::ZERO, f.get_value_at(somewhere));
}

TEST(ForceTest, EverywhereForceActivation)
{
    EverywhereForce f;
    f.deactivate();
    EXPECT_FALSE(f.is_active());
    f.activate();
    EXPECT_TRUE(f.is_active());
}

TEST(ForceTest, EverywhereForceToggle)
{
    EverywhereForce f;
    f.deactivate();
    f.toggle();
    EXPECT_TRUE(f.is_active());
    f.toggle();
    EXPECT_FALSE(f.is_active());
}

TEST(ForceTest, EverywhereForceValue)
{
    const Vector V1(1,6,2);
    const Vector V2(5,5,5);
    
    EverywhereForce f(V1);
    EXPECT_EQ(V1, f.get_value());
    f.set_value(V2);
    EXPECT_EQ(V2, f.get_value());
}

TEST(ForceTest, PointForceDefault)
{
    const PointForce f;
    EXPECT_EQ(Vector::ZERO, f.get_point_of_application());
    EXPECT_EQ(0, f.get_radius());
}

TEST(ForceTest, PointForceProperties)
{
    const Vector value(1,1,1);
    const Vector P1(0,1,2);
    const Vector P2(3,4,5);
    const Real R1 = 0.01;
    const Real R2 = 0.03;
    PointForce f(value, P1, R1);
    EXPECT_EQ(P1, f.get_point_of_application());
    f.set_point_of_application(P2);
    EXPECT_EQ(P2, f.get_point_of_application());
    f.set_radius(R2);
    EXPECT_EQ(R2, f.get_radius());
}

TEST(ForceTest, PointForceIsApplied)
{
    const Vector value(1,1,1);
    const Vector point(0,0,0);
    const Real radius = 1;
    const Vector far(1,1,1);
    const Vector near(0.5, 0.5, 0.5);
    const Vector border(0,1,0);

    PointForce f(value, point, radius);
    EXPECT_FALSE( f.is_applied_to(far) );
    EXPECT_TRUE( f.is_applied_to(near) );
    EXPECT_TRUE( f.is_applied_to(border) );
}

TEST(ForceTest, PointForceRadiusCorrection)
{
    suppress_warnings();
    const Vector V(1,1,1);
    PointForce f(V, V, -1);
    EXPECT_EQ(0, f.get_radius());
    f.set_radius(-3);
    EXPECT_EQ(0, f.get_radius());
    unsuppress_warnings();
}

TEST(ForceTest, PlaneForceDefault)
{
    const PlaneForce f;
    EXPECT_EQ(Vector::ZERO, f.get_plane_point());
    const Vector N = f.get_plane_normal();
    EXPECT_EQ(1, N.sqared_norm());
    EXPECT_EQ(0, f.get_max_distance());
}

TEST(ForceTest, PlaneForceProperties)
{
    const Vector value(1,1,1);
    const Vector P1(3,3,3);
    const Vector P2(1,3,7);
    const Vector N1(1,4,5);
    const Vector N2(8,4,8);
    const Real D1 = 0.01;
    const Real D2 = 0.21;
    
    PlaneForce f(value, P1, N1, D1);
    EXPECT_EQ(P1, f.get_plane_point());
    EXPECT_EQ(N1.normalized(), f.get_plane_normal());
    EXPECT_EQ(D1, f.get_max_distance());

    f.set_plane(P2, N2);
    EXPECT_EQ(P2, f.get_plane_point());
    EXPECT_EQ(N2.normalized(), f.get_plane_normal());

    f.set_max_distance(D2);
    EXPECT_EQ(D2, f.get_max_distance());
}

TEST(ForceTest, PlaneForceBadPlaneNormal)
{
    PlaneForce f;
    set_tester_err_callback();
    EXPECT_THROW( f.set_plane(f.get_plane_point(), Vector::ZERO), CoreTesterException ); 
    unset_tester_err_callback();
}

TEST(ForceTest, PlaneForceIsApplied)
{
    const Vector value;
    const Vector point(0,0,0);
    const Vector normal(0,0,2);
    const Real epsilon = 1;
    const Vector far(25, 25, -25);
    const Vector near(125, 125, -0.5);
    const Vector border(225, -225, 1);

    PlaneForce f(value, point, normal, epsilon);
    EXPECT_FALSE( f.is_applied_to(far) );
    EXPECT_TRUE( f.is_applied_to(near) );
    EXPECT_TRUE( f.is_applied_to(border) );
}

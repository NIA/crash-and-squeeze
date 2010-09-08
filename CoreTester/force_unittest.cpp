#include "core_tester.h"
#include "Core/force.h"

TEST(ForceTest, EverywhereForceDefault)
{
    const EverywhereForce f;
    const Vector somewhere(2, 3, 20);
    const Vector no_speed = Vector::ZERO;
    EXPECT_TRUE(f.is_active());
    EXPECT_EQ(Vector::ZERO, f.get_value_at(somewhere, no_speed));
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

TEST(ForceTest, EverywhereForceIsApplied)
{
    const EverywhereForce force;
    const Force &f = force;
    const Vector somewhere(2, 3, 20);
    
    EXPECT_TRUE( f.is_applied_to(somewhere) );
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
    EXPECT_EQ(R1, f.get_radius());
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

    const PointForce force(value, point, radius);
    const Force &f = force;
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
    EXPECT_EQ(1, N.squared_norm());
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

TEST(ForceTest, PlaneForceMaxDistanceCorrection)
{
    suppress_warnings();
    const Vector V(1,1,1);
    PlaneForce f(V, V, V, -1);
    EXPECT_EQ(0, f.get_max_distance());
    f.set_max_distance(-3);
    EXPECT_EQ(0, f.get_max_distance());
    unsuppress_warnings();
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

    const PlaneForce force(value, point, normal, epsilon);
    const Force &f = force;
    EXPECT_FALSE( f.is_applied_to(far) );
    EXPECT_TRUE( f.is_applied_to(near) );
    EXPECT_TRUE( f.is_applied_to(border) );
}

TEST(ForceTest, HalfSpaceSpringForceDefault)
{
    const HalfSpaceSpringForce f;
    EXPECT_EQ(Vector::ZERO, f.get_plane_point());
    const Vector N = f.get_plane_normal();
    EXPECT_EQ(1, N.squared_norm());
    EXPECT_EQ(0, f.get_spring_constant());
    EXPECT_EQ(0, f.get_damping_constant());
}

TEST(ForceTest, HalfSpaceSpringForceProperties)
{
    const Vector P1(3,3,3);
    const Vector P2(1,3,7);
    const Vector N1(1,4,5);
    const Vector N2(8,4,8);
    const Real k1 = 100;
    const Real k2 = 5000;
    const Real d1 = 10;
    const Real d2 = 50;
    
    HalfSpaceSpringForce f(k1, P1, N1, d1);
    EXPECT_EQ(P1, f.get_plane_point());
    EXPECT_EQ(N1.normalized(), f.get_plane_normal());
    EXPECT_EQ(k1, f.get_spring_constant());
    EXPECT_EQ(d1, f.get_damping_constant());

    f.set_plane(P2, N2);
    EXPECT_EQ(P2, f.get_plane_point());
    EXPECT_EQ(N2.normalized(), f.get_plane_normal());

    f.set_spring_constant(k2);
    EXPECT_EQ(k2, f.get_spring_constant());

    f.set_damping_constant(d2);
    EXPECT_EQ(d2, f.get_damping_constant());
}

TEST(ForceTest, HalfSpaceSpringForceSpringConstantCorrection)
{
    suppress_warnings();
    const Vector V(1,1,1);
    HalfSpaceSpringForce f(-1, V, V);
    EXPECT_EQ(0, f.get_spring_constant());
    f.set_spring_constant(-3);
    EXPECT_EQ(0, f.get_spring_constant());
    unsuppress_warnings();
}

TEST(ForceTest, HalfSpaceSpringForceIsApplied)
{
    const Vector point(0,0,0);
    const Vector normal(0,0,2);
    const Real k = 1;

    const Vector outside(2, 2, 2);
    const Vector inside(-1, -1, -1);
    const Vector border(0, 0, 0);

    const HalfSpaceSpringForce force(k, point, normal);
    const Force &f = force;

    EXPECT_FALSE( f.is_applied_to(outside) );
    EXPECT_TRUE( f.is_applied_to(border) );
    EXPECT_TRUE( f.is_applied_to(inside) );
}

TEST(ForceTest, HalfSpaceSpringForceValueAt)
{
    const Vector value;
    const Vector point(0,0,0);
    const Vector normal(0,0,2);
    const Real k = 1;
    const Vector no_speed = Vector::ZERO;

    const Vector outside(2, 2, 2);
    const Vector inside(-1, -1, -1);
    const Vector border(0, 0, 0);

    const HalfSpaceSpringForce force(k, point, normal);
    const Force &f = force;

    EXPECT_EQ( Vector::ZERO, f.get_value_at(outside, no_speed) );
    EXPECT_EQ( Vector::ZERO, f.get_value_at(border, no_speed) );
    
    const Vector val = f.get_value_at(inside, no_speed);
    EXPECT_EQ( k*abs(inside[2])*normal.normalized(), val );
}

TEST(ForceTest, CylinderSpringForceDefault)
{
    const CylinderSpringForce f;
    EXPECT_FALSE( (f.get_point2() - f.get_point1()).is_zero() );
    EXPECT_EQ(0, f.get_radius());
    EXPECT_EQ(0, f.get_spring_constant());
    EXPECT_EQ(0, f.get_damping_constant());
}

TEST(ForceTest, CylinderSpringForceProperties)
{
    const Vector P1(3,3,3);
    const Vector Q1(4,4,4);
    const Vector P2(1,3,7);
    const Vector Q2(-1,-3,-7);
    const Real R1 = 0.01;
    const Real R2 = 0.03;
    const Real k1 = 100;
    const Real k2 = 5000;
    const Real d1 = 10;
    const Real d2 = 50;

    CylinderSpringForce f(k1, P1, Q1, R1, d1);
    EXPECT_EQ(R1, f.get_radius());
    EXPECT_EQ(P1, f.get_point1());
    EXPECT_EQ(Q1, f.get_point2());
    EXPECT_EQ(k1, f.get_spring_constant());
    EXPECT_EQ(d1, f.get_damping_constant());

    f.set_radius(R2);
    EXPECT_EQ(R2, f.get_radius());

    f.set_points(P2, Q2);
    EXPECT_EQ(P2, f.get_point1());
    EXPECT_EQ(Q2, f.get_point2());

    f.set_spring_constant(k2);
    EXPECT_EQ(k2, f.get_spring_constant());

    f.set_damping_constant(d2);
    EXPECT_EQ(d2, f.get_damping_constant());
}

TEST(ForceTest, CylinderSpringForceIsApplied)
{
    const Vector p1(0,0,0);
    const Vector p2(0,0,1);
    const Real R = 1;
    const Real k = 1;

    const Vector outside1(2, 2, 0.5);
    const Vector outside2(0, 0, 10);
    const Vector outside3(2, 2, 10);
    const Vector surface(1, 0, 0.3);
    const Vector base(0.1, -0.1, 1);
    const Vector inside(0.1, -0.1, 0.1);

    CylinderSpringForce force(k, p1, p2, R);
    const Force &f = force;
    
    EXPECT_FALSE( f.is_applied_to(outside1) );
    EXPECT_FALSE( f.is_applied_to(outside2) );
    EXPECT_FALSE( f.is_applied_to(outside3) );
    EXPECT_TRUE( f.is_applied_to(surface) );
    EXPECT_TRUE( f.is_applied_to(base) );
}

TEST(ForceTest, CylinderSpringForceValueAt)
{
    const Vector p1(0,0,0);
    const Vector p2(0,0,1);
    const Real R = 1;
    const Real k = 1;
    const Vector no_speed = Vector::ZERO;

    const Vector outside1(2, 2, 0.5);
    const Vector outside2(0, 0, 10);
    const Vector outside3(2, 2, 10);
    const Vector surface(1, 0, 0.3);
    const Vector base(0.1, 0, 1);
    const Vector inside(0, -0.9, 0.1);

    CylinderSpringForce force(k, p1, p2, R);
    const Force &f = force;

    EXPECT_EQ( Vector::ZERO, f.get_value_at(outside1, no_speed) );
    EXPECT_EQ( Vector::ZERO, f.get_value_at(outside2, no_speed) );
    EXPECT_EQ( Vector::ZERO, f.get_value_at(outside3, no_speed) );
    EXPECT_EQ( Vector::ZERO, f.get_value_at(surface, no_speed) );

    Vector val = f.get_value_at(base, no_speed);
    EXPECT_EQ( k*(R - base[0])*Vector(1,0,0), val);

    val = f.get_value_at(inside, no_speed);
    EXPECT_EQ( k*(inside[1] - (-R))*Vector(0,-1,0), val);
}
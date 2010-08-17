#include "math_tester.h"
#include "../Math/vector.h"

using namespace ::CrashAndSqueeze::Math;
// TODO: make test variables constants

TEST(VectorTest, DefaultConstruct)
{
    const Point p;
    EXPECT_EQ( 0, p[0] );
    EXPECT_EQ( 0, p[1] );
    EXPECT_EQ( 0, p[2] );
}

TEST(VectorTest, Construct)
{
    const Point p(2, 3, 4.8);
    EXPECT_EQ( 2.0, p[0] );
    EXPECT_EQ( 3.0, p[1] );
    EXPECT_EQ( 4.8, p[2] );
}

TEST(VectorTest, CopyConstruct)
{
    const Point p(2, 3, 4.8);
    const Point p1( p );
    const Point p2 = p;
    EXPECT_EQ( p, p1 );
    EXPECT_EQ( p, p2 );
}

TEST(VectorTest, Equal)
{
    const Point p1a(2, 3, 4.8);
    const Point p1b(2, 3, 4.8);
    const Point p2(1, 5, 3.1);
    EXPECT_TRUE( p1a == p1a );
    EXPECT_TRUE( p1a == p1b );
    EXPECT_FALSE( p1a == p2 );
}

TEST(VectorTest, NotEqual)
{
    const Point p1a(2, 3, 4.8);
    const Point p1b = p1a;
    const Point p2(1, 5, 3.1);
    EXPECT_FALSE( p1a != p1a );
    EXPECT_FALSE( p1a != p1b );
    EXPECT_TRUE( p1a != p2 );
}

TEST(VectorTest, UnaryPlus)
{
    const Point p(2, 3, 4.8);
    EXPECT_EQ( p, +p );
}

TEST(VectorTest, UnaryMinus)
{
    const Point p(2, 3, 4.8);
    const Point p1(-2, -3, -4.8);
    EXPECT_EQ( p1, -p );
}

TEST(VectorTest, AddAssign)
{
    Point p1(2, 3, 4.8);
    const Point p2(1, -8, 1.1);
    const Point p3( p1[0] + p2[0], p1[1] + p2[1], p1[2] + p2[2] );
    EXPECT_EQ( &p1, &(p1 += p2) ); // It actually returns first argument

    EXPECT_EQ( p3, p1 );
}

TEST(VectorTest, SubAssign)
{
    Point p1(2, 3, 4.8);
    const Point p2(1, -8, 1.1);
    const Point p3( p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2] );
    EXPECT_EQ( &p1, &(p1 -= p2) ); // It actually returns first argument

    EXPECT_EQ( p3, p1 );
}

TEST(VectorTest, Add)
{
    const Point p1(2, 3, 4.8);
    const Point p2(1, -8, 1.1);
    const Point p3( p1[0] + p2[0], p1[1] + p2[1], p1[2] + p2[2] );

    EXPECT_EQ( p3, p1 + p2 );
}

TEST(VectorTest, Subtract)
{
    const Point p1(2, 3, 4.8);
    const Point p2(1, -8, 1.1);
    const Point p3( p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2] );

    EXPECT_EQ( p3, p1 - p2 );
}

TEST(VectorTest, MulAssign)
{
    Point p1(2, 3, 4.8);
    const Real d = 2.3;
    const Point p3( p1[0]*d, p1[1]*d, p1[2]*d );
    EXPECT_EQ( &p1, &(p1 *= d) ); // It actually returns first argument

    EXPECT_EQ( p3, p1 );
}

TEST(VectorTest, DivAssign)
{
    Point p1(2, 3, 4.8);
    const Real d = 2.3;
    const Point p3( p1[0]/d, p1[1]/d, p1[2]/d );
    EXPECT_EQ( &p1, &(p1 /= d) ); // It actually returns first argument

    EXPECT_EQ( p3, p1 );
}

TEST(VectorTest, Multiply)
{
    const Point p1(2, 3, 4.8);
    const Real d = 2.3;
    const Point p3( p1[0]*d, p1[1]*d, p1[2]*d );

    EXPECT_EQ( p3, p1*d );
    EXPECT_EQ( p3, d*p1 );
}

TEST(VectorTest, Divide)
{
    const Point p1(2, 3, 4.8);
    const Real d = 2.3;
    const Point p3( p1[0]/d, p1[1]/d, p1[2]/d );

    EXPECT_EQ( p3, p1/d );
}

TEST(VectorTest, ScalarMultiply)
{
    const Real a = 2, b = 3, c = 4.8;
    const Real d = 1, e = 0, f = -4.1;
    Point p1(a, b, c);
    Point p2(d, e, f);

    EXPECT_DOUBLE_EQ( a*d + b*e + c*f, p1*p2 );
}

TEST(VectorTest, SqaredNorm)
{
    const Point p1(1, 2, 3);
    const Point p2(1, -1, 0);

    EXPECT_DOUBLE_EQ( 14, p1.sqared_norm() );
    EXPECT_DOUBLE_EQ(  2, p2.sqared_norm() );
}

TEST(VectorTest, Norm)
{
    const Point p(3, -4, 0);
    EXPECT_DOUBLE_EQ( 5, p.norm() );
}

TEST(VectorTest, Normalize)
{
    Point p(3, -4, 0);
    const Point p1(3.0/5, -4.0/5, 0);
    EXPECT_EQ( &p, &( p.normalize() ) ); // It actually returns point itself
    EXPECT_EQ( p1, p );
    
    // Test normalizing when norm is 0
    Point zero1(0, 0, 0);
    const Point zero2(0, 0, 0);
    zero1.normalize();
    EXPECT_EQ( zero2, zero1 );
}

TEST(VectorTest, Normalized)
{
    const Point p(3, -4, 0);
    const Point p1(3.0/5, -4.0/5, 0);
    EXPECT_EQ( p1, p.normalized() );
}

TEST(VectorTest, Distance)
{
    const Point p1(1, 2, -3);
    const Point p2(4, -2, -3);
    EXPECT_DOUBLE_EQ( 5, distance( p1, p2 ) );
}

TEST(VectorTest, VectorMultiply)
{
    const Vector ex(1, 0, 0);
    const Vector ey(0, 1, 0);
    const Vector ez(0, 0, 1);
    const Vector ZERO(0, 0, 0);
    
    const Vector v1(1, 0, -1);
    const Vector v2(0, 1, -1);
    Vector v3(1, 1, 1);
    v3.normalize();
    v3 *= v1.norm()*v2.norm()*sin( acos( v1.normalized()*v2.normalized()) ); // area of parallelogram

    EXPECT_EQ( ez, cross_product( ex, ey ) );
    EXPECT_EQ( -ez, cross_product( ey, ex ) );
    EXPECT_EQ( ZERO, cross_product( ex, ex ) );

    EXPECT_EQ( v3, cross_product( v1, v2 ) );
}

TEST(VectorTest, IsZero)
{
    const Point ZERO(0, 0, 0);
    const Point NON_ZERO(0, 0.001, 0);

    EXPECT_TRUE( ZERO.is_zero() );
    EXPECT_FALSE( NON_ZERO.is_zero() );
}

TEST(VectorTest, IsCollinearTo)
{
    const Vector v1(1, 0, -1);
    const Vector v2(0, 1, -1);

    EXPECT_FALSE( v1.is_collinear_to(v2) );
    EXPECT_FALSE( v2.is_collinear_to(v1) );
    EXPECT_TRUE( v1.is_collinear_to(v1) );
    EXPECT_TRUE( v1.is_collinear_to(-35.8*v1) );
}

TEST(VectorTest, IsOrthogonalTo)
{
    const Vector v1 (1, 0, 1);
    const Vector v11(1, 1, 1);
    const Vector v2 (0, 1, 0);

    EXPECT_FALSE( v1.is_orthogonal_to(v1) );
    EXPECT_FALSE( v1.is_orthogonal_to(-35.8*v1) );
    EXPECT_FALSE( v1.is_orthogonal_to(v11) );
    EXPECT_TRUE( v1.is_orthogonal_to(v2) );
    EXPECT_TRUE( v2.is_orthogonal_to(v1) );
}

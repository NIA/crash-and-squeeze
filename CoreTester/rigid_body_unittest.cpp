#define _USE_MATH_DEFINES
#include "core_tester.h"
#include "Core/rigid_body.h"
#include <cmath>

inline std::ostream &operator<<(std::ostream &stream, const Matrix &m)
{
    return stream << "{{" << m.get_at(0,0) << ", " << m.get_at(0,1) << ", " << m.get_at(0,2) << "}, "
                  <<  "{" << m.get_at(1,0) << ", " << m.get_at(1,1) << ", " << m.get_at(1,2) << "}, "
                  <<  "{" << m.get_at(2,0) << ", " << m.get_at(2,1) << ", " << m.get_at(2,2) << "}}";
}

TEST(RigidBodyRotationMatrixTest, Zero)
{
    RigidBody b(Vector::ZERO, Vector::ZERO);

    EXPECT_EQ(Matrix::IDENTITY, b.get_rotation_matrix());
}

TEST(RigidBodyRotationMatrixTest, AroundX)
{
    RigidBody b(Vector::ZERO, Vector(M_PI/4, 0, 0));

    const Vector initial(0, 1, 0);
    const Vector final = Vector(0, 1, 1).normalized();

    const Vector unchanged(1, 0, 0);

    EXPECT_EQ(final, b.get_rotation_matrix()*initial);
    EXPECT_EQ(unchanged, b.get_rotation_matrix()*unchanged);
}

TEST(RigidBodyRotationMatrixTest, AroundY)
{
    RigidBody b(Vector::ZERO, Vector(0, -M_PI/2, 0));

    const Vector initial(0, 0, 1);
    const Vector final(-1, 0, 0);

    const Vector unchanged(0, 1, 0);

    EXPECT_EQ(final, b.get_rotation_matrix()*initial);
    EXPECT_EQ(unchanged, b.get_rotation_matrix()*unchanged);
}

TEST(RigidBodyRotationMatrixTest, AroundZ)
{
    RigidBody b(Vector::ZERO, Vector(0, 0, -8*M_PI + M_PI/3));

    const Vector initial(1, 0, 0);
    const Vector final(1.0/2, sqrt(3.0)/2, 0);

    const Vector unchanged(0, 0, 1);

    EXPECT_EQ(final, b.get_rotation_matrix()*initial);
    EXPECT_EQ(unchanged, b.get_rotation_matrix()*unchanged);
}

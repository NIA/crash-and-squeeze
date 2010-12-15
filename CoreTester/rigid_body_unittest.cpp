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

TEST(RigidBodyMatricesTest, Zero)
{
    EXPECT_EQ(Matrix::IDENTITY, RigidBody::x_rotation_matrix(0));
    EXPECT_EQ(Matrix::IDENTITY, RigidBody::y_rotation_matrix(0));
    EXPECT_EQ(Matrix::IDENTITY, RigidBody::z_rotation_matrix(0));
}

TEST(RigidBodyMatricesTest, AroundX)
{
    const Matrix M = RigidBody::x_rotation_matrix(M_PI/4);

    const Vector initial(0, 1, 0);
    const Vector final = Vector(0, 1, 1).normalized();

    const Vector unchanged(1, 0, 0);

    EXPECT_EQ(final,     M*initial);
    EXPECT_EQ(unchanged, M*unchanged);
}

TEST(RigidBodyMatricesTest, AroundY)
{
    const Matrix M = RigidBody::y_rotation_matrix(-M_PI/2);

    const Vector initial(0, 0, 1);
    const Vector final(-1, 0, 0);

    const Vector unchanged(0, 1, 0);

    EXPECT_EQ(final,     M*initial);
    EXPECT_EQ(unchanged, M*unchanged);
}

TEST(RigidBodyMatricesTest, AroundZ)
{
    const Matrix M = RigidBody::z_rotation_matrix(-8*M_PI + M_PI/3);

    const Vector initial(1, 0, 0);
    const Vector final(1.0/2, sqrt(3.0)/2, 0);

    const Vector unchanged(0, 0, 1);

    EXPECT_EQ(final,     M*initial);
    EXPECT_EQ(unchanged, M*unchanged);
}

TEST(RigidBodyMatricesTest, CrossProduct)
{
    const Vector v(1, 2, 3);
    const Vector u(4,-1, 8);

    const Matrix A = RigidBody::cross_product_matrix(v);

    EXPECT_EQ(cross_product(v, u), A*u);
}

TEST(RigidBodyMatricesTest, CrossProductWithMatrix)
{
    const Vector v(1, 2, 3);
    const Matrix A = RigidBody::cross_product_matrix(v);

    const Real values[MATRIX_ELEMENTS_NUM] = { 1.1, 2, 3,   6, 5, 4,   8, 7, 9 };
    const Matrix M(values);

    const Matrix R = A*M;

    for(int i = 0; i < VECTOR_SIZE; ++i)
    {
        EXPECT_EQ(cross_product(v, M.get_column(i)), R.get_column(i)) << "with i: " << i;
    }
}

void expect_matrices_almost_equal(const Matrix &m1, const Matrix &m2, Real precision)
{
    for(int i = 0; i < VECTOR_SIZE; ++i)
    {
        for(int j = 0; j < VECTOR_SIZE; ++j)
        {
            if( ! equal(m1.get_at(i,j), m2.get_at(i,j), precision) )
            {
                ADD_FAILURE() << "Matrices are not equal (precision " << precision << "):\n"
                              << "Expected: " << m1 << "\nActual: " << m2;
                return;
            }
        }
    }
}

TEST(RigidBodyTest, IntegrationWithoutAcceleration)
{
    const Vector velocity(1, 2, 3);
    const Vector angular_velocity(0, 0, 1);
    const Real dt = 0.01;

    RigidBody b(Vector::ZERO, Matrix::IDENTITY, velocity, angular_velocity);
    b.integrate(dt);

    EXPECT_EQ(velocity*dt, b.get_position());
    
    const Matrix expected_orientation = RigidBody::z_rotation_matrix(angular_velocity.norm()*dt);
    expect_matrices_almost_equal(expected_orientation, b.get_orientation(), 1e-4);
}
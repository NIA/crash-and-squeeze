#include "math_tester.h"
#include "../Math/matrix.h"

using namespace ::CrashAndSqueeze::Math;

class MatrixTest : public ::testing::Test
{
protected:
    Matrix empty_mx;
    Matrix m1;
    Matrix m2;
    Matrix m1_plus_m2;
    Matrix m1_minus_m2;
    double alpha;
    Matrix m1_mul_alpha;
    Matrix m1_div_alpha;
    Matrix m1_transposed;

    virtual void SetUp()
    {
        const double values1[MATRIX_ELEMENTS_NUM] =
            { 9, 8, 7,
              6, 5, 4,
              3, 2, 1 };
        const double values_transposed[MATRIX_ELEMENTS_NUM] =
            { 9, 6, 3,
              8, 5, 2,
              7, 4, 1 };
       
        m1 = Matrix(values1);
        m1_transposed = Matrix(values_transposed);
        
        const double values2[MATRIX_ELEMENTS_NUM] =
            { 1, 2, 1,
              0, 1, 2,
             -1, 0, 1 };
        m2 = Matrix(values2);

        double values1plus2[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1plus2[i] = values1[i] + values2[i];
        m1_plus_m2 = Matrix(values1plus2);

        double values1minus2[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1minus2[i] = values1[i] - values2[i];
        m1_minus_m2 = Matrix(values1minus2);

        alpha = 1.25;

        double values1_mul_alpha[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1_mul_alpha[i] = values1[i]*alpha;
        m1_mul_alpha = Matrix(values1_mul_alpha);

        double values1_div_alpha[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1_div_alpha[i] = values1[i]/alpha;
        m1_div_alpha = Matrix(values1_div_alpha);
    }
};

TEST_F(MatrixTest, DefaultConstructAndRead)
{
    EXPECT_EQ( 0, empty_mx.get_at(0,0) );
    EXPECT_EQ( 0, empty_mx.get_at(0,1) );
    EXPECT_EQ( 0, empty_mx.get_at(2,2) );
}

TEST_F(MatrixTest, ConstructAndRead)
{
    EXPECT_EQ( 9, m1.get_at(0,0) );
    EXPECT_EQ( 8, m1.get_at(0,1) );
    EXPECT_EQ( 1, m1.get_at(2,2) );
}
TEST_F(MatrixTest, ConstructFromVectorMultiply)
{
    const double values[MATRIX_ELEMENTS_NUM] =
        { 4, 5, 6,
          8, 10, 12,
          12, 15, 18 };
    const Matrix M(values);
    const Vector left(1,2,3);
    const Vector right(4,5,6);

    EXPECT_TRUE(M == Matrix(left, right));
}

TEST_F(MatrixTest, Equals)
{
    const Matrix mm1 = m1;

    EXPECT_TRUE(m1 == mm1);
    EXPECT_FALSE(m2 == m1);
}

TEST_F(MatrixTest, NotEquals)
{
    const Matrix mm1 = m1;

    EXPECT_FALSE(mm1 != m1);
    EXPECT_TRUE(m1 != m2);
}

TEST_F(MatrixTest, AddAssign)
{
    m1 += m2;
    EXPECT_TRUE(m1_plus_m2 == m1);
}

TEST_F(MatrixTest, SubAssign)
{
    m1 -= m2;
    EXPECT_TRUE(m1_minus_m2 == m1);
}

TEST_F(MatrixTest, MulAssign)
{
    m1 *= alpha;
    EXPECT_TRUE(m1_mul_alpha == m1);
}

TEST_F(MatrixTest, DivAssign)
{
    m1 /= alpha;
    EXPECT_TRUE(m1_div_alpha == m1);
}

TEST_F(MatrixTest, Add)
{
    EXPECT_TRUE(m1_plus_m2 == m1 + m2);
}

TEST_F(MatrixTest, Sub)
{
    EXPECT_TRUE(m1_minus_m2 == m1 - m2);
}

TEST_F(MatrixTest, Mul)
{
    EXPECT_TRUE(m1_mul_alpha == m1*alpha);
    EXPECT_TRUE(m1_mul_alpha == alpha*m1);
}

TEST_F(MatrixTest, Div)
{
    EXPECT_TRUE(m1_div_alpha == m1/alpha);
}

TEST_F(MatrixTest, UnaryMinus)
{
    const Matrix minus_m1 = m1*(-1);
    EXPECT_TRUE(minus_m1 == -m1);
}

TEST_F(MatrixTest, UnaryPlus)
{
    EXPECT_TRUE(m1 == +m1);
}

TEST_F(MatrixTest, IdentityMatrix)
{
    // it should exist and look plausible
    EXPECT_EQ(1, Matrix::IDENTITY.get_at(1,1));
    EXPECT_EQ(0, Matrix::IDENTITY.get_at(0,2));
}

TEST_F(MatrixTest, MatrixMultiply)
{
    const double values[MATRIX_ELEMENTS_NUM] =
        { 2, 26, 32,
          2, 17, 20,
          2, 8, 8};
    Matrix m1_mul_m2(values);
    EXPECT_TRUE(m1_mul_m2 == m1*m2);
}

TEST_F(MatrixTest, VectorMultiply)
{
    const Vector before(9456, 0.3, -18);
    const Vector after(84980.4, 56665.5, 28350.6);

    EXPECT_EQ(after, m1*before);
}

TEST_F(MatrixTest, Transposed)
{
    EXPECT_TRUE(m1_transposed == m1.transposed());
}

TEST_F(MatrixTest, Transpose)
{
    EXPECT_EQ( &m1, &m1.transpose() ); // it actually returns itself
    EXPECT_TRUE(m1_transposed == m1);
}

TEST_F(MatrixTest, Determinant)
{
    EXPECT_EQ( 0, m1.determinant() );
    EXPECT_EQ( -2, m2.determinant() );
    const double d = Matrix::IDENTITY.determinant();
    EXPECT_EQ( 1, d );
}

TEST_F(MatrixTest, Inverted)
{
    const double values[MATRIX_ELEMENTS_NUM] =
        { -1,  2, -3,
           2, -2,  2,
          -1,  2, -1 };
    Matrix m2_inv(values);
    m2_inv /= 2;
    EXPECT_TRUE( m2_inv == m2.inverted() );
}

TEST_F(MatrixTest, InvertedIdentity)
{
    Matrix const &I = Matrix::IDENTITY;
    EXPECT_TRUE( I == I.inverted() );
}
TEST_F(MatrixTest, InvertedArbitrary)
{
    const double values[MATRIX_ELEMENTS_NUM] =
        { 6, -2,  1,
         18,  1, -2,
          1, 22,  1 };
    const double inv_values[MATRIX_ELEMENTS_NUM] =
        { 45,  24 ,  3,
         -20,   5 , 30,
         395, -134, 42 };
    const Matrix M(values);
    Matrix inv(inv_values);
    inv /= 705;
    EXPECT_TRUE( inv == M.inverted() );
}


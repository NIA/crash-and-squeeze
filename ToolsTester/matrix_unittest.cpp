#include "tools_tester.h"
#include "Math/matrix.h"

using namespace ::CrashAndSqueeze::Math;


class MatrixTest : public ::testing::Test
{
protected:
    Matrix m1;
    Matrix m2;
    Matrix m3;
    Matrix m1_plus_m2;
    Matrix m1_minus_m2;
    Real alpha;
    Matrix m1_mul_alpha;
    Matrix m1_div_alpha;
    Matrix m1_transposed;
    Matrix I;
    Matrix diagonal;
    Matrix exp_diagonal;
    Matrix orthogonal;

    virtual void SetUp()
    {
        const Real values1[MATRIX_ELEMENTS_NUM] =
            { 9, 8, 7,
              6, 5, 4,
              3, 2, 1 };
        const Real values_transposed[MATRIX_ELEMENTS_NUM] =
            { 9, 6, 3,
              8, 5, 2,
              7, 4, 1 };
       
        m1 = Matrix(values1);
        m1_transposed = Matrix(values_transposed);
        
        const Real values2[MATRIX_ELEMENTS_NUM] =
            { 1, 2, 1,
              0, 1, 2,
             -1, 0, 1 };
        m2 = Matrix(values2);

        const Real values3[MATRIX_ELEMENTS_NUM] =
            { 1.2,  2.8, 1.008,
                0, -5.6,     2,
                0,    0,   7.4 };
        m3 = Matrix(values3);

        Real values1plus2[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1plus2[i] = values1[i] + values2[i];
        m1_plus_m2 = Matrix(values1plus2);

        Real values1minus2[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1minus2[i] = values1[i] - values2[i];
        m1_minus_m2 = Matrix(values1minus2);

        alpha = 1.25;

        Real values1_mul_alpha[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1_mul_alpha[i] = values1[i]*alpha;
        m1_mul_alpha = Matrix(values1_mul_alpha);

        Real values1_div_alpha[MATRIX_ELEMENTS_NUM];
        for(int i = 0; i < MATRIX_ELEMENTS_NUM; ++i)
            values1_div_alpha[i] = values1[i]/alpha;
        m1_div_alpha = Matrix(values1_div_alpha);

        I = Matrix::IDENTITY;

        diagonal = Matrix::ZERO;
        diagonal.set_at(0, 0, 8);
        diagonal.set_at(1, 1, -1.94);
        diagonal.set_at(2, 2, 2.65);

        exp_diagonal = Matrix::ZERO;
        for(int i = 0; i < VECTOR_SIZE; ++i)
            exp_diagonal.set_at(i, i, exp( diagonal.get_at(i, i)));

        Real orthogonal_values[MATRIX_ELEMENTS_NUM] =
            {  0 , -0.80, -0.60,
             0.80, -0.36,  0.48,
             0.60,  0.48, -0.64 };
        orthogonal = Matrix(orthogonal_values);
    }

    void test_jacobi_rotation(const Matrix & matrix);
    void test_diagonalization(const Matrix & matrix, int rotations, Real accuracy);
    void test_polar_decomposition(const Matrix &matrix, int rotations, Real accuracy);

    bool is_approximately_diagonalized(const Matrix &matrix, Real accuracy)
    {
        return equal( 0, matrix.get_at(0,1), accuracy )
            && equal( 0, matrix.get_at(0,2), accuracy )
            && equal( 0, matrix.get_at(1,2), accuracy )
            && equal( 0, matrix.get_at(1,0), accuracy )
            && equal( 0, matrix.get_at(2,0), accuracy )
            && equal( 0, matrix.get_at(2,1), accuracy );
    }
};

TEST_F(MatrixTest, ConstructAndRead)
{
    EXPECT_EQ( 9, m1.get_at(0,0) );
    EXPECT_EQ( 8, m1.get_at(0,1) );
    EXPECT_EQ( 1, m1.get_at(2,2) );
}

TEST_F(MatrixTest, ConstructAlternative)
{
    Matrix m1_like(9, 8, 7,
                   6, 5, 4,
                   3, 2, 1);
    EXPECT_EQ(m1, m1_like);
}

TEST_F(MatrixTest, GetAtIndex)
{
    EXPECT_EQ( 9, m1.get_at_index(0) );
    EXPECT_EQ( 8, m1.get_at_index(1) );
    EXPECT_EQ( 1, m1.get_at_index(8) );
}

#ifndef NDEBUG
// checks enabled only in debug
TEST_F(MatrixTest, BadAccess)
{
    set_tester_err_callback();
    EXPECT_THROW( m1.get_at(0,40), ToolsTesterException );
    EXPECT_THROW( m2.get_at(-1,0), ToolsTesterException );
    unset_tester_err_callback();
}
TEST_F(MatrixTest, BadGetAtIndex)
{
    set_tester_err_callback();
    EXPECT_THROW( m1.get_at_index(40), ToolsTesterException );
    EXPECT_THROW( m2.get_at_index(-1), ToolsTesterException );
    unset_tester_err_callback();
}
#endif // #ifndef NDEBUG

TEST_F(MatrixTest, Write)
{
    m1.set_at(1, 1, 155);
    EXPECT_EQ( 155, m1.get_at(1, 1) );
    m1.set_at(0, 2, 156);
    EXPECT_EQ( 156, m1.get_at(0, 2) );
    m1.set_at(1, 0, 157);
    EXPECT_EQ( 157, m1.get_at(1, 0) );
}

TEST_F(MatrixTest, AddWrite)
{
    Real old = m1.get_at(1, 1);
    m1.add_at(1, 1, -155);
    EXPECT_EQ( old - 155, m1.get_at(1,1) );
}

TEST_F(MatrixTest, GetRow)
{
    Vector row1(m1.get_at(1,0), m1.get_at(1,1), m1.get_at(1,2));
    Vector row2(m1.get_at(2,0), m1.get_at(2,1), m1.get_at(2,2));
    
    EXPECT_EQ(row1, m1.get_row(1));
    EXPECT_EQ(row2, m1.get_row(2));
}

TEST_F(MatrixTest, GetColumn)
{
    Vector col0(m1.get_at(0,0), m1.get_at(1,0), m1.get_at(2,0));
    Vector col2(m1.get_at(0,2), m1.get_at(1,2), m1.get_at(2,2));
    
    EXPECT_EQ(col0, m1.get_column(0));
    EXPECT_EQ(col2, m1.get_column(2));
}

TEST_F(MatrixTest, ConstructFromVectorMultiply)
{
    const Real values[MATRIX_ELEMENTS_NUM] =
        { 4, 5, 6,
          8, 10, 12,
          12, 15, 18 };
    const Matrix M(values);
    const Vector left(1,2,3);
    const Vector right(4,5,6);

    EXPECT_EQ(M, Matrix(left, right));
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
    EXPECT_EQ(m1_plus_m2, m1);
}

TEST_F(MatrixTest, SubAssign)
{
    m1 -= m2;
    EXPECT_EQ(m1_minus_m2, m1);
}

TEST_F(MatrixTest, MulAssign)
{
    m1 *= alpha;
    EXPECT_EQ(m1_mul_alpha, m1);
}

TEST_F(MatrixTest, DivAssign)
{
    m1 /= alpha;
    EXPECT_EQ(m1_div_alpha, m1);
}

TEST_F(MatrixTest, Add)
{
    EXPECT_EQ(m1_plus_m2, m1 + m2);
}

TEST_F(MatrixTest, Sub)
{
    EXPECT_EQ(m1_minus_m2, m1 - m2);
}

TEST_F(MatrixTest, Mul)
{
    EXPECT_EQ(m1_mul_alpha, m1*alpha);
    EXPECT_EQ(m1_mul_alpha, alpha*m1);
}

TEST_F(MatrixTest, Div)
{
    EXPECT_EQ(m1_div_alpha, m1/alpha);
}

TEST_F(MatrixTest, UnaryMinus)
{
    const Matrix minus_m1 = m1*(-1);
    EXPECT_EQ(minus_m1, -m1);
}

TEST_F(MatrixTest, UnaryPlus)
{
    EXPECT_EQ(m1, +m1);
}

TEST_F(MatrixTest, IdentityMatrix)
{
    // it should exist and look plausible
    EXPECT_EQ(1, I.get_at(1,1));
    EXPECT_EQ(0, I.get_at(0,2));
}

TEST_F(MatrixTest, MatrixMultiply)
{
    const Real values[MATRIX_ELEMENTS_NUM] =
        { 2, 26, 32,
          2, 17, 20,
          2, 8, 8};
    Matrix m1_mul_m2(values);
    EXPECT_EQ(m1_mul_m2, m1*m2);
}

TEST_F(MatrixTest, VectorMultiply)
{
    const Vector before(9456, 0.3, -18);
    const Vector after(84980.4, 56665.5, 28350.6);

    EXPECT_EQ(after, m1*before);
}

TEST_F(MatrixTest, Transposed)
{
    EXPECT_EQ(m1_transposed, m1.transposed());
}

TEST_F(MatrixTest, Transpose)
{
    EXPECT_EQ( &m1, &m1.transpose() ); // it actually returns itself
    EXPECT_EQ(m1_transposed, m1);
}

TEST_F(MatrixTest, Determinant)
{
    EXPECT_EQ( 0, m1.determinant() );
    EXPECT_EQ( -2, m2.determinant() );
    EXPECT_EQ( 1, I.determinant() );
}

TEST_F(MatrixTest, IsInvertible)
{
    EXPECT_FALSE( m1.is_invertible() );
    EXPECT_TRUE(  m2.is_invertible() );
}

TEST_F(MatrixTest, Inverted)
{
    EXPECT_EQ( I, m2*m2.inverted() );
    EXPECT_EQ( I, m2.inverted()*m2 );
}

TEST_F(MatrixTest, BadInverted)
{
    const Matrix M = Matrix::ZERO;
    set_tester_err_callback();
    EXPECT_THROW( M.inverted(), ToolsTesterException );
    unset_tester_err_callback();
}

TEST_F(MatrixTest, Invert)
{
    const Matrix old_m2 = m2;
    EXPECT_TRUE( m2.invert() );
    EXPECT_EQ( I, old_m2*m2 );
    EXPECT_EQ( I, m2*old_m2 );
}

TEST_F(MatrixTest, BadInvert)
{
    const Matrix old_m1 = m1;
    EXPECT_FALSE( m1.invert() );
    EXPECT_EQ( old_m1, m1 ); // matrix should be left untouched if it cannot be inverted
}

TEST_F(MatrixTest, InvertSym)
{
    Matrix sym(
        1, -1, 0,
        -1, 2, -1,
        0, -1, -1
    );
    const Matrix inv(
        1.5, 0.5, -0.5,
        0.5, 0.5, -0.5,
        -0.5, -0.5, -0.5
    );
    sym.invert_sym();
    EXPECT_EQ(inv, sym);
}

TEST_F(MatrixTest, InvertedIdentity)
{
    EXPECT_EQ( I, I.inverted() );
}
TEST_F(MatrixTest, InvertedArbitrary)
{
    const Real values[MATRIX_ELEMENTS_NUM] =
        { 6.8,   -2,  1,
         18.1, 1.05, -2,
           -1,   22,  1 };
    const Matrix M(values);
    EXPECT_EQ( I, M*M.inverted() );
    EXPECT_EQ( I, M.inverted()*M );
}

TEST_F(MatrixTest, SquaredNorm)
{
    EXPECT_EQ(3, I.squared_norm());
    EXPECT_EQ(13, m2.squared_norm());
}

TEST_F(MatrixTest, Norm)
{
    const Real values[MATRIX_ELEMENTS_NUM] =
        { 1,-1, 1,
          1, 1,-1,
         -1, 1, 1 };
    const Matrix M(values);
    EXPECT_EQ(3, M.norm());
}

void MatrixTest::test_jacobi_rotation(const Matrix & src_matrix)
{
    Matrix P = I;
    Matrix matrix = src_matrix;

    for(int i = 0; i < 100; ++i)
    {
        matrix.do_jacobi_rotation(0, 2, P);
        EXPECT_EQ(0, matrix.get_at(0, 2));
        EXPECT_EQ(0, matrix.get_at(2, 0));
        EXPECT_EQ( src_matrix, P*matrix*P.transposed() );

        matrix.do_jacobi_rotation(1, 0, P);
        EXPECT_EQ(0, matrix.get_at(1, 0));
        EXPECT_EQ(0, matrix.get_at(0, 1));
        EXPECT_EQ( src_matrix, P*matrix*P.transposed() );

        matrix.do_jacobi_rotation(2, 1, P);
        EXPECT_EQ(0, matrix.get_at(2, 1));
        EXPECT_EQ(0, matrix.get_at(1, 2));
        EXPECT_EQ( src_matrix, P*matrix*P.transposed() );
    }
}

void simmetrize(Matrix &m)
{
    m.set_at(1, 0, m.get_at(0, 1));
    m.set_at(2, 0, m.get_at(0, 2));
    m.set_at(2, 1, m.get_at(1, 2));
}

TEST_F(MatrixTest, JacobiRotation1)
{
    simmetrize(m1);
    test_jacobi_rotation(m1);
}

TEST_F(MatrixTest, JacobiRotation2)
{
    simmetrize(m2);
    test_jacobi_rotation(m2);
}

TEST_F(MatrixTest, JacobiRotation3)
{
    simmetrize(m3);
    test_jacobi_rotation(m3);
}

TEST_F(MatrixTest, JacobiRotationDummy)
{
    Matrix P = I;
    Matrix A = I;
    A.do_jacobi_rotation(2, 0, P);
    EXPECT_EQ(I, A);
    EXPECT_EQ(I, P);
}

void MatrixTest::test_diagonalization(const Matrix & src_matrix, int rotations, Real accuracy)
{
    Matrix V;
    Matrix matrix = src_matrix.diagonalized(rotations, V, accuracy);
    EXPECT_EQ( src_matrix, V*matrix*V.transposed() );
    EXPECT_TRUE( is_approximately_diagonalized(matrix, accuracy) );
}

TEST_F(MatrixTest, Diagonalization1_4)
{
    simmetrize(m1);
    test_diagonalization(m1, 4, 0.9);
}
TEST_F(MatrixTest, Diagonalization1_6)
{
    simmetrize(m1);
    test_diagonalization(m1, 6, 0.035);
}
TEST_F(MatrixTest, Diagonalization1_9)
{
    simmetrize(m1);
    test_diagonalization(m1, 9, 1e-9);
}
TEST_F(MatrixTest, Diagonalization1_12)
{
    simmetrize(m1);
    test_diagonalization(m1, 12, 1e-40);
}

TEST_F(MatrixTest, Diagonalization2)
{
    simmetrize(m2);
    test_diagonalization(m2, 6, 0.0005);
}

TEST_F(MatrixTest, Diagonalization3)
{
    simmetrize(m3);
    test_diagonalization(m3, 5, 0.006);
}

TEST_F(MatrixTest, DiagonalizationOfDiagonalized)
{
    Matrix P = I;
    Matrix A = diagonal;
    A.diagonalize(1, P);
    EXPECT_EQ(diagonal, A);
    EXPECT_EQ(I, P);
}

TEST_F(MatrixTest, FunctionOfDiagonalized)
{
    EXPECT_EQ(exp_diagonal, diagonal.compute_function(exp));
}

TEST_F(MatrixTest, FunctionOfArbitrary)
{
    const Matrix argument = orthogonal.transposed()*diagonal*orthogonal; // transform diagonal matrix somehow
    const Matrix expected = orthogonal.transposed()*exp_diagonal*orthogonal; // transform result the same way
    EXPECT_EQ(expected, argument.compute_function(exp, 3));
}

TEST_F(MatrixTest, BadFunction)
{
    set_tester_err_callback();
    EXPECT_THROW( m1.compute_function(NULL), ToolsTesterException );
    unset_tester_err_callback();
}

void MatrixTest::test_polar_decomposition(Matrix const &matrix, int rotations, Real accuracy)
{
    Matrix R;
    Matrix S;
    matrix.do_polar_decomposition(R, S, rotations, 2*rotations, accuracy);
    
    EXPECT_EQ( matrix, R*S );
    
    if ( !equal(0, matrix.determinant()) )
    {
        EXPECT_NEAR(1, abs(R.determinant()), accuracy);
    }
    
    Matrix S_simmetrized = S;
    simmetrize(S_simmetrized);
    EXPECT_EQ( S, S_simmetrized );
}

TEST_F(MatrixTest, PolarDecompositionOfInvertible1)
{
    test_polar_decomposition(m2, ::CrashAndSqueeze::Math::DEFAULT_JACOBI_ROTATIONS_COUNT, 1e-8);
}

TEST_F(MatrixTest, PolarDecompositionOfInvertible2)
{
    test_polar_decomposition(m3, 12, 1e-10);
}

TEST_F(MatrixTest, PolarDecompositionOfUninvertible)
{
    test_polar_decomposition(m1, 9, 1e-10);
}

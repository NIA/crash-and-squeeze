#include "tools_tester.h"
#include "Math/quadratic.h"

TEST(QxTest, TriVectorInit) {
    const Vector v(-1, 2, 3);
    const TriVector tv(v);
    EXPECT_EQ(v, tv.vectors[0]);
    EXPECT_EQ(Vector( 1, 4, 9), tv.vectors[1]);
    EXPECT_EQ(Vector(-2, 6,-3), tv.vectors[2]);
}

TEST(QxTest, TriMatrixInit)
{
    const Vector v(1, 0, -1);
    const TriVector tv(Vector(1, 0, 2), Vector(0, -1, 0), Vector(0, -3, 1));
    const TriMatrix expected(
        Matrix(1, 0, 2,
               0, 0, 0,
              -1, 0,-2),
        Matrix(0,-1, 0,
               0, 0, 0,
               0, 1, 0),
        Matrix(0,-3, 1,
               0, 0, 0,
               0, 3,-1)
    );
    const TriMatrix actual(v, tv);
    EXPECT_EQ(expected.matrices[0], actual.matrices[0]);
    EXPECT_EQ(expected.matrices[1], actual.matrices[1]);
    EXPECT_EQ(expected.matrices[2], actual.matrices[2]);
}

TEST(QxTest, TriMatrixAdd)
{
    TriMatrix tm1(
        Matrix(1, 0, 2,
               0, 0, 0,
              -1, 0,-2),
        Matrix(0,-1, 0,
               0, 0, 0,
               0, 1, 0),
        Matrix(0,-3, 1,
               0, 0, 0,
               0, 3,-1)
    );
    const TriMatrix tm2(
        Matrix(0, 2, 0,
               5, 0, 0,
               0, 0, 1),
        Matrix(3, 0, 0,
               0, 2, 0,
               0, 0, 1),
        Matrix(0, 0,-2,
               0,-1, 0,
               1, 0, 0)
    );
    const TriMatrix expected(
        Matrix(1, 2, 2,
               5, 0, 0,
              -1, 0,-1),
        Matrix(3,-1, 0,
               0, 2, 0,
               0, 1, 1),
        Matrix(0,-3,-1,
               0,-1, 0,
               1, 3,-1)
    );
    tm1 += tm2;
    EXPECT_EQ(expected.matrices[0], tm1.matrices[0]);
    EXPECT_EQ(expected.matrices[1], tm1.matrices[1]);
    EXPECT_EQ(expected.matrices[2], tm1.matrices[2]);
}

TEST(QxTest, TriMatrixMulConst)
{
    TriMatrix tm(
        Matrix(1, 0, 2,
               0, 0, 0,
              -1, 0,-2),
        Matrix(0,-1, 0,
               0, 0, 0,
               0, 1, 0),
        Matrix(0,-3, 1,
               0, 0, 0,
               0, 3,-1)
    );
    const TriMatrix expected(
        Matrix(3, 0, 6,
               0, 0, 0,
              -3, 0,-6),
        Matrix(0,-3, 0,
               0, 0, 0,
               0, 3, 0),
        Matrix(0,-9, 3,
               0, 0, 0,
               0, 9,-3)
    );
    tm *= 3;
    EXPECT_EQ(expected.matrices[0], tm.matrices[0]);
    EXPECT_EQ(expected.matrices[1], tm.matrices[1]);
    EXPECT_EQ(expected.matrices[2], tm.matrices[2]);
}

//  1  2  0  1  2  3  1  1 -1                                   4
//  0  1  0  3  4  5  0 -1  1   x  [ 1 2 3 1 0 -1 4 -2 1 ]T  =  3
//  3  0  1  3  2  1  0  0 -1                                   7
TEST(QxTest, TriMatrixMultTriVector)
{
    const TriMatrix tm(
        Matrix(1, 2, 0,
               0, 1, 0,
               3, 0, 1),
        Matrix(1, 2, 3,
               3, 4, 5,
               3, 2, 1),
        Matrix(1, 1,-1,
               0,-1, 1,
               0, 0,-1)
    );
    const TriVector tv(Vector(1, 2, 3), Vector(1, 0, -1), Vector(4, -2, 1));
    EXPECT_EQ(Vector(4, 3, 7), tm*tv);
}

// Full expected result: (only few submatrices checked)
// 1  2  3  1  1 -1  4 -2  1
// 2  4  6  2  2 -2  8 -4  2
// 3  6  9  3  3 -3 12 -6  3
// 1  2  3  1  1 -1  4 -2  1
// 1  2  3  1  1 -1  4 -2  1
//-1 -2 -3 -1 -1  1 -4  2 -1
// 4  8 12  4  4 -4 16 -8  4
//-2 -4 -6 -2 -2  2 -8  4 -2
// 1  2  3  1  1 -1  4 -2  1
TEST(QxTest, NineMatrixInit)
{
    const TriVector tv(Vector(1, 2, 3), Vector(1, 1, -1), Vector(4, -2, 1));
    const Matrix expected_0_0(1, 2, 3,
                              2, 4, 6,
                              3, 6, 9);
    const Matrix expected_2_0(4, 8,12,
                             -2,-4,-6,
                              1, 2, 3);
    const Matrix expected_1_2(4,-2, 1,
                              4,-2, 1,
                             -4, 2,-1);
    const NineMatrix actual(tv, tv);
    EXPECT_EQ(expected_0_0, actual.submatrix(0,0));
    EXPECT_EQ(expected_2_0, actual.submatrix(2,0));
    EXPECT_EQ(expected_1_2, actual.submatrix(1,2));
}

// TODO: test NineMatrix::operator+=

TEST(QxTest, NineMatrixMulTriMatrix)
{
    const TriVector tv(Vector(1, 2, 3), Vector(1, 1, -1), Vector(4, -2, 1));
    // Use the same ninematrix as in previous test:
    const NineMatrix nm(tv, tv);
    const TriMatrix tm(
        Matrix(1, 2, 0,
               0, 1, 0,
               3, 0, 1),
        Matrix(1, 2, 3,
               3, 4, 5,
               3, 2, 1),
        Matrix(1, 1,-1,
               0,-1, 1,
               0, 0,-1)
    );
    const TriMatrix expected(
        Matrix(6,  12,  18,
               7,  14,  21,
               9,  18,  27),
        Matrix( 6,  6,  -6,
                7,  7,  -7,
                9,  9,  -9),
        Matrix( 24, -12, 6,
                28, -14, 7,
                36, -18, 9)
    );
    TriMatrix actual;
    nm.left_mult_by(tm, actual);
    EXPECT_EQ(expected.matrices[0], actual.matrices[0]);
    EXPECT_EQ(expected.matrices[1], actual.matrices[1]);
    EXPECT_EQ(expected.matrices[2], actual.matrices[2]);
}

TEST(QxTest, NineMatrixInvert)
{
    // input
    const Real m_values[NineMatrix::SIZE][NineMatrix::SIZE] = {
        {1,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,2,0,0,0},
        {0,0,1,0,0,0,0,0,0},
        {0,0,0,0.5,0,0,-1,0,0},
        {0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0},
        {0,0,0,0,0,0,0,0,1}
    };
    NineMatrix m(m_values);
    // expected result
    const Real m_inv_values[NineMatrix::SIZE][NineMatrix::SIZE] = {
        {1, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 0, 0, 0, -2, 0, 0, 0},
        {0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, -2, 0, 2, 0, 4, 2, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 1, 0, 0, 0},
        {0, -1, 0, 0, 0, 2, 1, 0, 0},
        {0, 0, -1, 0, 0, 0, 0, 1, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 1}
    };
    NineMatrix expected(m_inv_values);

    EXPECT_TRUE(m.invert());
    // check only few submatrices
    EXPECT_EQ(expected.submatrix(2, 0), m.submatrix(2, 0));
    EXPECT_EQ(expected.submatrix(1, 1), m.submatrix(1, 1));
    EXPECT_EQ(expected.submatrix(0, 1), m.submatrix(0, 1));
}

TEST(QxTest, NineMatrixBadInvert) {
    // input (singular: line 3 and 4 equal)
    const Real m_values[NineMatrix::SIZE][NineMatrix::SIZE] = {
        {1,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,2,0,0,0},
        {0,0,0,0.5,0,0,-1,0,0},
        {0,0,0,0.5,0,0,-1,0,0},
        {0,0,0,0,1,0,0,0,0},
        {0,0,0,0,0,1,0,0,0},
        {0,1,0,0,0,0,1,0,0},
        {0,0,1,0,0,0,0,1,0},
        {0,0,0,0,0,0,0,0,1}
    };
    NineMatrix m(m_values);
    const NineMatrix old_m = m;
    EXPECT_FALSE( m.invert() );
    EXPECT_TRUE( old_m == m ); // matrix should be left untouched if it cannot be inverted
}

#include "../Math/matrix.h"
#include <gtest/gtest.h>

using namespace ::CrashAndSqueeze::Math;

TEST(MatrixTest, DefaultConstructAndRead)
{
    const Matrix M;
    EXPECT_EQ( 0, M.get_at(0,0) );
    EXPECT_EQ( 0, M.get_at(0,1) );
    EXPECT_EQ( 0, M.get_at(2,2) );
}

TEST(MatrixTest, ConstructAndRead)
{
    const double values[MATRIX_ELEMENTS_NUM] =
        { 9, 8, 7,
          6, 5, 4,
          3, 2, 1 };
    const Matrix M(values);
    EXPECT_EQ( 9, M.get_at(0,0) );
    EXPECT_EQ( 8, M.get_at(0,1) );
    EXPECT_EQ( 1, M.get_at(2,2) );
}

TEST(MatrixTest, ConstructFromVectorMultiply)
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

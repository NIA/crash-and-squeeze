#include "../Math/matrix.h"
#include <gtest/gtest.h>

using namespace ::CrashAndSqueeze::Math;

TEST(MatrixTest, DefaultConstructAndRead)
{
    const Matrix M;
    EXPECT_EQ( 0, M.get_at(1,1) );
    EXPECT_EQ( 0, M.get_at(1,2) );
    EXPECT_EQ( 0, M.get_at(3,3) );
}

TEST(MatrixTest, ConstructAndRead)
{
    const double values[MATRIX_ELEMENTS_NUM] =
        { 9, 8, 7,
          6, 5, 4,
          3, 2, 1 };
    const Matrix M(values);
    EXPECT_EQ( 9, M.get_at(1,1) );
    EXPECT_EQ( 8, M.get_at(1,2) );
    EXPECT_EQ( 1, M.get_at(3,3) );
}

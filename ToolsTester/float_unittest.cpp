#include "tools_tester.h"
#include "Math/floating_point.h"

// Floating point functions tests

TEST(FloatingPointTest, Sign)
{
    EXPECT_EQ( 0, sign(0.0) );
    EXPECT_EQ( 1, sign(3.3) );
    EXPECT_EQ( -1, sign(-800.008) );
}

TEST(FloatingPointTest, CubeRoot)
{
    EXPECT_DOUBLE_EQ( 2, cube_root(8) );
    EXPECT_DOUBLE_EQ( 0, cube_root(0) );
    EXPECT_DOUBLE_EQ( -6, cube_root(-216) );
}

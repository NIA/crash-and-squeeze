#include "tools_tester.h"
#include "Math/floating_point.h"

// Floating point functions tests

TEST(FloatingPointTest, Sign)
{
    EXPECT_EQ( 0, sign(0.0) );
    EXPECT_EQ( 1, sign(3.3) );
    EXPECT_EQ( -1, sign(-800.008) );
}

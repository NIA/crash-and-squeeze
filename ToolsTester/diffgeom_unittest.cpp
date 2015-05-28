#include "tools_tester.h"
#include "Math/diffgeom.h"

TEST(DiffGeomTest, GijkGetSet)
{
    ConnectionCoeffs gijk;
    gijk.set_all(0);

    gijk.set_at(1,1,1, 1.2);
    gijk.set_at(2,1,0, -3);

    EXPECT_EQ(1.2, gijk.get_at(1,1,1));
    EXPECT_EQ(-3,  gijk.get_at(2,1,0));
}
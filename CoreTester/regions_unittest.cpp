#include "core_tester.h"
#include "Core/regions.h"

TEST(RegionsTest, EmptyRegion)
{
    const EmptyRegion region_instance;
    const IRegion & region = region_instance;

    Vector point(0,1,2);
    EXPECT_FALSE( region.contains(point) );
}

TEST(RegionsTest, SphericalRegionProperties)
{
    const Vector center(0,0,0);
    const Real radius = 3;
    const SphericalRegion region( center, radius );
    
    EXPECT_EQ( center, region.get_center() );
    EXPECT_EQ( radius, region.get_radius() );
}

TEST(RegionsTest, SphericalRegionNegativeRadius)
{
    suppress_warnings();
    const SphericalRegion region( Vector::ZERO, -2 );
    EXPECT_EQ( 0, region.get_radius() );
    unsuppress_warnings();
}

TEST(RegionsTest, SphericalRegionContainsPoint)
{
    const SphericalRegion region_instance( Vector::ZERO, 2 );
    const IRegion & region = region_instance;

    EXPECT_TRUE( region.contains( Vector(0.5, 0.5, 0.5) ) ); // inside
    EXPECT_TRUE( region.contains( Vector(2, 0, 0) ) );       // border
    EXPECT_FALSE( region.contains( Vector(2, 2, 2) ) );      // outside
}

TEST(RegionsTest, SphericalRegionMove)
{
    SphericalRegion region_instance( Vector(1,2,3), 2 );
    IRegion & region = region_instance;

    region.move(Vector(1,2,1));
    EXPECT_EQ(Vector(2,4,4), region_instance.get_center());
    EXPECT_EQ(2, region_instance.get_radius());
}

TEST(RegionsTest, CylindricalRegionProperties)
{
    const Vector bottom(1,1,1);
    const Vector top(2,2,2);
    const Real radius = 1;
    const CylindricalRegion region(top, bottom, radius);

    EXPECT_EQ( top, region.get_top_center() );
    EXPECT_EQ( bottom, region.get_bottom_center() );
    EXPECT_EQ( top - bottom, region.get_axis() );
    EXPECT_EQ( (top+bottom)/2, region.get_center());
    EXPECT_EQ( radius, region.get_radius() );
}

TEST(RegionsTest, CylindricalRegionBadAxis)
{
    set_tester_err_callback();
    const Vector point(1,43,3);
    EXPECT_THROW( CylindricalRegion(point, point, 2), CoreTesterException );
    unset_tester_err_callback();
}

TEST(RegionsTest, CylindricalRegionNegativeRadius)
{
    suppress_warnings();
    const CylindricalRegion region( Vector::ZERO, Vector(1,1,1), -2 );
    EXPECT_EQ( 0, region.get_radius() );
    unsuppress_warnings();
}

TEST(RegionsTest, CylindricalRegionContainsPoint)
{
    const CylindricalRegion region_instance( Vector(0,0,0), Vector(0,0,1), 2 );
    const IRegion & region = region_instance;

    EXPECT_TRUE( region.contains( Vector(0.5, 0.5, 0.5) ) ); // inside
    EXPECT_TRUE( region.contains( Vector(0.1, 0.7, 0) ) );   // bottom
    EXPECT_TRUE( region.contains( Vector(2, 0, 0) ) );       // edge
    EXPECT_TRUE( region.contains( Vector(0, 2, 0.5) ) );     // surface
    EXPECT_FALSE( region.contains( Vector(2, 2, 2) ) );      // outside
}

TEST(RegionsTest, CylindricalRegionMove)
{
    CylindricalRegion region_instance( Vector(1,2,4), Vector(1,2,3), 2 );
    IRegion & region = region_instance;

    region.move(Vector(1,2,1));
    EXPECT_EQ(Vector(2,4,5), region_instance.get_top_center());
    EXPECT_EQ(Vector(2,4,4), region_instance.get_bottom_center());
    EXPECT_EQ(2, region_instance.get_radius());
}

TEST(RegionsTest, BoxRegionProperties)
{
    Vector min(1,2,2);
    Vector max(3,3,3);
    const BoxRegion region(min, max);
    
    EXPECT_EQ( min, region.get_min_corner() );
    EXPECT_EQ( max, region.get_max_corner() );
    EXPECT_EQ( 2, region.get_dimension(0) );
    EXPECT_EQ( 1, region.get_dimension(1) );
    EXPECT_EQ( 1, region.get_dimension(2) );
    EXPECT_EQ( Vector(2, 2.5, 2.5), region.get_center() );
}

TEST(RegionsTest, BoxRegionLittleMax)
{
    suppress_warnings();
    Vector min(1,2,2);
    Vector max(3,3,3);
    
    // invert order: give max as min and vice versa
    const BoxRegion region(max, min);
    
    EXPECT_EQ( region.get_max_corner(), region.get_min_corner() );
    unsuppress_warnings();
}

TEST(RegionsTest, BoxRegionContainsPoint)
{
    const BoxRegion region_instance( Vector(0,0,0), Vector(1,1,1) );
    const IRegion & region = region_instance;

    EXPECT_TRUE( region.contains( Vector(0.5, 0.5, 0.5) ) ); // inside
    EXPECT_TRUE( region.contains( Vector(0.1, 1, 0.1) ) );   // bottom
    EXPECT_TRUE( region.contains( Vector(0.3, 0, 0) ) );     // edge
    EXPECT_TRUE( region.contains( Vector(1, 1, 1) ) );       // corner
    EXPECT_FALSE( region.contains( Vector(-1, -1, -1) ) );   // outside
}

TEST(RegionsTest, BoxRegionMove)
{
    BoxRegion region_instance( Vector(0,0,0), Vector(1,1,1) );
    IRegion & region = region_instance;

    region.move(Vector(1,2,1));
    EXPECT_EQ(Vector(1,2,1), region_instance.get_min_corner());
    EXPECT_EQ(Vector(2,3,2), region_instance.get_max_corner());
}

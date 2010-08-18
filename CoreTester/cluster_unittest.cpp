#include "core_tester.h"
#include "cluster.h"
#include "physical_vertex.h"

// Floating point functions tests

TEST(ClusterTest, Creation)
{
    Cluster c;
    EXPECT_EQ( 0, c.get_vertices_num() );
    EXPECT_EQ( 0, c.get_total_mass() );
    EXPECT_EQ( c.get_initial_center_of_mass(), c.get_center_of_mass() );
}

TEST(ClusterTest, AddOne)
{
    Cluster c;
    PhysicalVertex v;
    v.mass = 12;
    v.pos = Vector(1,2,3);
    c.add_vertex(3, v);

    EXPECT_EQ( 1, c.get_vertices_num() );
    EXPECT_EQ( v.mass, c.get_total_mass() );
    EXPECT_EQ( v.pos, c.get_initial_center_of_mass() );
    EXPECT_EQ( v.pos, c.get_center_of_mass() );
    EXPECT_EQ( 3, c.get_vertex_index(0) );
    EXPECT_EQ( Vector(0,0,0), c.get_initial_vertex_offset_position(0) );
}

TEST(ClusterTest, AddTwo)
{
    Cluster c;
    PhysicalVertex u, v;
    u.mass = 10;
    v.mass = 20;
    u.pos = Vector(1,1,1);
    v.pos = Vector(4,7,10);
    c.add_vertex(1, u);
    c.add_vertex(0, v);

    EXPECT_EQ( 2, c.get_vertices_num() );
    EXPECT_EQ( 30, c.get_total_mass() );
    EXPECT_EQ( Vector(3, 5, 7), c.get_initial_center_of_mass() );
    EXPECT_EQ( Vector(3, 5, 7), c.get_center_of_mass() );
    EXPECT_EQ( 1, c.get_vertex_index(0) );
    EXPECT_EQ( 0, c.get_vertex_index(1) );
    EXPECT_EQ( Vector(-2,-4,-6), c.get_initial_vertex_offset_position(0) );
    EXPECT_EQ( Vector(1,2,3), c.get_initial_vertex_offset_position(1) );
}

TEST(ClusterTest, AddSeveral)
{
    Cluster c;
    PhysicalVertex u, v, w, z;
    u.mass = v.mass = w.mass = z.mass = 1;
    u.pos = Vector(0, 0, 0);
    v.pos = Vector(0, 2, 0);
    w.pos = Vector(2, 0, 0);
    z.pos = Vector(2, 2, 0);
    c.add_vertex(0, u);
    c.add_vertex(1, v);
    c.add_vertex(2, w);
    c.add_vertex(3, z);

    EXPECT_EQ( 4, c.get_vertices_num() );
    EXPECT_EQ( 4, c.get_total_mass() );
    EXPECT_EQ( Vector(1, 1, 0), c.get_initial_center_of_mass() );
    EXPECT_EQ( Vector(1, 1, 0), c.get_center_of_mass() );
    EXPECT_EQ( 0, c.get_vertex_index(0) );
    EXPECT_EQ( 1, c.get_vertex_index(1) );
    EXPECT_EQ( 2, c.get_vertex_index(2) );
    EXPECT_EQ( 3, c.get_vertex_index(3) );
    EXPECT_EQ( Vector(-1,-1,0), c.get_initial_vertex_offset_position(0) );
    EXPECT_EQ( Vector(-1, 1,0), c.get_initial_vertex_offset_position(1) );
    EXPECT_EQ( Vector( 1,-1,0), c.get_initial_vertex_offset_position(2) );
    EXPECT_EQ( Vector( 1, 1,0), c.get_initial_vertex_offset_position(3) );
}

TEST(ClusterTest, AddMany)
{
    Cluster c;
    const int MANY = 222; // should be more than Cluster::INITIAL_ALLOCATED_VERTICES_NUM
    Real mass = 0;
    for(int i = 0; i < MANY; ++i)
    {
        PhysicalVertex v;
        v.mass = abs( static_cast<Real>(MANY-1)/2 - i );
        mass += v.mass;
        v.pos = Vector(0, 0, i);
        c.add_vertex(i, v);
    }
    
    Vector cm(0, 0, static_cast<Real>(MANY-1)/2);
    
    EXPECT_EQ( MANY, c.get_vertices_num() );
    EXPECT_EQ( mass, c.get_total_mass() );
    EXPECT_EQ( cm, c.get_initial_center_of_mass() );
    EXPECT_EQ( cm, c.get_center_of_mass() );
    for(int i = 0; i < MANY; ++i)
    {
        ASSERT_EQ( i, c.get_vertex_index(i) );
        ASSERT_EQ( Vector(0, 0, i - cm[2]), c.get_initial_vertex_offset_position(i) );
    }
}


#include "core_tester.h"
#include "Core/cluster.h"
#include "Core/physical_vertex.h"

inline std::ostream &operator<<(std::ostream &stream, const Matrix &m)
{
    return stream << "{{" << m.get_at(0,0) << ", " << m.get_at(0,1) << ", " << m.get_at(0,2) << "}, "
                  <<  "{" << m.get_at(1,0) << ", " << m.get_at(1,1) << ", " << m.get_at(1,2) << "}, "
                  <<  "{" << m.get_at(2,0) << ", " << m.get_at(2,1) << ", " << m.get_at(2,2) << "}}";
}

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
    PhysicalVertex v(Vector(1,2,3), 12);
    c.add_vertex(v);
    EXPECT_EQ( 1, v.get_including_clusters_num() );
    
    c.compute_initial_characteristics();

    EXPECT_EQ( 1, c.get_vertices_num() );
    EXPECT_EQ( v.get_mass(), c.get_total_mass() );
    EXPECT_EQ( v.get_pos(), c.get_initial_center_of_mass() );
    EXPECT_EQ( v.get_pos(), c.get_center_of_mass() );
    EXPECT_EQ( &v, &c.get_vertex(0) );
    EXPECT_EQ( Vector(0,0,0), c.get_equilibrium_position(0) );
}

TEST(ClusterTest, AddTwo)
{
    Cluster c;
    PhysicalVertex u(Vector(1,1,1), 10);
    PhysicalVertex v(Vector(4,7,10), 20);
    c.add_vertex(u);
    c.add_vertex(v);

    EXPECT_EQ( 1, u.get_including_clusters_num() );
    EXPECT_EQ( 1, v.get_including_clusters_num() );

    c.compute_initial_characteristics();

    EXPECT_EQ( 2, c.get_vertices_num() );
    EXPECT_EQ( 30, c.get_total_mass() );
    EXPECT_EQ( Vector(3, 5, 7), c.get_initial_center_of_mass() );
    EXPECT_EQ( Vector(3, 5, 7), c.get_center_of_mass() );
    EXPECT_EQ( &u, &c.get_vertex(0) );
    EXPECT_EQ( &v, &c.get_vertex(1) );
    EXPECT_EQ( Vector(-2,-4,-6), c.get_equilibrium_position(0) );
    EXPECT_EQ( Vector(1,2,3), c.get_equilibrium_position(1) );
}

TEST(ClusterTest, AddSeveral)
{
    Cluster c;
    PhysicalVertex u(Vector(0, 0, 0), 1);
    PhysicalVertex v(Vector(0, 2, 0), 1);
    PhysicalVertex w(Vector(2, 0, 0), 1);
    PhysicalVertex z(Vector(2, 2, 0), 1);
    c.add_vertex(u);
    c.add_vertex(v);
    c.add_vertex(w);
    c.add_vertex(z);
    c.compute_initial_characteristics();

    EXPECT_EQ( 4, c.get_vertices_num() );
    EXPECT_EQ( 4, c.get_total_mass() );
    EXPECT_EQ( Vector(1, 1, 0), c.get_initial_center_of_mass() );
    EXPECT_EQ( Vector(1, 1, 0), c.get_center_of_mass() );
    EXPECT_EQ( &u, &c.get_vertex(0) );
    EXPECT_EQ( &v, &c.get_vertex(1) );
    EXPECT_EQ( &w, &c.get_vertex(2) );
    EXPECT_EQ( &z, &c.get_vertex(3) );
    EXPECT_EQ( Vector(-1,-1,0), c.get_equilibrium_position(0) );
    EXPECT_EQ( Vector(-1, 1,0), c.get_equilibrium_position(1) );
    EXPECT_EQ( Vector( 1,-1,0), c.get_equilibrium_position(2) );
    EXPECT_EQ( Vector( 1, 1,0), c.get_equilibrium_position(3) );
}

TEST(ClusterTest, AddToMoreThanOne)
{
    Cluster c1;
    Cluster c2;
    PhysicalVertex v;

    c1.add_vertex(v);
    c2.add_vertex(v);

    EXPECT_EQ(2, v.get_including_clusters_num());
}

TEST(ClusterTest, AddMany)
{
    Cluster c;
    const int MANY = 2*Cluster::INITIAL_ALLOCATED_VERTICES_NUM+3;
    PhysicalVertex v[MANY];
    
    Real total_mass = 0;
    for(int i = 0; i < MANY; ++i)
    {
        v[i] = PhysicalVertex(Vector(0, 0, i),
                               abs( static_cast<Real>(MANY-1)/2 - i ) );
        c.add_vertex(v[i]);
        total_mass += v[i].get_mass();
    }
    c.compute_initial_characteristics();
    
    Vector cm(0, 0, static_cast<Real>(MANY-1)/2);
    
    EXPECT_EQ( MANY, c.get_vertices_num() );
    EXPECT_EQ( total_mass, c.get_total_mass() );
    EXPECT_EQ( cm, c.get_initial_center_of_mass() );
    EXPECT_EQ( cm, c.get_center_of_mass() );
    for(int i = 0; i < MANY; ++i)
    {
        EXPECT_EQ( &v[i], &c.get_vertex(i) );
        ASSERT_EQ( Vector(0, 0, i - cm[2]), c.get_equilibrium_position(i) );
    }
}

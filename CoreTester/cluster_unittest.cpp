#include "core_tester.h"
#include "Core/cluster.h"
#include "Core/physical_vertex.h"

// A macro to do EXPECT_EQ on cluster.get_equilibrium_offset_pos 
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
    #define EXPECT_EQ_EQUIL(v1, v2) EXPECT_EQ(TriVector(v1), v2) 
    #define ASSERT_EQ_EQUIL(v1, v2) ASSERT_EQ(TriVector(v1), v2)

    inline std::ostream &operator<<(std::ostream &stream, const TriVector &vector)
    {
        return stream << "[" << vector.vectors[0] << ", " << vector.vectors[1] << ", " << vector.vectors[2] << "]";
    }
#else
    #define EXPECT_EQ_EQUIL(v1, v2) EXPECT_EQ(v1, v2)
    #define ASSERT_EQ_EQUIL(v1, v2) ASSERT_EQ(v1, v2) 
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

inline std::ostream &operator<<(std::ostream &stream, const Matrix &m)
{
    return stream << "{{" << m.get_at(0,0) << ", " << m.get_at(0,1) << ", " << m.get_at(0,2) << "}, "
                  <<  "{" << m.get_at(1,0) << ", " << m.get_at(1,1) << ", " << m.get_at(1,2) << "}, "
                  <<  "{" << m.get_at(2,0) << ", " << m.get_at(2,1) << ", " << m.get_at(2,2) << "}}";
}

TEST(ClusterTest, Creation)
{
    Cluster c;
    EXPECT_EQ( 0, c.get_physical_vertices_num() );
    EXPECT_EQ( 0, c.get_graphical_vertices_num() );
    EXPECT_EQ( 0, c.get_total_mass() );
}

TEST(ClusterTest, AddOne)
{
    Cluster c;
    PhysicalVertex v(Vector(1,2,3), 12);
    c.add_physical_vertex(v);
    
    suppress_warnings();
    c.compute_initial_characteristics();
    unsuppress_warnings();

    EXPECT_EQ( 1, c.get_physical_vertices_num() );
    EXPECT_EQ( v.get_mass(), c.get_total_mass() );
    EXPECT_EQ( v.get_pos(), c.get_center_of_mass() );
    EXPECT_EQ( &v, &c.get_physical_vertex(0) );
    EXPECT_EQ_EQUIL( Vector(0,0,0), c.get_equilibrium_offset_pos(0) );
}

TEST(ClusterTest, AddTwo)
{
    Cluster c;
    PhysicalVertex u(Vector(1,1,1), 10);
    PhysicalVertex v(Vector(4,7,10), 20);
    c.add_physical_vertex(u);
    c.add_physical_vertex(v);

    suppress_warnings();
    c.compute_initial_characteristics();
    unsuppress_warnings();

    EXPECT_EQ( 2, c.get_physical_vertices_num() );
    EXPECT_EQ( 30, c.get_total_mass() );
    EXPECT_EQ( Vector(3, 5, 7), c.get_center_of_mass() );
    EXPECT_EQ( &u, &c.get_physical_vertex(0) );
    EXPECT_EQ( &v, &c.get_physical_vertex(1) );
    EXPECT_EQ_EQUIL( Vector(-2,-4,-6), c.get_equilibrium_offset_pos(0) );
    EXPECT_EQ_EQUIL( Vector( 1, 2, 3), c.get_equilibrium_offset_pos(1) );
}

TEST(ClusterTest, AddSeveral)
{
    Cluster c;
    PhysicalVertex u(Vector(0, 0, 0), 1);
    PhysicalVertex v(Vector(0, 2, 0), 1);
    PhysicalVertex w(Vector(2, 0, 0), 1);
    PhysicalVertex z(Vector(2, 2, 0), 1);
    c.add_physical_vertex(u);
    c.add_physical_vertex(v);
    c.add_physical_vertex(w);
    c.add_physical_vertex(z);
    suppress_warnings();
    c.compute_initial_characteristics();
    unsuppress_warnings();

    EXPECT_EQ( 4, c.get_physical_vertices_num() );
    EXPECT_EQ( 4, c.get_total_mass() );
    EXPECT_EQ( Vector(1, 1, 0), c.get_center_of_mass() );
    EXPECT_EQ( &u, &c.get_physical_vertex(0) );
    EXPECT_EQ( &v, &c.get_physical_vertex(1) );
    EXPECT_EQ( &w, &c.get_physical_vertex(2) );
    EXPECT_EQ( &z, &c.get_physical_vertex(3) );
    EXPECT_EQ_EQUIL( Vector(-1,-1,0), c.get_equilibrium_offset_pos(0) );
    EXPECT_EQ_EQUIL( Vector(-1, 1,0), c.get_equilibrium_offset_pos(1) );
    EXPECT_EQ_EQUIL( Vector( 1,-1,0), c.get_equilibrium_offset_pos(2) );
    EXPECT_EQ_EQUIL( Vector( 1, 1,0), c.get_equilibrium_offset_pos(3) );
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
        c.add_physical_vertex(v[i]);
        total_mass += v[i].get_mass();
    }
    suppress_warnings();
    c.compute_initial_characteristics();
    unsuppress_warnings();
    
    Vector cm(0, 0, static_cast<Real>(MANY-1)/2);
    
    EXPECT_EQ( MANY, c.get_physical_vertices_num() );
    EXPECT_EQ( total_mass, c.get_total_mass() );
    EXPECT_EQ( cm, c.get_center_of_mass() );
    for(int i = 0; i < MANY; ++i)
    {
        EXPECT_EQ( &v[i], &c.get_physical_vertex(i) );
        ASSERT_EQ_EQUIL( Vector(0, 0, i - cm[2]), c.get_equilibrium_offset_pos(i) );
    }
}

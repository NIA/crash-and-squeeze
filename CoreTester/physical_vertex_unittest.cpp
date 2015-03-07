#include "core_tester.h"
#include "Core/physical_vertex.h"

// TODO: Test for integrate_... methods (needed?)

TEST(PhysicalVertexTest, Properties)
{
    const Vector pos(1,1,1);
    Real mass = 4;
    const Vector velocity(0,0,1);

    PhysicalVertex vertex(pos, mass, velocity);
    const PhysicalVertex &v = vertex;
    EXPECT_EQ( pos, v.get_pos() );
    EXPECT_EQ( mass, v.get_mass() );
    EXPECT_EQ( velocity, v.get_velocity() );

    Vector body_angular_velocity(0, 0, 1);
    EXPECT_EQ( Vector(0, 1, 0), v.angular_velocity_to_linear( body_angular_velocity, pos - Vector(1, 0, 0) ) );

    const Vector velocity_correction(0, 0, -3);
    vertex.add_to_velocity( velocity_correction );
    EXPECT_EQ( velocity + velocity_correction, v.get_velocity() );
}

TEST(PhysicalVertexTest, InterfaceForCluster)
{
    const Vector pos(1,1,1);
    Real mass = 4;
    const Vector velocity(0,0,1);
    const Vector addition(0,0,2);

    PhysicalVertex vertex(pos, mass, velocity);
    const PhysicalVertex &v = vertex;
    
    vertex.include_to_one_more_cluster(0, 1);
    vertex.include_to_one_more_cluster(1, 1);
    EXPECT_EQ( 2, v.get_including_clusters_num() );
    
    // addition is divided by 2 here because vertex belons to 2 clusters
    vertex.add_to_average_velocity_addition(addition, 0);
    vertex.add_to_average_velocity_addition(Vector::ZERO, 1);
    vertex.compute_velocity_addition();
    EXPECT_EQ( addition/2, v.get_velocity_addition() );
}

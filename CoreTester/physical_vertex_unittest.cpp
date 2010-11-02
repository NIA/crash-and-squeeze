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

    EXPECT_EQ( mass*velocity, v.get_linear_momentum() );
    EXPECT_EQ( mass*Vector(0, -1, 0), v.get_angular_momentum( pos - Vector(1, 0, 0) ) );

    Vector body_angular_velocity(0, 0, 1);
    EXPECT_EQ( Vector(0, 1, 0), v.angular_velocity_to_linear( body_angular_velocity, pos - Vector(1, 0, 0) ) );
}

TEST(PhysicalVertexTest, InterfaceForCluster)
{
    const Vector pos(1,1,1);
    Real mass = 4;
    const Vector velocity(0,0,1);
    const Vector addition_before(0,0,2);
    Vector addition = addition_before;

    PhysicalVertex vertex(pos, mass, velocity);
    const PhysicalVertex &v = vertex;
    
    vertex.include_to_one_more_cluster();
    vertex.include_to_one_more_cluster();
    EXPECT_EQ( 2, v.get_including_clusters_num() );
    
    vertex.set_nearest_cluster_index(5);
    EXPECT_EQ( 5, v.get_nearest_cluster_index() );
    
    // addition is divided by 2 here because vertex belons to 2 clusters
    vertex.add_to_velocity_addition(addition);
    EXPECT_EQ( addition_before/2, addition );
    EXPECT_EQ( mass*addition, v.get_linear_momentum_addition() );
    EXPECT_EQ( mass*Vector(0, -1, 0), v.get_angular_momentum_addition( pos - Vector(1, 0, 0) ) );

    const Vector addition_correction(0, 0, -1/mass);
    // correction is multiplied by mass and added to current velocity addition
    vertex.correct_velocity_addition( addition_correction );
    EXPECT_EQ( Vector::ZERO, v.get_linear_momentum_addition() );
}

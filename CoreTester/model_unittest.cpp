#include "core_tester.h"
#include "Core/model.h"
#include "Core/cluster.h"
#include "Core/physical_vertex.h"

namespace
{
    const int  CLUSTERS_BY_AXES[VECTOR_SIZE] = {1, 1, 1};
    const int  TOTAL_CLUSTERS_NUM = CLUSTERS_BY_AXES[0]*CLUSTERS_BY_AXES[1]*CLUSTERS_BY_AXES[2];
    const Real PADDING = 0.6;

    struct TestVertex1
    {
        VertexFloat x, y, z;
    } vertices1[] =
        {
            { 1,    2,    3},
            {-4, 5.3f,    8},
            { 0,    0,    0},
            { 4,   -1, 0.1f}

        };
    const int VERTICES1_NUM = sizeof(vertices1)/sizeof(vertices1[0]);

    struct TestVertex2
    {
        int dummy;
        VertexFloat x, y, z;
    } vertices2[] = 
        {
            { 0xBADF00D, 6, 5, 4},
            {0xDEADBEEF, 7, 2, 1},
            { 0xBADF00D, 0, 0, 0},
            {0xDEADBEEF, 1, 1, 1},
        };

    template<class V> Vector get_pos(V vertex) { return Vector(); }
    template<> Vector get_pos<TestVertex1>(TestVertex1 v) { return Vector(v.x, v.y, v.z); }
    template<> Vector get_pos<TestVertex2>(TestVertex2 v) { return Vector(v.x, v.y, v.z); }

    template<int SIZE,class V>
    void test_creation(V (&vertices)[SIZE], const VertexInfo &vi, const MassFloat *masses, MassFloat constant_mass = 0)
    {
        Model m(vertices, SIZE, vi, CLUSTERS_BY_AXES, PADDING, masses, constant_mass);
        int vnum = m.get_vertices_num();
        
        int cnum = m.get_clusters_num();
        
        ASSERT_EQ(SIZE, vnum);
        ASSERT_EQ(TOTAL_CLUSTERS_NUM, cnum);
        
        for(int i = 0; i < vnum; ++i)
        {
            const PhysicalVertex & v = m.get_vertex(i);
            ASSERT_EQ(get_pos(vertices[i]), v.get_pos());
            if( NULL == masses )
                ASSERT_EQ(constant_mass, v.get_mass());
            else
                ASSERT_EQ(masses[i], v.get_mass());

            ASSERT_GE( v.get_nearest_cluster_index(), 0 );
            ASSERT_LT( v.get_nearest_cluster_index(), cnum );
        }

        for(int i = 0; i < cnum; ++i)
        {
            ASSERT_TRUE(m.get_cluster(i).is_valid());
        }
    }
};

// TODO: fixtures
// TODO: typed tests

TEST(ModelTest, Creation1)
{
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    test_creation(vertices1, vi1, NULL, 4);
}

TEST(ModelTest, Creation2)
{
    VertexInfo vi2( sizeof(vertices2[0]), sizeof(vertices2[0].dummy) );
    test_creation(vertices2, vi2, NULL, 4);
}

TEST(ModelTest, Creation1WithMasses)
{
    MassFloat masses[VERTICES1_NUM] = { 26, 0.00004, 4, 8 };
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    test_creation(vertices1, vi1, masses);
}

TEST(ModelTest, StepComputationShouldNotFail)
{
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    Model m(vertices1, VERTICES1_NUM, vi1, CLUSTERS_BY_AXES, PADDING, NULL, 4);
    
    const int FORCES_NUM = 10;
    ForcesArray forces(FORCES_NUM);
    PlaneForce f;
    for(int i = 0; i < FORCES_NUM; ++i)
        forces.push_back( &f );

    EXPECT_NO_THROW( m.compute_next_step(forces) );
    // should work with no forces
    ForcesArray empty;
    EXPECT_NO_THROW( m.compute_next_step(empty) );
}

TEST(ModelTest, BadForces)
{
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    Model m(vertices1, VERTICES1_NUM, vi1, CLUSTERS_BY_AXES, PADDING, NULL, 4);
    set_tester_err_callback();
    ForcesArray bad;
    bad.push_back(NULL);
    EXPECT_THROW( m.compute_next_step(bad), CoreTesterException );
    unset_tester_err_callback();
}

void check_axis_indices(int index, int axis_indices[VECTOR_SIZE], int clusters_by_axes[VECTOR_SIZE])
{
    EXPECT_EQ( index, Model::axis_indices_to_index(axis_indices, clusters_by_axes) );
    
    int result[VECTOR_SIZE];
    Model::index_to_axis_indices(index, clusters_by_axes, result);
    for(int i = 0; i < VECTOR_SIZE; ++i)
    {
        EXPECT_EQ( axis_indices[i], result[i] ) << "result[" << i << "] incorrect";
    }
}

TEST(ModelTest, AxisIndicesTrivial)
{
    int clusters_by_axes[VECTOR_SIZE] = { 1, 1, 1 };
    int index = 0;
    int axis_indices[VECTOR_SIZE] = { 0, 0, 0 };

    check_axis_indices(index, axis_indices, clusters_by_axes);
}

TEST(ModelTest, AxisIndicesCenter)
{
    int clusters_by_axes[VECTOR_SIZE] = { 3, 5, 7 };
    int index = (3*5*7 - 1)/2;
    int axis_indices[VECTOR_SIZE] = { 1, 2, 3 };

    check_axis_indices(index, axis_indices, clusters_by_axes);
}

TEST(ModelTest, AxisIndicesEnd)
{
    int clusters_by_axes[VECTOR_SIZE] = { 2, 3, 6 };
    int index = 2*3*6 - 1;
    int axis_indices[VECTOR_SIZE] = { 2-1, 3-1, 6-1 };

    check_axis_indices(index, axis_indices, clusters_by_axes);
}

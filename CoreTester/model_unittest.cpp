#include "core_tester.h"
#include "Core/model.h"
#include "Core/physical_vertex.h"

namespace
{
    struct TestVertex1
    {
        VertexFloat x, y, z;
    } vertices1[] =
        {
            {1,2,3},
            {-4,5.3f,8},
            {4,-1,0.1f}

        };

    struct TestVertex2
    {
        int dummy;
        VertexFloat x, y, z;
    } vertices2[] = 
        {
            { 0xBADF00D, 6, 5, 4},
            {0xDEADBEEF, 3, 2, 1}
        };

    template<class V> Vector get_pos(V vertex) { return Vector(); }
    template<> Vector get_pos<TestVertex1>(TestVertex1 v) { return Vector(v.x, v.y, v.z); }
    template<> Vector get_pos<TestVertex2>(TestVertex2 v) { return Vector(v.x, v.y, v.z); }

    template<int SIZE,class V>
    void test_creation(V (&vertices)[SIZE], VertexInfo vi, const MassFloat *masses, MassFloat constant_mass = 0)
    {
        Model m(vertices, SIZE, vi, masses, constant_mass);
        int vnum = m.get_vertices_num();
        const PhysicalVertex *vv = m.get_vertices();
        /* int cnum = m.get_clusters_num();
        const Cluster *cc = m.get_clusters(); */
        
        ASSERT_EQ(SIZE, vnum);
        for(int i = 0; i < vnum; ++i)
        {
            ASSERT_EQ(get_pos(vertices[i]), vv[i].pos);
            if( NULL == masses )
                ASSERT_EQ(constant_mass, vv[i].mass);
            else
                ASSERT_EQ(masses[i], vv[i].mass);
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
    MassFloat masses[] = { 26, 0.00004 };
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    test_creation(vertices1, vi1, masses);
}

TEST(ModelTest, StepComputationShouldNotFail)
{
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    Model m(vertices1, 3, vi1, NULL, 4);
    const int FORCES_NUM = 10;
    Force * forces[FORCES_NUM];
    PlaneForce f;
    for(int i = 0; i < FORCES_NUM; ++i)
        forces[i] = &f;

    EXPECT_NO_THROW( m.compute_next_step(forces, FORCES_NUM) );
    // when there is no forces, pointer to them is allowed to be NULL
    EXPECT_NO_THROW( m.compute_next_step(NULL, 0) );
}

TEST(ModelTest, BadForces)
{
    VertexInfo vi1( sizeof(vertices1[0]), 0 );
    Model m(vertices1, 3, vi1, NULL, 4);
    set_tester_err_callback();
    EXPECT_THROW( m.compute_next_step(NULL, 45), CoreTesterException );
    unset_tester_err_callback();
}
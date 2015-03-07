#include "core_tester.h"
#include "Core/model.h"
#include "Core/cluster.h"
#include "Core/physical_vertex.h"
#include "Parallel/single_thread_prim.h"

using namespace ::CrashAndSqueeze::Parallel;

namespace
{
    const int  CLUSTERS_BY_AXES[VECTOR_SIZE] = {1, 1, 1};
    const int  TOTAL_CLUSTERS_NUM = CLUSTERS_BY_AXES[0]*CLUSTERS_BY_AXES[1]*CLUSTERS_BY_AXES[2];
    const Real PADDING = 0.6;

    struct TestVertex1
    {
        VertexFloat x, y, z;
        ClusterIndex ci[VertexInfo::CLUSTER_INDICES_NUM];
        unsigned cn;
    } vertices1[] =
        {
            { 1,    2,    3},
            {-4, 5.3f,    8},
            { 0,    0,    0},
            { 4,   -1, 0.1f},
            { 5,   1,  0.1f}
        };
    const int VERTICES1_NUM = sizeof(vertices1)/sizeof(vertices1[0]);

    TestVertex1 stick[] =
        {
            {     0,     0, 0},
            { 0.01f,     0, 0},
            {     0, 0.01f, 0},
            { 0.01f, 0.01f, 0},
            {     0,     0, 2},
            { 0.01f,     0, 2},
            {     0, 0.01f, 2},
            { 0.01f, 0.01f, 2},
    };
    const int STICK_VERTICES_NUM = sizeof(stick)/sizeof(stick[0]);

    struct TestVertex2
    {
        int dummy;
        VertexFloat x, y, z;
        ClusterIndex ci[VertexInfo::CLUSTER_INDICES_NUM];
        unsigned cn;
    } vertices2[] = 
        {
            { 0xBADF00D, 6, 5, 4},
            {0xDEADBEEF, 7, 2, 1},
            { 0xBADF00D, 0, 0, 0},
            {0xDEADBEEF, 1, 1, 1},
            { 0xBADF00D, -1, -1, -1},
        };

    template<class V> Vector get_pos(V v) { return Vector(v.x, v.y, v.z); }

    class MyVelocitiesChangeCallback : public VelocitiesChangedCallback
    {
    private:
        Vector linear;
        Vector angular;
    public:
        virtual void invoke(const Vector &linear_velocity_change,
                            const Vector &angular_velocity_change)
        {
            linear = linear_velocity_change;
            angular = angular_velocity_change;
        }

        const Vector & get_linear_velocity_change() { return linear; }
        const Vector & get_angular_velocity_change() { return angular; }
    };
}

class ModelTest : public ::testing::Test
{
protected:
    IndexArray frame;
    static const int FRAME_SIZE = 4;

    VertexInfo vi1;
    VertexInfo vi2;

    SingleThreadFactory prim_factory;

    Real dt;

    MyVelocitiesChangeCallback vcb;

    ModelTest()
        : vi1( sizeof(vertices1[0]), 0, 3*sizeof(vertices1[0].x),  sizeof(vertices1[0]) - sizeof(vertices1[0].cn)),
          vi2( sizeof(vertices2[0]), sizeof(vertices2[0].dummy), sizeof(vertices2[0].dummy) + 3*sizeof(vertices2[0].x),  sizeof(vertices2[0]) - sizeof(vertices2[0].cn) )
    {}

    virtual void SetUp()
    {
        for(int i = 0; i < FRAME_SIZE; ++i)
            frame.push_back(i);

        dt = 0.01;

        set_tester_err_callback();
    }
    
    virtual void TearDown()
    {
        unset_tester_err_callback();
    }

    template<int SIZE,class V>
    void test_creation(V (&vertices)[SIZE], const VertexInfo &vi, MassFloat constant_mass = 1, const MassFloat *masses = NULL)
    {
        Model m(vertices, SIZE, vi, vertices, SIZE, vi, CLUSTERS_BY_AXES, PADDING, constant_mass, masses, &prim_factory);
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

            ASSERT_GE( v.get_including_clusters_num(), 0 );
        }

#if (! CAS_QUADRATIC_EXTENSIONS_ENABLED)
        // TODO: fix this test for CAS_QUADRATIC_EXTENSIONS_ENABLED. Currently just ignore this test
        for(int i = 0; i < cnum; ++i)
        {
            ASSERT_TRUE(m.get_cluster(i).is_valid());
        }
#endif // (! CAS_QUADRATIC_EXTENSIONS_ENABLED)
    }

    void compute_next_step(Model &m, const ForcesArray &forces, VelocitiesChangedCallback & vcb)
    {
        m.wait_for_step();
        m.prepare_tasks(forces, dt, &vcb);
        m.wait_for_tasks();
        while( false != m.complete_next_task() ) {}
        m.wait_for_clusters();
        m.react_to_events();
        m.wait_for_step();
    }
};

// TODO: typed tests

TEST_F(ModelTest, Creation1)
{
    test_creation(vertices1, vi1, 4);
}

TEST_F(ModelTest, Creation2)
{
    test_creation(vertices2, vi2, 4);
}

TEST_F(ModelTest, Creation1WithMasses)
{
    MassFloat masses[VERTICES1_NUM] = { 26, 0.00004, 4, 8, 3 };
    test_creation(vertices1, vi1, 0, masses);
}

TEST_F(ModelTest, StepComputationShouldNotFail)
{
    Model m(vertices1, VERTICES1_NUM, vi1, vertices1, VERTICES1_NUM, vi1, CLUSTERS_BY_AXES, PADDING, 4, NULL, &prim_factory);
    
    const int FORCES_NUM = 10;
    ForcesArray forces(FORCES_NUM);
    PlaneForce f;
    for(int i = 0; i < FORCES_NUM; ++i)
        forces.push_back( &f );

    EXPECT_NO_THROW( compute_next_step(m, forces, vcb) );
    
    // should work with no forces and no VelocitiesChangedCallback
    ForcesArray empty;
    EXPECT_NO_THROW( compute_next_step(m, empty, *reinterpret_cast<VelocitiesChangedCallback*>(NULL)) );
    
    // should work with frame
    m.set_frame(frame);
    EXPECT_NO_THROW( compute_next_step(m, forces, vcb) );
}

TEST_F(ModelTest, BadForces)
{
    Model m(vertices1, VERTICES1_NUM, vi1, vertices1, VERTICES1_NUM, vi1, CLUSTERS_BY_AXES, PADDING, 4, NULL, &prim_factory);
    ForcesArray bad;
    bad.push_back(NULL);
    EXPECT_THROW( compute_next_step(m, bad, vcb), CoreTesterException );
}

bool vectors_almost_equal(const Vector &v1, const Vector &v2, Real accuracy)
{
    for(int i = 0; i < VECTOR_SIZE; ++i)
    {
        if( ! equal(v1[i], v2[i], accuracy) )
            return false;
    }
    return true;
}

TEST_F(ModelTest, Hit)
{
    Model m(stick, STICK_VERTICES_NUM, vi1, stick, STICK_VERTICES_NUM, vi1, CLUSTERS_BY_AXES, PADDING, 4, NULL, &prim_factory);

    const Vector hit_velocity(0, 1, 0);
    const Vector exp_lin_velocity(0, 0.5, 0);
    const Vector exp_ang_velocity(0.5, 0, 0);

    ForcesArray empty(0);
    m.hit( SphericalRegion( Vector(0,0,0), 0.1 ), hit_velocity);
    EXPECT_NO_THROW( compute_next_step(m, empty, vcb) );

    EXPECT_EQ( exp_lin_velocity, vcb.get_linear_velocity_change() );
    EXPECT_TRUE( vectors_almost_equal(exp_ang_velocity, vcb.get_angular_velocity_change(), 0.001) ) << "expected " << exp_ang_velocity << ", got " << vcb.get_angular_velocity_change();
}

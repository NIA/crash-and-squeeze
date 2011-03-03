#include "core_tester.h"
#include "Core/vertex_info.h"

TEST(VertexInfoTest, Creation1)
{
    const VertexInfo vi(28, 3, 20, 16);
    EXPECT_EQ(28, vi.get_vertex_size());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(0, vi.get_vectors_num());
    EXPECT_EQ(3, vi.get_point_offset(0));
    EXPECT_EQ(20, vi.get_cluster_indices_offset());
}

TEST(VertexInfoTest, Creation2)
{
    const VertexInfo vi(38, 3, 16, true, 30, 26);
    EXPECT_EQ(38, vi.get_vertex_size());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(1, vi.get_vectors_num());
    EXPECT_EQ(3, vi.get_point_offset(0));
    EXPECT_EQ(16, vi.get_vector_offset(0));
    EXPECT_EQ(30, vi.get_cluster_indices_offset());
}

TEST(VertexInfoTest, BadSizeCreation)
{
    set_tester_err_callback();
    EXPECT_THROW( VertexInfo(-10, 1, 1, 1), CoreTesterException );
    EXPECT_THROW( VertexInfo(-10, 1, 1, true, 1, 1), CoreTesterException );
    unset_tester_err_callback();
}

TEST(VertexInfoTest, BadOffsetsCreation)
{
    set_tester_err_callback();
    EXPECT_THROW( VertexInfo(28, -1, 20, 16), CoreTesterException );
    EXPECT_THROW( VertexInfo(28, 27, 20, 16), CoreTesterException ); // no room for point
    EXPECT_THROW( VertexInfo(28, 20, 27, 16), CoreTesterException ); // no room for cluster indices
    EXPECT_THROW( VertexInfo(28, 20, 16, 27), CoreTesterException ); // no room for clusters number
    EXPECT_THROW( VertexInfo(28, 0, -1, true, 20, 16), CoreTesterException );
    EXPECT_THROW( VertexInfo(28, 0, 27, true, 20, 16), CoreTesterException ); // no room for vector
    EXPECT_THROW( VertexInfo(28, 0, 20, true, 27, 16), CoreTesterException ); // no room for cluster indices
    EXPECT_THROW( VertexInfo(28, 0, 20, true, 16, 27), CoreTesterException ); // no room for clusters number
    unset_tester_err_callback();
}

TEST(VertexInfoTest, AddPoint)
{
    VertexInfo vi(48, 0, 40, 36);
    vi.add_point(15);
    EXPECT_EQ(2, vi.get_points_num());
    EXPECT_EQ(0, vi.get_vectors_num());
    EXPECT_EQ(15, vi.get_point_offset(1));
}

TEST(VertexInfoTest, AddFirstVector)
{
    VertexInfo vi(48, 0, 40, 36);
    vi.add_vector(15, true);
    EXPECT_EQ(1, vi.get_vectors_num());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(15, vi.get_vector_offset(0));
}

TEST(VertexInfoTest, AddSecondVector)
{
    VertexInfo vi(68, 0, 15, true, 60, 56);
    vi.add_vector(30, false);
    EXPECT_EQ(2, vi.get_vectors_num());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(30, vi.get_vector_offset(1));
}

TEST(VertexInfoTest, AddBadlyManyPoints)
{
    VertexInfo vi(512, 0, 500, 496);
    for(int i = 0; i < VertexInfo::MAX_COMPONENT_NUM - 1; ++i)
    {
        vi.add_point(20*i);
    }
    set_tester_err_callback();
    EXPECT_THROW( vi.add_point(400), CoreTesterException );
    unset_tester_err_callback();
}

TEST(VertexInfoTest, AddBadlyManyVectors)
{
    VertexInfo vi(512, 450, 500, 496);
    for(int i = 0; i < VertexInfo::MAX_COMPONENT_NUM; ++i)
    {
        vi.add_vector(20*i, false);
    }
    set_tester_err_callback();
    EXPECT_THROW( vi.add_vector(400, true), CoreTesterException );
    unset_tester_err_callback();
}

#include "core_tester.h"
#include "Core/vertex_info.h"

TEST(VertexInfoTest, Creation1)
{
    const VertexInfo vi(20, 3);
    EXPECT_EQ(20, vi.get_vertex_size());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(0, vi.get_vectors_num());
    EXPECT_EQ(3, vi.get_point_offset(0));
}

TEST(VertexInfoTest, Creation2)
{
    const VertexInfo vi(30, 3, 16);
    EXPECT_EQ(30, vi.get_vertex_size());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(1, vi.get_vectors_num());
    EXPECT_EQ(3, vi.get_point_offset(0));
    EXPECT_EQ(16, vi.get_vector_offset(0));
}

TEST(VertexInfoTest, BadSizeCreation)
{
    set_tester_err_callback();
    EXPECT_THROW( VertexInfo(-10, 1), CoreTesterException );
    EXPECT_THROW( VertexInfo(-10, 1, 1), CoreTesterException );
    unset_tester_err_callback();
}

TEST(VertexInfoTest, BadOffsetsCreation)
{
    set_tester_err_callback();
    EXPECT_THROW( VertexInfo(20, -1), CoreTesterException );
    EXPECT_THROW( VertexInfo(20, 19), CoreTesterException );
    EXPECT_THROW( VertexInfo(20, 0, -1), CoreTesterException );
    EXPECT_THROW( VertexInfo(20, 0, 19), CoreTesterException );
    unset_tester_err_callback();
}

TEST(VertexInfoTest, AddPoint)
{
    VertexInfo vi(40, 0);
    vi.add_point(15);
    EXPECT_EQ(2, vi.get_points_num());
    EXPECT_EQ(0, vi.get_vectors_num());
    EXPECT_EQ(15, vi.get_point_offset(1));
}

TEST(VertexInfoTest, AddFirstVector)
{
    VertexInfo vi(40, 0);
    vi.add_vector(15);
    EXPECT_EQ(1, vi.get_vectors_num());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(15, vi.get_vector_offset(0));
}

TEST(VertexInfoTest, AddSecondVector)
{
    VertexInfo vi(60, 0, 15);
    vi.add_vector(30);
    EXPECT_EQ(2, vi.get_vectors_num());
    EXPECT_EQ(1, vi.get_points_num());
    EXPECT_EQ(30, vi.get_vector_offset(1));
}

TEST(VertexInfoTest, AddBadlyManyPoints)
{
    VertexInfo vi(512, 0);
    for(int i = 0; i < 9; ++i)
    {
        vi.add_point(20*i);
    }
    set_tester_err_callback();
    EXPECT_THROW( vi.add_point(400), CoreTesterException );
    unset_tester_err_callback();
}

TEST(VertexInfoTest, AddBadlyManyVectors)
{
    VertexInfo vi(512, 450, 0);
    for(int i = 0; i < 9; ++i)
    {
        vi.add_vector(20*i);
    }
    set_tester_err_callback();
    EXPECT_THROW( vi.add_vector(400), CoreTesterException );
    unset_tester_err_callback();
}

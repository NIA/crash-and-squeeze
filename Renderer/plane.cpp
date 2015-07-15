#include "plane.h"

using namespace DirectX; // for XMVECTOR and its load/store functions

const int PLANE_STEPS_PER_HALF_SIDE = 10;
const Index PLANE_VERTICES_COUNT = (2*PLANE_STEPS_PER_HALF_SIDE + 1)*(2*PLANE_STEPS_PER_HALF_SIDE + 1);
const DWORD PLANE_INDICES_COUNT = 2*VERTICES_PER_TRIANGLE*(2*PLANE_STEPS_PER_HALF_SIDE)*(2*PLANE_STEPS_PER_HALF_SIDE);

void plane(float3 x_direction, float3 y_direction, float3 center_, Vertex *res_vertices, Index *res_indices, const float4 & color)
{
    Index vertex = 0; // current vertex
    Index index = 0; // current index
    _STATIC_ASSERT(PLANE_STEPS_PER_HALF_SIDE != 0);

    const XMVECTOR center = XMLoadFloat3(&center_);
    const XMVECTOR x_step = XMLoadFloat3(&x_direction)/(2*PLANE_STEPS_PER_HALF_SIDE);
    const XMVECTOR y_step = XMLoadFloat3(&y_direction)/(2*PLANE_STEPS_PER_HALF_SIDE);
    const Index vertices_in_line = (2*PLANE_STEPS_PER_HALF_SIDE + 1);
    float3 normal;
    XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(x_step, y_step)));

    for( int i = -PLANE_STEPS_PER_HALF_SIDE; i <= PLANE_STEPS_PER_HALF_SIDE; ++i )
    {
        for( int j = -PLANE_STEPS_PER_HALF_SIDE; j <= PLANE_STEPS_PER_HALF_SIDE; ++j )
        {
            float3 pos;
            XMStoreFloat3(&pos, center + x_step*static_cast<float>(i) + y_step*static_cast<float>(j));
            res_vertices[vertex] = Vertex(pos, color, normal);
            if( i != -PLANE_STEPS_PER_HALF_SIDE && j != -PLANE_STEPS_PER_HALF_SIDE)
            {
                // if not first line and column
                add_triangle(vertex-1, vertex, vertex-1-vertices_in_line, res_indices, index);
                add_triangle(vertex-1-vertices_in_line, vertex, vertex-vertices_in_line, res_indices, index);
            }
            ++vertex;
        }
    }
}

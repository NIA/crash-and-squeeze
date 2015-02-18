#include "tessellate.h"

using namespace DirectX;

const unsigned TESSELATE_DEGREE = 8;
const Index TESSELATED_VERTICES_COUNT = (TESSELATE_DEGREE+1)*(TESSELATE_DEGREE+2)/2;
const Index TESSELATED_INDICES_COUNT = 3*TESSELATE_DEGREE*TESSELATE_DEGREE;

void tessellate(const Vertex *src_vertices, const Index *src_indices, DWORD src_index_offset,
                Vertex *res_vertices, Index res_vertices_offset, Index *res_indices, const float4 & color)
// Divides each side of triangle into given number of parts
// Writes data into arrays given as `res_vertices' and `res_indices',
//   assuming that there are already `res_vertices_offset' vertices before `res_vertices' pointer.
{
    _ASSERT(src_vertices != NULL);
    _ASSERT(src_indices != NULL);
    _ASSERT(res_vertices != NULL);
    _ASSERT(res_indices != NULL);
    // i1, i2 i3 are indices of source triangle vertices
    const Index i1 = src_indices[src_index_offset];
    const Index i2 = src_indices[src_index_offset + 1];
    const Index i3 = src_indices[src_index_offset + 2];
    const XMVECTOR step_down  = (XMLoadFloat3(&src_vertices[i1].pos) - XMLoadFloat3(&src_vertices[i2].pos))/TESSELATE_DEGREE;
    const XMVECTOR step_right = (XMLoadFloat3(&src_vertices[i3].pos) - XMLoadFloat3(&src_vertices[i1].pos))/TESSELATE_DEGREE;
    float3 normal;
    XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(step_down, step_right)));

    res_vertices[0] = src_vertices[i2];
    res_vertices[0].set_normal(normal);
    res_vertices[0].color = color;
    Index vertex = 1; // current vertex
    Index index = 0; // current index
    
    XMVECTOR start_pos = XMLoadFloat3(&res_vertices[0].pos);
    for( Index line = 1; line <= TESSELATE_DEGREE; ++line )
    {
        for( Index column = 0; column < line + 1; ++column ) // line #1 contains 2 vertices
        {
            float3 pos;
            XMStoreFloat3(&pos, start_pos
                                + static_cast<float>(line)*step_down
                                + static_cast<float>(column)*step_right);
            res_vertices[vertex] = Vertex( pos, color, normal);
            if( column != 0 ) // not first coumn
            {
                // add outer triangle
                add_triangle( vertex, vertex - 1, vertex - line - 1, res_indices, index, res_vertices_offset );
            }
            if( ( column != 0 ) && ( column != line ) ) // not first and not last column
            {
                // add inner triangle
                add_triangle(  vertex, vertex - line - 1, vertex - line, res_indices, index, res_vertices_offset );
            }
            ++vertex;
        }
    }
}

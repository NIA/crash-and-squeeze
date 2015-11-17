#include "cubic.h"

const Index CUBIC_X_EDGES = 6;
const Index CUBIC_Y_EDGES = 6;
const Index CUBIC_Z_EDGES = 24;

const Index CUBIC_VERTICES_COUNT = (CUBIC_X_EDGES + 1)*(CUBIC_Y_EDGES + 1)*(CUBIC_Z_EDGES + 1);

namespace
{
    const DWORD X_SEGMENTS_COUNT = CUBIC_X_EDGES;
    const DWORD XY_SEGMENTS_COUNT = X_SEGMENTS_COUNT*(CUBIC_Y_EDGES + 1)
                                  + CUBIC_Y_EDGES*(CUBIC_X_EDGES + 1);
    const DWORD XYZ_SEGMENTS_COUNT = XY_SEGMENTS_COUNT*(CUBIC_Z_EDGES + 1)
                                   + CUBIC_Z_EDGES*(CUBIC_X_EDGES + 1)*(CUBIC_Y_EDGES + 1);
}
const Index CUBIC_PRIMITIVES_COUNT = XYZ_SEGMENTS_COUNT;
const DWORD CUBIC_INDICES_COUNT = 2*CUBIC_PRIMITIVES_COUNT; // Calculated for D3DPT_LINELIST primitive type

namespace
{
    struct GENERATION_PARAMS
    {
        float x_step;
        float y_step;
        float z_step;
        float3 position;
        float4 color;

        Vertex *res_vertices;
        Index *res_indices;
    };

    // generates an adge along x-axis
    void generate_edge(float x_step, float3 position, float4 color,
                       bool connect_with_previous_edge, bool connect_with_previous_layer, bool inside,
                       Index &vertex, DWORD &index, Vertex *res_vertices, Index *res_indices)
    {
        float4 no_alpha_color(color); // same color as color, but with alpha = 0
        no_alpha_color.w = 0;
        Vertex start_vertex(position, color, float3(0,0,0));
        
        for(int i = 0; i <= CUBIC_X_EDGES; ++i)
        {
            res_vertices[vertex] = start_vertex;
            res_vertices[vertex].pos += float3(i*x_step, 0, 0);
            
            if(inside && 0 != i && CUBIC_X_EDGES != i)
            {
                res_vertices[vertex].color = no_alpha_color;
            }
            
            if( 0 != i )
            {
                res_indices[index++] = vertex - 1;
                res_indices[index++] = vertex;
            }
            if(connect_with_previous_edge)
            {
                res_indices[index++] = vertex - CUBIC_X_EDGES - 1;
                res_indices[index++] = vertex;
            }
            if(connect_with_previous_layer)
            {
                res_indices[index++] = vertex - (CUBIC_X_EDGES + 1)*(CUBIC_Y_EDGES + 1);
                res_indices[index++] = vertex;
            }
            
            ++vertex;
        }
    }
    
    void generate_layer(float x_step, float y_step, const float3 & position, const float4 & color,
                        bool connect_with_previous_layer, bool inside,
                        Index &vertex, DWORD &index, Vertex *res_vertices, Index *res_indices)
    {
        for(int i = 0; i <= CUBIC_Y_EDGES; ++i)
        {
            bool connect_with_previous_edge = ( 0 != i );
            bool actually_inside = ( inside && 0 != i && CUBIC_Y_EDGES != i );
            
                generate_edge(x_step, position + float3(0, i*y_step, 0), color,
                              connect_with_previous_edge, connect_with_previous_layer, actually_inside,
                              vertex, index, res_vertices, res_indices);
        }
    }
}

void cubic( float x_size, float y_size, float z_size, const float3 & position, const float4 & color,
            Vertex *res_vertices, Index *res_indices)
{
    Index vertex = 0; // index of current vertex
    DWORD index = 0; // index of current index
    
    _ASSERT(res_vertices != NULL);
    _ASSERT(res_indices != NULL);

    float x_step = x_size / CUBIC_X_EDGES;
    float y_step = y_size / CUBIC_Y_EDGES;
    float z_step = z_size / CUBIC_Z_EDGES;

    for(int i = 0; i <= CUBIC_Z_EDGES; ++i)
    {
        bool connect_with_previous_level = ( 0 != i);
        bool inside = ( 0 != i && CUBIC_Z_EDGES != i);
        generate_layer(x_step, y_step, position + float3(0, 0, i*z_step), color,
                       connect_with_previous_level, inside,
                       vertex, index, res_vertices, res_indices);
    }
}

const Index SimpleCube::INDICES[INDICES_NUM] =
{
    0, 1, 2,
    0, 2, 3,

    6, 4, 7,
    6, 5, 4,

    0, 7, 4,
    0, 3, 7,

    6, 1, 5,
    6, 2, 1,

    6, 7, 3,
    6, 3, 2,

    0, 4, 5,
    0, 5, 1,
};

SimpleCube::SimpleCube(const float3 & min_corner, const float3 & max_corner, const float4 & color)
{
    for (int i = 0; i < VERTICES_NUM; ++i)
    {
        vertices[i].color = color;
        vertices[i].normal = float3(1, 0, 0); // NB: SimpleCube is not well suited for lighting
    }
    vertices[0].pos = float3(min_corner.x, min_corner.y, min_corner.z);
    vertices[1].pos = float3(min_corner.x, max_corner.y, min_corner.z);
    vertices[2].pos = float3(max_corner.x, max_corner.y, min_corner.z);
    vertices[3].pos = float3(max_corner.x, min_corner.y, min_corner.z);
    vertices[4].pos = float3(min_corner.x, min_corner.y, max_corner.z);
    vertices[5].pos = float3(min_corner.x, max_corner.y, max_corner.z);
    vertices[6].pos = float3(max_corner.x, max_corner.y, max_corner.z);
    vertices[7].pos = float3(max_corner.x, min_corner.y, max_corner.z);
}

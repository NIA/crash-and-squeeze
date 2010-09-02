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
const DWORD CUBIC_PRIMITIVES_COUNT = XYZ_SEGMENTS_COUNT;
const DWORD CUBIC_INDICES_COUNT = 2*CUBIC_PRIMITIVES_COUNT; // Calculated for D3DPT_LINELIST primitive type

namespace
{
    struct GENERATION_PARAMS
    {
        float x_step;
        float y_step;
        float z_step;
        D3DXVECTOR3 position;
        D3DCOLOR color;

        Vertex *res_vertices;
        Index *res_indices;
    };

    // generates an adge along x-axis
    void generate_edge(float x_step, D3DXVECTOR3 position, D3DCOLOR color,
                       bool connect_with_previous_edge, bool connect_with_previous_layer, bool inside,
                       Index &vertex, DWORD &index, Vertex *res_vertices, Index *res_indices)
    {
        D3DXCOLOR no_alpha_color(color);
        no_alpha_color.a = 0;
        
        for(int i = 0; i <= CUBIC_X_EDGES; ++i)
        {
            res_vertices[vertex] = Vertex(position + D3DXVECTOR3(i*x_step, 0, 0), color, D3DXVECTOR3(0,0,0));
            
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
    
    void generate_layer(float x_step, float y_step, D3DXVECTOR3 position, D3DCOLOR color,
                        bool connect_with_previous_layer, bool inside,
                        Index &vertex, DWORD &index, Vertex *res_vertices, Index *res_indices)
    {
        for(int i = 0; i <= CUBIC_Y_EDGES; ++i)
        {
            bool connect_with_previous_edge = ( 0 != i );
            bool actually_inside = ( inside && 0 != i && CUBIC_Y_EDGES != i );
            
                generate_edge(x_step, position + D3DXVECTOR3(0, i*y_step, 0), color,
                              connect_with_previous_edge, connect_with_previous_layer, actually_inside,
                              vertex, index, res_vertices, res_indices);
        }
    }
}

void cubic( float x_size, float y_size, float z_size, D3DXVECTOR3 position, const D3DCOLOR color,
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
        generate_layer(x_step, y_step, position + D3DXVECTOR3(0, 0, i*z_step), color,
                       connect_with_previous_level, inside,
                       vertex, index, res_vertices, res_indices);
    }
}
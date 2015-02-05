#include "cylinder.h"
#include <cstdlib>

Index cylinder_vertices_count(Index edges_per_base, Index edges_per_height, Index edges_per_cap)
{
    return (edges_per_base)*((edges_per_height + 1) + 2 + 2*(edges_per_cap -1)) // vertices per edges_per_height+1 levels plus last ans first levels again, plus edges_per_cap-1 levels per each of 2 caps
           + 2; // plus centers of 2 caps
}

DWORD cylinder_indices_count(Index edges_per_base, Index edges_per_height, Index edges_per_cap)
{
    return 2*(edges_per_base + 1)*(edges_per_height + 2*(edges_per_cap - 1)) // indices per edges_per_height levels plus edges_per_cap-1 levels per each of 2 caps
           + 2*(2*edges_per_base + 1) // plus 2 ends of caps
           + 2; // plus degenerate triangle for jump from top to bottom
}

namespace
{
    float random(float max)
    {
        return rand()/static_cast<float>(RAND_MAX+1)*max;
    }

    struct GENERATION_PARAMS
    {
        // output buffers
        Vertex *res_vertices;
        Index *res_indices;
        // sizes
        float radius;
        float random_amp;
        float height;
        D3DXVECTOR3 position;
        // mesh dimensions
        Index edges_per_base;
        Index edges_per_height;
        Index edges_per_cap;
        // colors
        const D3DCOLOR *colors;
        unsigned colors_count;
        // options
        bool radial_strips; // radial (depending on step) or vertical (depending on level) color distribution
        bool vertical;      // vertical (for cylinder side) or horisontal (for caps) moving whe generating
        bool top;           // for cap generation: is it top or bottom cap. Ignored when vertical == true
    };

    void generate_levels(Index &vertex, DWORD &index, const GENERATION_PARAMS &params)
    {
        const float STEP_ANGLE = 2*D3DX_PI/params.edges_per_base;
        const float STEP_UP = params.height/params.edges_per_height;
        const float STEP_RADIAL = params.radius/params.edges_per_cap;

        Index levels_count = params.vertical ? params.edges_per_height + 1 : params.edges_per_cap;
        Index levels_or_steps_count = params.radial_strips ? params.edges_per_base : levels_count;
        _ASSERT(params.colors_count != 0);
        Index part_size = (levels_or_steps_count + params.colors_count)/params.colors_count; // `+ colors_count' just for excluding a bound of interval [0, colors_count)
        
        D3DXVECTOR3 normal_if_horisontal = D3DXVECTOR3(0, 0, params.top ? 1.0f : -1.0f);
        float z_if_horisontal = params.top ? params.height : 0.0f;
        float weight_if_horisontal = params.top ? 1.0f : 0.0f;
    
        for( Index level = 0; level < levels_count; ++level )
        {
            for( Index step = 0; step < params.edges_per_base; ++step )
            {
                // !! randomization !!
                float radius = params.vertical ? params.radius*(1 + random(params.random_amp)) : (params.radius - STEP_RADIAL*level);
                float z, weight;
                D3DXVECTOR3 normal;
                if (params.vertical)
                {
                    z = level*STEP_UP;
                    weight = static_cast<float>(level)/params.edges_per_height;
                    normal = D3DXVECTOR3( cos(step*STEP_ANGLE), sin(step*STEP_ANGLE), 0 );
                }
                else
                {
                    normal = normal_if_horisontal;
                    z = z_if_horisontal;
                    weight = weight_if_horisontal;
                }
                D3DXVECTOR3 position = params.position + D3DXVECTOR3( radius*cos(step*STEP_ANGLE),
                                                                      radius*sin(step*STEP_ANGLE),
                                                                      z);
                D3DCOLOR color = params.radial_strips ? params.colors[step/part_size] : params.colors[level/part_size];

                if( level == 0 && !params.vertical)
                {
                    // first level for horisontal is just copy of:
                    // * last vertices (for top cap)
                    //    OR
                    // * first vertices (for bottom cap)
                    unsigned copy_from = params.top ? vertex - params.edges_per_base : step;
                    params.res_vertices[vertex] = params.res_vertices[copy_from];
                    params.res_vertices[vertex].set_normal( normal );
                    params.res_vertices[vertex].color = color;
                    ++vertex;
                    continue;
                }
                params.res_vertices[vertex] = Vertex(position, color, normal);
                if( level != 0 )
                {
                    params.res_indices[index++] = vertex;                           // from current level
                    params.res_indices[index++] = vertex - params.edges_per_base; // from previous level
                    if( step == params.edges_per_base - 1 ) // last step
                    {
                        params.res_indices[index++] = vertex - params.edges_per_base + 1; // first from current level
                        params.res_indices[index++] = vertex - 2*params.edges_per_base + 1; // first from previuos level
                    }
                }
                ++vertex;
            }
        }
        if( !params.vertical )
        {
            // for caps: add center vertex and triangles with it
            D3DXVECTOR3 position = D3DXVECTOR3( 0, 0, z_if_horisontal ) + params.position;
            params.res_vertices[vertex] = Vertex( position, params.colors[0], normal_if_horisontal );
            for( Index step = 0; step < params.edges_per_base; ++step )
            {
                params.res_indices[index++] = vertex;
                params.res_indices[index++] = vertex - params.edges_per_base + step;
            }
            params.res_indices[index++] = vertex - params.edges_per_base;
            ++vertex;
        }
    }
}

void cylinder( float radius, float height, D3DXVECTOR3 position,
               const D3DCOLOR *colors, unsigned colors_count,
               Index edges_per_base, Index edges_per_height, Index edges_per_cap,
               Vertex *res_vertices, Index *res_indices, float random_amp)
// Writes data into arrays given as `res_vertices' and `res_indices',
{
    Index vertex = 0; // current vertex
    DWORD index = 0; // current index
    
    _ASSERT(NULL != res_vertices);
    _ASSERT(NULL != res_indices);
    _ASSERT(NULL != colors);
    _ASSERT(0 != colors_count);

    GENERATION_PARAMS params;
    // output buffers
    params.res_vertices = res_vertices;
    params.res_indices = res_indices;
    // sizes
    params.radius = radius;
    params.random_amp = random_amp;
    params.height = height;
    params.position = position;
    // mesh dimensions
    params.edges_per_base = edges_per_base;
    params.edges_per_height = edges_per_height;
    params.edges_per_cap = edges_per_cap;
    // colors
    params.colors = colors;
    params.colors_count = colors_count;
    // options
    params.radial_strips = false;
    params.vertical = true;

    generate_levels(vertex, index, params);

    // Cap
    params.radial_strips = true;
    params.vertical = false;
    params.top = true;
    generate_levels(vertex, index, params);

    for( unsigned i = 0; i < VERTICES_PER_TRIANGLE-1; ++i )
    {
        // making degenerate triangle
        res_indices[index++] = vertex;
    }

    params.top = false;
    generate_levels(vertex, index, params);
}

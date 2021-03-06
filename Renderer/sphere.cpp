#define _USE_MATH_DEFINES
#include "sphere.h"
#include <cmath>

Index sphere_vertices_count(Index edges_per_meridian)
{
    return 2 // 2 poles
           + (edges_per_meridian - 1)*2*edges_per_meridian; // (n - 1) layers of 2*n vertices
}

// Calculated for TRIANGLELIST primitive type
Index sphere_indices_count(Index edges_per_meridian)
{
    return 3*( 4*edges_per_meridian // 4*n triangles, 2*n near each pole
             + 2*(edges_per_meridian - 2)*2*edges_per_meridian ); // 2 triangles per cell, (n - 2) layers of 2*n cells
}

void sphere(float radius, const float3 & position, const float4 & color, Index edges_per_meridian,
            /*out*/ Vertex *res_vertices, /*out*/ Index *res_indices)
{
    Index vertex = 0;
    Index index = 0;

    _ASSERT(0 != edges_per_meridian);
    float angle_step = static_cast<float>(M_PI)/edges_per_meridian;

    Index edges_per_diameter = 2*edges_per_meridian;

    for(Index theta_index = 0; theta_index <= edges_per_meridian; ++theta_index)
    {
        float theta = theta_index*angle_step;
        
        for(Index phi_index = 0; phi_index < edges_per_diameter; ++ phi_index)
        {
            float phi = phi_index*angle_step;

            float3 normal(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));

            // add vertex only if not last layer: south pole
            bool add_vertex = !(edges_per_meridian == theta_index && phi_index != 0);

            if(add_vertex)
            {
                res_vertices[vertex] = Vertex(position + normal*radius, color, normal);
            }

            Index last_vertex = add_vertex ? vertex : vertex - 1;
            Index vertex_over = add_vertex ? last_vertex - edges_per_diameter : last_vertex - edges_per_diameter + phi_index; // if vertex was not added, vertex counter is not incremented, have to add
            Index vertex_back =      ( 0 != phi_index ) ? last_vertex - 1 : last_vertex + edges_per_diameter - 1;
            Index vertex_over_back = ( 0 != phi_index ) ? vertex_over - 1 : vertex_over + edges_per_diameter - 1;

            if(1 == theta_index)
            {
                // first layer: connect to north pole (north pole is vertex_over and is 0)
                vertex_over = 0;
                add_triangle(vertex_back, last_vertex, vertex_over, res_indices, index);
            }
            else if(edges_per_meridian == theta_index)
            {
                // last layer: connect to south pole (south pole is vertex)
                add_triangle(vertex_over_back, last_vertex, vertex_over, res_indices, index);
            }
            else if(0 != theta_index)
            {
                // connect up and back
                add_triangle(vertex_back, last_vertex, vertex_over, res_indices, index);
                add_triangle(vertex_back, vertex_over, vertex_over_back, res_indices, index);
            }

            if(add_vertex)
            {
                // if vertex really was added, increment counter
                ++vertex;
            }

            if(0 == theta_index)
            {
                // north pole is only one point, break phi loop
                break;
            }
        }
    }
}

static float & get_component(float3 & vector, int comp_ind)
{
    switch (comp_ind)
    {
    case 0:
    	return vector.x;
    case 1:
        return vector.y;
    case 2:
        return vector.z;
    default:
        _ASSERT(false);
        return vector.x;
    }
}

void squeeze_sphere(float coeff, int axis, /*in/out*/ Vertex *vertices, Index vertices_count) {
    _ASSERT(axis >= 0 && axis < 3);

    for(Index i = 0; i < vertices_count; ++i) {
        get_component(vertices[i].pos, axis)    *= coeff;
        get_component(vertices[i].normal, axis) /= coeff;
    }
}
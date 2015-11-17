#pragma once
#include "main.h"
#include "Vertex.h"

extern const Index CUBIC_X_EDGES;
extern const Index CUBIC_Y_EDGES;
extern const Index CUBIC_Z_EDGES;

extern const Index CUBIC_VERTICES_COUNT;
extern const Index CUBIC_PRIMITIVES_COUNT;
extern const DWORD CUBIC_INDICES_COUNT; // Calculated for D3DPT_LINELIST primitive type

// Writes data into arrays given as `res_vertices' and `res_indices',
void cubic( float x_size, float y_size, float z_size, const float3 & position, const float4 & color,
            Vertex *res_vertices, Index *res_indices);

// A simple cube of 8 vertices (for visualizing of box regions)
 // NB: SimpleCube is not well suited for lighting (all normals are the same)
class SimpleCube
{
public:
    static const unsigned VERT_PER_FACE = 4;
    static const unsigned IND_PER_FACE = 6; // for D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    static const unsigned FACES_NUM = 6;
    static const unsigned VERTICES_NUM = 2*VERT_PER_FACE;
    static const unsigned INDICES_NUM = FACES_NUM*IND_PER_FACE;
    static const D3D_PRIMITIVE_TOPOLOGY TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

private:
    Vertex vertices[VERTICES_NUM];
    static const Index INDICES[INDICES_NUM];
public:
    SimpleCube(const float3 & min_corner, const float3 & max_corner, const float4 & color);

    const Vertex * get_vertices() const { return vertices; }
    const Index get_vertices_num() const { return VERTICES_NUM; }
    const Index * get_indices() const { return INDICES; }
    const Index get_indices_num() const { return INDICES_NUM; }
    D3D_PRIMITIVE_TOPOLOGY get_topology() { return TOPOLOGY; }
};

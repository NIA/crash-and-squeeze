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
void cubic( float x_size, float y_size, float z_size, D3DXVECTOR3 position, const D3DCOLOR color,
            Vertex *res_vertices, Index *res_indices);

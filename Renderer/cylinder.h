#pragma once
#include "main.h"
#include "Vertex.h"

Index cylinder_vertices_count(Index edges_per_base, Index edges_per_height, Index edges_per_cap);
// Calculated for TRIANGLESTRIP primitive type
DWORD cylinder_indices_count(Index edges_per_base, Index edges_per_height, Index edges_per_cap);

// Writes data into arrays given as `res_vertices' and `res_indices',
void cylinder( float radius, float height, D3DXVECTOR3 position,
               const D3DCOLOR *colors, unsigned colors_count,
               Index edges_per_base, Index edges_per_height, Index edges_per_cap,
               Vertex *res_vertices, Index *res_indices);

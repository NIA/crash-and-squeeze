#pragma once
#include "main.h"
#include "Vertex.h"

Index sphere_vertices_count(Index edges_per_diameter);
// Calculated for TRIANGLELIST primitive type
DWORD sphere_indices_count(Index edges_per_diameter);

void sphere(float radius, D3DXVECTOR3 position, D3DCOLOR color, Index edges_per_diameter,
            /*out*/ Vertex *res_vertices, /*out*/ Index *res_indices);

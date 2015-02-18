#pragma once
#include "main.h"
#include "Vertex.h"

Index sphere_vertices_count(Index edges_per_diameter);
// Calculated for TRIANGLELIST primitive type
Index sphere_indices_count(Index edges_per_diameter);

void sphere(float radius, const float3 & position, const float4 & color, Index edges_per_diameter,
            /*out*/ Vertex *res_vertices, /*out*/ Index *res_indices);

// Function to make an ellipse from sphere
void squeeze_sphere(float coeff, int axis, /*in/out*/ Vertex *vertices, Index vertices_count);

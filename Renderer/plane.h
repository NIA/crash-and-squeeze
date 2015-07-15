#pragma once
#include "main.h"
#include "Vertex.h"

extern const int PLANE_STEPS_PER_HALF_SIDE;
extern const Index PLANE_VERTICES_COUNT;
extern const DWORD PLANE_INDICES_COUNT;

void plane(float3 x_direction, float3 y_direction, float3 center, Vertex *res_vertices, Index *res_indices, const float4 & color);

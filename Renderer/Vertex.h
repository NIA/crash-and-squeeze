#pragma once
#include "main.h"
#include "Core/vertex_info.h"

extern const DXGI_FORMAT INDEX_FORMAT;

const int VERTICES_PER_TRIANGLE = 3;

inline float rand_col_comp()
// Returns random color component: a float between 0.0f and 1.0f
{
    return static_cast<float>(rand())/RAND_MAX;
}

inline float4 random_color()
{
    return float4( rand_col_comp(), rand_col_comp(), rand_col_comp(), 1.0f );
}

struct Vertex
{
    float3 pos;            // The position for the vertex
    float3 normal;         // The outer normal of model
    float4 color;          // The vertex color // TODO: use more compact DirectX::PackedVector::XMCOLOR instead??
    // 8 BYTE cluster indices = 2 UBYTE4s
    BYTE cluster_indices[CrashAndSqueeze::Core::VertexInfo::CLUSTER_INDICES_NUM];
    DWORD clusters_num;
    
    void set_normal(const float3 & tri_normal)
    {
        normal = tri_normal;
    }
    Vertex()
    {
        color = random_color();
        set_normal(float3(1.0f, 0, 0));
    }
    Vertex(const float3 & pos, const float4 & color, const float3 & normal) : pos(pos), color(color)
    {
        set_normal(normal);
    }
    Vertex(const float3 & pos, const float3 & normal) : pos(pos)
    {
        color = random_color();
        set_normal(normal);
    }
};

extern const D3D11_INPUT_ELEMENT_DESC  VERTEX_DESC[];
extern const unsigned                  VERTEX_DESC_NUM;

extern const ::CrashAndSqueeze::Core::VertexInfo VERTEX_INFO;

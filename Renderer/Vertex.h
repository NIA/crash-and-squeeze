#pragma once
#include "main.h"
#include "Core/vertex_info.h"

extern const D3DFORMAT INDEX_FORMAT;

const int VERTICES_PER_TRIANGLE = 3;

inline int rand_col_comp()
// Returns random color component: an integer between 0 and 255
{
    return rand()*255/RAND_MAX;
}

inline D3DCOLOR random_color()
{
    return D3DCOLOR_XRGB( rand_col_comp(), rand_col_comp(), rand_col_comp() );
}

class Vertex
{
public:
    D3DXVECTOR3 pos;            // The position for the vertex
    D3DXVECTOR3 normal;         // The outer normal of model
    D3DCOLOR color;             // The vertex color
    // 8 BYTE cluster indices = 2 UBYTE4s
    BYTE cluster_indices[CrashAndSqueeze::Core::VertexInfo::CLUSTER_INDICES_NUM];
    DWORD clusters_num;
    
    void set_normal(D3DXVECTOR3 tri_normal)
    {
        normal = tri_normal;
    }
    Vertex() : color(0)
    {
        set_normal(D3DXVECTOR3(1.0f, 0, 0));
    }
    Vertex(D3DXVECTOR3 pos, D3DCOLOR color, D3DXVECTOR3 normal) : pos(pos), color(color)
    {
        set_normal(normal);
    }
    Vertex(D3DXVECTOR3 pos, D3DXVECTOR3 normal) : pos(pos)
    {
        color = random_color();
        set_normal(normal);
    }
};


extern const D3DVERTEXELEMENT9 VERTEX_DECL_ARRAY[];

extern const ::CrashAndSqueeze::Core::VertexInfo VERTEX_INFO;

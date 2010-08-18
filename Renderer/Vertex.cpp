#include "Vertex.h"

const D3DFORMAT INDEX_FORMAT = D3DFMT_INDEX32;

const D3DVERTEXELEMENT9 VERTEX_DECL_ARRAY[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
    {0, 28, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
};

const ::CrashAndSqueeze::Core::VertexInfo VERTEX_INFO( sizeof(Vertex), 0 );

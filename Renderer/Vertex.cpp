#include "Vertex.h"

const DXGI_FORMAT INDEX_FORMAT = DXGI_FORMAT_R32_UINT;

const D3D11_INPUT_ELEMENT_DESC  VERTEX_DESC[] =
{
    // semantic  semInd         format             slot  offset      inputSlotClass       instanceDataStepRate
    {"POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0,    0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,   0,   12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",      0, DXGI_FORMAT_B8G8R8A8_UNORM,    0,   24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",      1, DXGI_FORMAT_R8G8B8A8_UINT,     0,   28, D3D11_INPUT_PER_VERTEX_DATA, 0}, // 0..3 cluster indices
    {"COLOR",      2, DXGI_FORMAT_R8G8B8A8_UINT,     0,   32, D3D11_INPUT_PER_VERTEX_DATA, 0}, // 4..7 cluster indices
    {"COLOR",      3, DXGI_FORMAT_R8G8B8A8_UINT,     0,   36, D3D11_INPUT_PER_VERTEX_DATA, 0}  // clusters num
};
extern const unsigned VERTEX_DESC_NUM = array_size(VERTEX_DESC);

const ::CrashAndSqueeze::Core::VertexInfo VERTEX_INFO( sizeof(Vertex), 0, 12, true, 28, 36 );

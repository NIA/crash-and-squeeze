struct VS_INPUT
{
    float4 pos : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4  pos : POSITION;
    float4  color : COLOR;
    float4  psPos : TEXCOORD0; // pos for pixel shader
    float3  psNormal : TEXCOORD1; // normal for pixel shader
    float   point_size : PSIZE;
};

const float4x4 view : register(c0);
const float4x4 pos_and_rot : register(c22);

VS_OUTPUT main(const VS_INPUT src)
{
    // transformed pos
    float4 pos = mul(src.pos, pos_and_rot);
    
    VS_OUTPUT res;
    res.pos   = mul(pos, view);
    res.psPos = pos;
    res.color = src.color;
    res.point_size = 3;
    res.psNormal = src.normal;
    return res;
}
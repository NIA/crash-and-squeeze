struct VS_INPUT
{
    float4 pos : POSITION;
    float4 color : COLOR0;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4  pos : SV_POSITION;
    float4  color : COLOR0;
    float4  psPos : TEXCOORD0; // pos for pixel shader
    float3  psNormal : TEXCOORD1; // normal for pixel shader
    float   point_size : PSIZE;
};

cbuffer WorldConstants : register(b0)
{
    float4x4 world;  // aka post_transform
    float4x4 view;   // camera's view * projection
    // other fields not used
};

cbuffer ModelConstants : register(b1)
{
    float4x4 pos_and_rot; // aka model matrix
    // other fields not used
};

VS_OUTPUT main(const VS_INPUT src)
{
    // transformed pos
    float4 pos = mul(src.pos, pos_and_rot);
    pos = mul(pos, world);
    // transformed normal
    float3 normal = mul(src.normal, (float3x3)pos_and_rot);
    normal = normalize( mul(normal, (float3x3)world ) );
    
    VS_OUTPUT res;
    res.pos   = mul(pos, view);
    res.psPos = pos;
    res.color = src.color;
    res.point_size = 3;
    res.psNormal = normal;
    return res;
}
struct VS_INPUT
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR0;
    
    int4 ci_0_3 : COLOR1; // cluster indices 0..3
    int4 ci_4_7 : COLOR2; // cluster indices 4..7
    int clus_num : COLOR3; // clusters num
};

struct VS_OUTPUT
{
    float4  pos : SV_POSITION;
    float4  color : COLOR0;
    float4  psPos : TEXCOORD0; // pos for pixel shader
    float3  psNormal : TEXCOORD1; // normal for pixel shader
};

static int MAX_CLUSTERS_NUM = 50; // TODO: get this value from C++ code through shader defines
cbuffer WorldConstants : register(b0)
{
    float4x4 world;  // aka post_transform
    float4x4 view;   // camera's view * projection
    // other fields not used
};

cbuffer ModelConstants : register(b1)
{
    float4x4 pos_and_rot; // aka model matrix
    float4x4 clus_mx[MAX_CLUSTERS_NUM+1];     // clusters' position tranformation (deform+c.m. shift) matrices PLUS one zero matrix in the end
    float4x4 clus_nrm_mx[MAX_CLUSTERS_NUM+1]; // clusters' normal   tranformation matrices PLUS one zero matrix in the end
    float3   clus_cm[MAX_CLUSTERS_NUM+1];     // clusters' initial c.m. (centers of mass)
};

void sum_iter(float4 pos, float3 normal, inout float3 sum_pos, inout float3 sum_normal, int ci)
{
    float4 offset = pos - clus_cm[ci];
    sum_pos += mul(offset, clus_mx[ci]);
    sum_normal += mul(normal, (float3x3)clus_nrm_mx[ci]);
}

void deform(const VS_INPUT v, out float4 pos, out float3 normal)
{
    float3 sum_pos = 0;
    float3 sum_normal = 0;
    
    for(int i = 0; i < 4; ++i)
    {
        sum_iter(v.pos, v.normal, sum_pos, sum_normal, v.ci_0_3[i]);
        
        // WTF: if next line is commented, summing will be done
        //      only for first 4 cluster indices. Strange is that
        //      rendering will also be MORE than 2 times faster
        //      (but only the half of the summing is not supposed
        //      to take more the half of shader execution time!)
        sum_iter(v.pos, v.normal, sum_pos, sum_normal, v.ci_4_7[i]);
    }
    
    pos = float4(sum_pos/v.clus_num, 1);
    normal = sum_normal/v.clus_num;
}

VS_OUTPUT main(const VS_INPUT src)
{
    // transform pos & normal due to deformation
    float4 pos;
    float3 normal;
    deform(src, pos, normal);
    
    // transform pos & normal due to shift and rotation
    pos = mul(pos, pos_and_rot);
    pos = mul(pos, world);
    normal = mul(normal, (float3x3)pos_and_rot) );
    normal = normalize( mul(normal, (float3x3)world) );
    
    // apply projection into view space and lighting
    VS_OUTPUT res;
    res.pos   = mul(pos, view);
    res.psPos = pos;
    res.color = src.color;
    res.psNormal = normal;
    return res;
}
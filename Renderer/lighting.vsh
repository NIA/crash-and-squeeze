struct VS_INPUT
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    
    int4 ci_0_3 : COLOR1; // cluster indices 0..3
    int4 ci_4_7 : COLOR2; // cluster indices 4..7
    int clus_num : COLOR3; // clusters num
};

struct VS_OUTPUT
{
    float4  pos : POSITION;
    float4  color : COLOR;
};

const float4x4  view        : register(c0);
const float4x4  pos_and_rot : register(c22); // position and rotation matrix

const float4    clus_cm[24] : register(c26); // initial clusters' centers of mass 
const float4x3  clus_mx[25] : register(c50); // cluster matrices PLUS last zero matrix
const float4x3  clus_nrm_mx[25] : register(c125); // cluster normal matrices PLUS last zero matrix

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
    normal = normalize( mul(normal, (float3x3)pos_and_rot) );
    
    // apply projection into view space and lighting
    VS_OUTPUT res;
    res.pos   = mul(pos, view);
    res.color = float4(normal, 1);
    return res;
}
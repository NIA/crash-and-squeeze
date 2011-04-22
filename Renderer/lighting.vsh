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
const float3    eye         : register(c21);
const float4x4  pos_and_rot : register(c22); // position and rotation matrix

const float     diff_coef   : register(c14); // diffuse component coefficient
const float     spec_coef   : register(c19); // specular component coefficient
const float     spec_factor : register(c20); // specular factor (f)

const float3    direct_vec  : register(c12); // directional light vector
const float4    direct_col  : register(c13); // directional light color

const float3    point_pos   : register(c17); // point light position
const float4    point_col   : register(c16); // point light color
const float3    atten_coefs : register(c18); // attenuation coeffs (a, b, c)

const float4    ambient_col : register(c15);

const float4    clus_cm[24] : register(c26); // initial clusters' centers of mass 
const float4x3  clus_mx[25] : register(c50); // cluster matrices PLUS last zero matrix
const float4x3  clus_nrm_mx[25] : register(c125); // cluster normal matrices PLUS last zero matrix

float4 directional_light(const float3 pos, const float3 normal, const float3 v)
{
    float cos_theta = dot(direct_vec, normal);
    
    // -- diffuse --
    
    float diff = diff_coef * cos_theta;
    
    // -- specular --
    
    // calculating vector r = 2*(l, n)*n - l, where l is direction to light
    float3 r = 2*cos_theta*normal - direct_vec;
    float cos_phi = max(dot(r, v), 0);
    float spec = spec_coef * pow(cos_phi, spec_factor);
    
    return direct_col*(max(diff, 0) + max(spec, 0));
}

float4 point_light(const float3 pos, const float3 normal, const float3 v)
{
    float3 light_vec = point_pos - pos;
    float dist = length(light_vec);
    light_vec = normalize(light_vec); // normalize

    float atten = 1/(atten_coefs.x + atten_coefs.y*dist + atten_coefs.z*dist*dist);

    float cos_theta = dot(light_vec, normal);
    
    // -- diffuse --
    
    float diff = diff_coef * cos_theta;
    
    // -- specular --
    
    // calculating vector r = 2*(l, n)*n - l, where l is direction to light
    float3 r = 2*cos_theta*normal - light_vec;
    float cos_phi = max(dot(r, v), 0);
    float spec = spec_coef * pow(cos_phi, spec_factor);
    
    return point_col* atten* (max(diff, 0) + max(spec, 0));
}

float4 light(const float3 pos, const float3 normal)
{
    // calculating normalized vector v (from vertex to eye)
    float3 v = normalize(eye - pos);
    
    return directional_light(pos, normal, v)
           + point_light(pos, normal, v)
           + ambient_col;
}

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
        if(v.clus_num > 4)
        {
            sum_iter(v.pos, v.normal, sum_pos, sum_normal, v.ci_4_7[i]);
        }
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
    res.color = src.color*light((float3)pos, normal);
    return res;
}
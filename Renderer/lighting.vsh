struct VS_INPUT
{
    float4 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
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

VS_OUTPUT main(const VS_INPUT src)
{
    VS_OUTPUT res;
    
    // transformed pos
    float4 pos = mul(src.pos, pos_and_rot);
    
    // transformed normal
    float3 normal = mul(src.normal, (float3x3)pos_and_rot);
    normal = normalize(normal);
    
    res.pos   = mul(pos, view);
    res.color = src.color*light((float3)pos, normal);
    return res;
}
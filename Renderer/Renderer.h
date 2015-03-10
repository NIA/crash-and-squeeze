#pragma once

#include <vector>

#include "Core/model.h"

#include "main.h"
#include "IRenderer.h"
#include "Camera.h"
#include "Window.h"
#include "Buffer.h"
#include "performance_reporter.h"

typedef ::CrashAndSqueeze::Core::Model PhysicalModel;
class AbstractModel;
struct ModelEntity
{
    AbstractModel       *high_model;
    AbstractModel       *low_model;
    PhysicalModel       *physical_model;
    PerformanceReporter *performance_reporter;
};
typedef std::vector<ModelEntity> ModelEntities;

extern const unsigned VECTORS_IN_MATRIX;

class Renderer : public IRenderer
{
private:
    ID3D11Device                *device;
    ID3D11DeviceContext         *context;

    ID3D11RenderTargetView      *render_target_view;
    ID3D11DepthStencilView      *depth_stencil_view;
    IDXGISwapChain              *swap_chain;
    ID3D11RasterizerState       *rs_wireframe_on;
    ID3D11RasterizerState       *rs_wireframe_off;

    bool directional_light_enabled;
    bool point_light_enabled;
    bool spot_light_enabled;
    bool ambient_light_enabled;

    bool alpha_test_enabled;
    bool wireframe;

    Camera * camera;
    float4x4 post_transform; // matrix multiplied by model matrix (kinda world matrix)

    const TCHAR * text_to_draw;

    // Initialization steps:
    void init_device(Window &window);
    void init_buffers();
    void init_font();

#pragma pack( push )
#pragma pack( 4 ) // use same packing for constant buffer structures as HLSL does (4)

#pragma warning( push )
#pragma warning( disable : 4324 ) // do not warn me about padding due to __declspec(align) because I know what I'm doing
    // Shader constants:
    __declspec( align(16) ) // constant buffer data size should be a multiple of 16
    struct WorldConstants
    {
        float4x4 world;  // aka post_transform
        float4x4 view;   // camera's view * projection
        float3   eye;    // camera's eye
    };
    ConstantBuffer<WorldConstants> *world_constants;

    // Currently some little value N, but can be made greater while the following is true:
    // (2*N+1 matrices)*(4 vectors in matrix) + (N vectors) <= 4096 (maximum number of items in constant buffer)
    // ^--- for CAS_QUADRATIC_EXTENSIONS_ENABLED substitute 4*N for 2*N
    // TODO: pass this value to shader via defines
    static const unsigned MAX_CLUSTERS_NUM = 50;
    __declspec( align(16) ) // constant buffer data size should be a multiple of 16
    struct ModelConstants
    {
        float4x4 pos_and_rot; // aka model matrix
        float4x4 clus_mx[MAX_CLUSTERS_NUM+1];     // clusters' position transformation (deform+c.m. shift) matrices PLUS one zero matrix in the end
        float4x4 clus_nrm_mx[MAX_CLUSTERS_NUM+1]; // clusters' normal   transformation matrices PLUS one zero matrix in the end
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
        float4x4 clus_mx_quad[MAX_CLUSTERS_NUM+1];// clusters' _quadratic_ position transformation _quad_ matrices PLUS one zero matrix in the end
        float4x4 clus_mx_mix[MAX_CLUSTERS_NUM+1]; // clusters' _quadratic_ position transformation _mix_ matrices PLUS one zero matrix in the end
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
        float4   clus_cm[MAX_CLUSTERS_NUM+1];     // clusters' initial c.m. (centers of mass)
    };
    ConstantBuffer<ModelConstants> *model_constants;

    __declspec( align(16) ) // constant buffer data size should be a multiple of 16
    struct LightingConstants
    {
        // TODO: find better way to properly pack these values
        float4   direct_col;  // directional light color
        float4   point_col;   // point light color
        float4   ambient_col; // ambient light color
        float3   direct_vec;  // directional light vector
        float    diff_coef;   // diffuse component coefficient
        float3   point_pos;   // point light position
        float    spec_coef;   // specular component coefficient
        float3   atten_coefs; // attenuation coeffs (a, b, c)
        float    spec_factor; // specular factor (f)
    };
    static const LightingConstants LIGHT_CONSTS_INIT_DATA;
    ConstantBuffer<LightingConstants> *lighting_constants;
#pragma warning( pop )
#pragma pack( pop )

    void set_alpha_test();

    void draw_text(const TCHAR * text, RECT rect, float4 color, bool align_right = false);

    void release_interfaces();

public:
    Renderer(Window &window, Camera * camera);

    ID3D11Device        * get_device()  const override;
    ID3D11DeviceContext * get_context() const override;

    void set_text_to_draw(const TCHAR * text);
    
    void render(const ModelEntities &model_entities, // TODO: store model_entities or pass each time?
                PerformanceReporter &internal_reporter,
                bool is_updating_vertices_on_gpu,
                bool show_high_model = true);

    void toggle_alpha_test();
    void toggle_wireframe();
    void set_wireframe();
    void unset_wireframe();

    virtual ~Renderer(void);
private:
    DISABLE_COPY(Renderer);
};


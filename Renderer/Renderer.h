#pragma once

#include <vector>

#include "Core/model.h"

#include "main.h"
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

// TODO: remove circular dependency (Renderer depends on Model, Camera, Buffer which in turn depend on Renderer)
class Renderer
{
private:
    ID3D11Device                *device;
    ID3D11DeviceContext         *context;

    ID3D11RenderTargetView      *render_target_view;
    IDXGISwapChain              *swap_chain;
    ID3D11RasterizerState       *rs_wireframe_on;
    ID3D11RasterizerState       *rs_wireframe_off;
    ID3DXFont                   *font;          // font to draw text

    bool directional_light_enabled;
    bool point_light_enabled;
    bool spot_light_enabled;
    bool ambient_light_enabled;

    bool alpha_test_enabled;
    bool wireframe;

    Camera * camera;
    D3DXMATRIX post_transform; // matrix multiplied by model matrix (kinda world matrix)

    const TCHAR * text_to_draw;

    // Initialization steps:
    void init_device(Window &window);
    void init_buffers();
    void init_font();

    // Shader constants:
    struct WorldConstants
    {
        float4x4 world;  // aka post_transform
        float4x4 view;   // camera's view * projection
        float3   eye;    // camera's eye
    };
    ConstantBuffer<WorldConstants> *world_constants;

    // Currently some little value N, but can be made greater while the following is true:
    // (2*N+1 matrices)*(4 vectors in matrix) + (N vectors) <= 4096 (maximum number of items in constant buffer)
    // TODO: pass this value to shader via defines
    static const unsigned MAX_CLUSTERS_NUM = 50;
    struct ModelConstants
    {
        float4x4 pos_and_rot; // aka model matrix
        float3   clus_cm[MAX_CLUSTERS_NUM+1];     // clusters' initial c.m. (centers of mass)
        float4x4 clus_mx[MAX_CLUSTERS_NUM+1];     // clusters' position tranformation (deform+c.m. shift) matrices PLUS one zero matrix in the end
        float4x4 clus_nrm_mx[MAX_CLUSTERS_NUM+1]; // clusters' normal   tranformation matrices PLUS one zero matrix in the end
    };
    ConstantBuffer<ModelConstants> *model_constants;

    struct LightingConstants
    {
        float    diff_coef;   // diffuse component coefficient
        float    spec_coef;   // specular component coefficient
        float    spec_factor; // specular factor (f)

        float3   direct_vec;  // directional light vector
        float4   direct_col;  // directional light color

        float3   point_pos;   // point light position
        float4   point_col;   // point light color
        float3   atten_coefs; // attenuation coeffs (a, b, c)

        float4   ambient_col; // ambient light color
    };
    static const LightingConstants LIGHT_CONSTS_INIT_DATA;
    ConstantBuffer<LightingConstants> *lighting_constants;

    void set_alpha_test();

    void draw_text(const TCHAR * text, RECT rect, D3DCOLOR color, bool align_right = false);

    void release_interfaces();

public:
    Renderer(Window &window, Camera * camera);

    ID3D11Device * get_device() const;
    ID3D11DeviceContext * get_context() const;

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


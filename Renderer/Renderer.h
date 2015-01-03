#pragma once

#include <vector>

#include "Core/model.h"

#include "main.h"
#include "Camera.h"
#include "Window.h"
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

class Renderer
{
private:
    IDirect3D9                  *d3d;           // used to create the D3DDevice
    IDirect3DDevice9            *device;        // our rendering device
    ID3DXFont                   *font;          // font to draw text

    bool directional_light_enabled;
    bool point_light_enabled;
    bool spot_light_enabled;
    bool ambient_light_enabled;

    bool alpha_test_enabled;
    bool wireframe;

    Camera * camera;
    D3DXMATRIX post_transform;

    const TCHAR * text_to_draw;

    // Initialization steps:
    void init_device(Window &window);
    void init_font();

    // Wrappers for SetVertexShaderConstantF:
    void set_shader_const(unsigned reg, const float *data, unsigned vector4_count)
    {
        check_render( device->SetVertexShaderConstantF(reg, data, vector4_count) );
        check_render( device->SetPixelShaderConstantF(reg, data, vector4_count) );
    }
    void set_shader_float(unsigned reg, float f)
    {
        set_shader_const(reg, D3DXVECTOR4(f, f, f, f), 1);
    }
    void set_shader_vector(unsigned reg, const D3DXVECTOR3 &vector)
    {
        set_shader_const(reg, D3DXVECTOR4(vector, 0), 1);
    }
    void set_shader_point(unsigned reg, const D3DXVECTOR3 &point)
    {
        set_shader_const(reg, D3DXVECTOR4(point, 1.0f), 1);
    }
    void set_shader_matrix(unsigned reg, const D3DXMATRIX &matrix)
    {
        set_shader_const(reg, matrix, VECTORS_IN_MATRIX);
    }
    void set_shader_matrix3x4(unsigned reg, const D3DXMATRIX &matrix)
    {
        set_shader_const(reg, matrix, VECTORS_IN_MATRIX-1);
    }
    void set_shader_color(unsigned reg, D3DCOLOR color)
    {
        set_shader_const(reg, D3DXCOLOR(color), 1);
    }

    void set_alpha_test();

    void draw_text(const TCHAR * text, RECT rect, D3DCOLOR color, bool align_right = false);

    void release_interfaces();

public:
    Renderer(Window &window, Camera * camera);

    IDirect3DDevice9 * get_device() { return device; }

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
};


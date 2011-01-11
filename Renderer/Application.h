#pragma once
#include "main.h"
#include "Camera.h"
#include "Window.h"
#include "Vertex.h"
#include "Model.h"
#include "performance_reporter.h"
#include <vector>
#include "Core/model.h"

extern const unsigned VECTORS_IN_MATRIX;


typedef ::CrashAndSqueeze::Core::Model PhysicalModel;

struct ModelEntity
{
    Model               *high_model;
    Model               *low_model;
    PhysicalModel       *physical_model;
    PerformanceReporter *performance_reporter;
};

typedef std::vector<ModelEntity> ModelEntities;

class Application
{
private:
    Logger &logger;

    IDirect3D9                  *d3d;           // used to create the D3DDevice
    IDirect3DDevice9            *device;        // our rendering device
    ID3DXFont                   *font;          // font to draw text

    bool directional_light_enabled;
    bool point_light_enabled;
    bool spot_light_enabled;
    bool ambient_light_enabled;
    bool alpha_test_enabled;

    bool emulation_enabled;
    bool emultate_one_step;

    bool wireframe;

    bool vertices_update_needed;
    int show_mode;

    bool show_help;

    Window window;

    ModelEntities model_entities;

    ::CrashAndSqueeze::Core::ForcesArray * forces;
    bool forces_enabled;

    ::CrashAndSqueeze::Core::IRegion * impact_region;
    ::CrashAndSqueeze::Math::Vector impact_velocity;
    Model * impact_model;
    bool impact_happened;
    void move_impact(const ::CrashAndSqueeze::Math::Vector & vector);
    void rotate_impact(const ::CrashAndSqueeze::Math::Vector & rotation_axis);

    Camera camera;

    // Initialization steps:
    void init_device();
    void init_font();

    // Wrappers for SetVertexShaderConstantF:
    void set_shader_const(unsigned reg, const float *data, unsigned vector4_count)
    {
        check_render( device->SetVertexShaderConstantF(reg, data, vector4_count) );
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
    void set_shader_color(unsigned reg, D3DCOLOR color)
    {
        set_shader_const(reg, D3DXCOLOR(color), 1);
    }

    void set_alpha_test();

    void set_show_mode(int new_show_mode);

    void rotate_models(float phi);
    void process_key(unsigned code, bool shift, bool ctrl, bool alt);

    void draw_text(const TCHAR * text, RECT rect, D3DCOLOR color, bool align_right = false);
    void draw_text_info();
    void render();

    // Deinitialization steps:
    void delete_model_stuff();
    void release_interfaces();

public:
    Application(Logger &logger);
    IDirect3DDevice9 * get_device();

    // Adds given model to Application's list of models;
    // creates a physical model if `physical` is true (and returns it);
    PhysicalModel * add_model(Model &high_model, bool physical = false, Model *low_model = NULL);
    void set_forces(::CrashAndSqueeze::Core::ForcesArray & forces);
    void set_impact(::CrashAndSqueeze::Core::IRegion & region,
                    const ::CrashAndSqueeze::Math::Vector &velocity,
                    Model &model);
    void run();

    void toggle_wireframe();
    void set_wireframe();
    void unset_wireframe();

    ~Application();

    enum ShowMode
    {
        SHOW_GRAPHICAL_VERTICES,
        SHOW_CURRENT_POSITIONS,
        SHOW_EQUILIBRIUM_POSITIONS,
        SHOW_INITIAL_POSITIONS,
        _SHOW_MODES_COUNT
    };

private:
    // No copying!
    Application(const Application&);
    Application &operator=(const Application&);
};

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
    Model               *display_model;
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

    bool directional_light_enabled;
    bool point_light_enabled;
    bool spot_light_enabled;
    bool ambient_light_enabled;
    bool alpha_test_enabled;

    bool emulation_enabled;
    bool emultate_one_step;

    Window window;

    ModelEntities model_entities;

    ::CrashAndSqueeze::Core::ForcesArray * forces;
    bool forces_enabled;

    Camera camera;

    // Initialization steps:
    void init_device();

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

    void rotate_models(float phi);
    void process_key(unsigned code);

    void render();

    // Deinitialization steps:
    void delete_model_stuff();
    void release_interfaces();

public:
    Application(Logger &logger);
    IDirect3DDevice9 * get_device();

    // Adds given model to Application's list of models;
    // creates a physical model if `physical` is true (and returns it);
    PhysicalModel * add_model(Model &model, bool physical = false);
    void set_forces(::CrashAndSqueeze::Core::ForcesArray & forces);
    void run();

    void toggle_wireframe();

    ~Application();

private:
    // No copying!
    Application(const Application&);
    Application &operator=(const Application&);
};

#pragma once
#include "main.h"
#include "Camera.h"
#include "Window.h"
#include "Vertex.h"
#include "Model.h"
#include "performance_reporter.h"
#include <vector>
#include "Core/model.h"
#include "parallel.h"
#include "logger.h"
#include "worker_thread.h"
#include "Stopwatch.h"

extern const unsigned VECTORS_IN_MATRIX;

typedef ::CrashAndSqueeze::Core::Model PhysicalModel;
using ::CrashAndSqueeze::Core::RigidBody;
using ::CrashAndSqueeze::Core::ForcesArray;
using ::CrashAndSqueeze::Math::Real;
using ::CrashAndSqueeze::Math::Vector;
using ::CrashAndSqueeze::Math::Matrix;
using ::CrashAndSqueeze::Core::IRegion;
using ::CrashAndSqueeze::Core::SphericalRegion;

class IntegrateRigidCallback : public ::CrashAndSqueeze::Core::VelocitiesChangedCallback
{
private:
    RigidBody * rigid_body;
public:
    IntegrateRigidCallback(RigidBody * rigid_body) : rigid_body(rigid_body) {}
    virtual void invoke(const Vector & linear_velocity_change, const Vector & angular_velocity_change);
};

class PhysicalModelEntity
{
private:
    AbstractModel         * high_model;
    AbstractModel         * low_model;
    PhysicalModel         * physical_model;
    
    IntegrateRigidCallback  velocities_changed_callback;
    RigidBody               rigid_body;
    Vector                  initial_center;

    Stopwatch               physics_stopwatch;
    PerformanceReporter   * performance_reporter;
    Stopwatch               update_stopwatch;
    PerformanceReporter   * update_performance_reporter;

    WinFactory              prim_factory;

    bool                    is_updating_vertices_on_gpu;

public:
    PhysicalModelEntity(AbstractModel &high_model,
                        AbstractModel &low_model,
                        bool is_updating_vertices_on_gpu,
                        const char *perf_rep_desc,
                        Logger & logger);

    void setup_body(const Vector & position,
                    const Matrix & orientation,
                    const Vector & linear_velocity,
                    const Vector & angular_velocity);
    
    void hit(const IRegion & region, const Vector & velocity_local, const Vector & velocity_global);
    void collide_with(const SphericalRegion & region);
    
    void compute_kinematics(double dt);
    void compute_deformation(ForcesArray * forces, double dt);
    void wait_for_deformation();

    AbstractModel * get_displayed_model(int show_mode);
    void update_geometry(int show_mode);

    void report_performance();

    PhysicalModel * get_physical_model() { return physical_model; }

    ~PhysicalModelEntity();
};

typedef std::vector<PhysicalModelEntity*> ModelEntities;
typedef std::vector<AbstractModel*> AbstractModels;

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

    // default is true
    bool is_updating_vertices_on_gpu;

    bool wireframe;

    bool vertices_update_needed;
    int show_mode;

    bool show_help;

    Window window;

    ModelEntities physical_models;
    AbstractModels visual_only_models;

    ForcesArray * forces;
    bool forces_enabled;

    SphericalRegion * impact_region;
    Vector impact_velocity;
    Vector impact_rot_center;
    int impact_axis; // index of impact rotation axis
    AbstractModel * impact_model;
    void move_impact(const Vector & vector);
    void rotate_impact(const Vector & rotation_axis);
    void move_impact_nearer(Real distance, const Vector & rotation_axis);

    static const int THREADS_COUNT = 2;
    WorkerThread threads[THREADS_COUNT];

    Camera camera;
    D3DXMATRIX post_transform;

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
    void set_shader_matrix3x4(unsigned reg, const D3DXMATRIX &matrix)
    {
        set_shader_const(reg, matrix, VECTORS_IN_MATRIX-1);
    }
    void set_shader_color(unsigned reg, D3DCOLOR color)
    {
        set_shader_const(reg, D3DXCOLOR(color), 1);
    }

    void set_alpha_test();

    void set_show_mode(int new_show_mode);

    void process_key(unsigned code, bool shift, bool ctrl, bool alt);

    void draw_text(const TCHAR * text, RECT rect, D3DCOLOR color, bool align_right = false);
    void draw_text_info();
    void draw_model(AbstractModel * model);
    void render(PerformanceReporter &internal_reporter);

    // Deinitialization steps:
    void stop_threads();
    void delete_model_stuff();
    void release_interfaces();

public:
    Application(Logger &logger);
    IDirect3DDevice9 * get_device();

    // Adds given model to Application's list of models;
    // creates a physical model if `physical` is true (and returns it);
    PhysicalModel * add_physical_model(AbstractModel & high_model, AbstractModel & low_model,
                                       const Vector & linear_velocity = Vector::ZERO,
                                       const Vector & angular_velocity = Vector::ZERO);
    void add_visual_only_model(AbstractModel & model);

    void set_forces(ForcesArray & forces);
    void set_impact(SphericalRegion & region,
                    const Vector &velocity,
                    const Vector &rotation_center,
                    AbstractModel &model);
    void set_updating_vertices_on_gpu(bool value) { is_updating_vertices_on_gpu = value; }

    void run();

    void toggle_wireframe();
    void set_wireframe();
    void unset_wireframe();

    ~Application();

private:
    // No copying!
    Application(const Application&);
    Application &operator=(const Application&);
};

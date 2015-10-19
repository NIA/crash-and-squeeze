#pragma once
#include "main.h"
#include "Camera.h"
#include "Window.h"
#include "Vertex.h"
#include "Model.h"
#include "parallel.h"
#include "logger.h"
#include "worker_thread.h"
#include "Renderer.h"
#include "IInputHandler.h"
#include "settings.h"

class Application : public IInputHandler, public ISettingsHandler, public ControlsWindow::ITextInfo
{
private:
    Logger &logger;

    bool emulation_enabled;
    bool emultate_one_step;
    bool vertices_update_needed;

    SimulationSettings sim_settings;
    GlobalSettings global_settings;
    RenderSettings render_settigns;

    Window window;
    ControlsWindow controls_window;
    Renderer renderer;
    Camera camera;

    ModelEntities model_entities;

    ::CrashAndSqueeze::Core::ForcesArray * forces;
    bool forces_enabled;

    ::CrashAndSqueeze::Core::IRegion * impact_region;
    ::CrashAndSqueeze::Math::Vector impact_velocity;
    ::CrashAndSqueeze::Math::Vector impact_rot_center;
    int impact_axis; // index of impact rotation axis
    Model * impact_model;
    bool impact_happened;
    void move_impact(const ::CrashAndSqueeze::Math::Vector & vector);
    void rotate_impact(const ::CrashAndSqueeze::Math::Real & angle, const ::CrashAndSqueeze::Math::Vector & rotation_axis);
    void move_impact_nearer(const ::CrashAndSqueeze::Math::Real & distance, const ::CrashAndSqueeze::Math::Vector & rotation_axis);

    WinFactory prim_factory;

    PerformanceReporter total_performance_reporter;

    static const int THREADS_COUNT = 4;
    WorkerThread threads[THREADS_COUNT];

    void set_show_mode(int new_show_mode);

    void rotate_models(float phi);

    virtual tstring get_text_info() const override;

    // Deinitialization steps:
    void stop_threads();
    void delete_model_stuff();

public:
    Application(Logger &logger);
    IRenderer* get_renderer();
    Logger & get_logger() const { return logger; }

    // Adds given model to Application's list of models;
    // creates a physical model if `physical` is true (and returns it);
    PhysicalModel * add_model(AbstractModel &high_model, bool physical = false, AbstractModel *low_model = NULL);
    void set_forces(::CrashAndSqueeze::Core::ForcesArray & forces);
    void set_impact(::CrashAndSqueeze::Core::IRegion & region,
                    const ::CrashAndSqueeze::Math::Vector &velocity,
                    const ::CrashAndSqueeze::Math::Vector &rotation_center,
                    Model &model);
    // set axis (0: Oz, 1: Oy, or 2: Ox) around which impact region will be rotated
    void set_impact_rot_axis(int axis_id);
    void set_updating_vertices_on_gpu(bool value) { global_settings.update_vertices_on_gpu = value; }
    void set_camera_position(float rho, float theta, float phi) { camera.set_position(rho, theta, phi); }

    void run();

    // Implement IInputHandler:
    void process_key(unsigned code, bool shift, bool ctrl, bool alt) override;
    void process_mouse_drag(short x, short y, short dx, short dy, bool shift, bool ctrl) override;
    void process_mouse_wheel(short x, short y, short dw, bool shift, bool ctrl) override;

    // Implement ISettingsHandler:
    virtual void set_settings(const SimulationSettings &sim, const GlobalSettings &global, const RenderSettings &render) override;
    virtual void get_settings(SimulationSettings &sim, GlobalSettings &global, RenderSettings &render) const override;

    ~Application();

private:
    DISABLE_COPY(Application)
};

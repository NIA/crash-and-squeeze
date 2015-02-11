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

class Application : public IInputHandler
{
private:
    Logger &logger;

    bool emulation_enabled;
    bool emultate_one_step;

    // default is true
    bool is_updating_vertices_on_gpu;

    bool vertices_update_needed;
    int show_mode;

    bool show_help;

    Window window;
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

    static const int MAX_TEXT_SIZE = 2048;
    TCHAR text_buffer[MAX_TEXT_SIZE];

    static const int THREADS_COUNT = 2;
    WorkerThread threads[THREADS_COUNT];

    void set_show_mode(int new_show_mode);

    void rotate_models(float phi);

    const TCHAR * get_text_info();

    // Deinitialization steps:
    void stop_threads();
    void delete_model_stuff();

public:
    Application(Logger &logger);
    Renderer* get_renderer();

    // Adds given model to Application's list of models;
    // creates a physical model if `physical` is true (and returns it);
    PhysicalModel * add_model(AbstractModel &high_model, bool physical = false, AbstractModel *low_model = NULL);
    void set_forces(::CrashAndSqueeze::Core::ForcesArray & forces);
    void set_impact(::CrashAndSqueeze::Core::IRegion & region,
                    const ::CrashAndSqueeze::Math::Vector &velocity,
                    const ::CrashAndSqueeze::Math::Vector &rotation_center,
                    Model &model);
    void set_updating_vertices_on_gpu(bool value) { is_updating_vertices_on_gpu = value; }
    void set_camera_position(float rho, float theta, float phi) { camera.set_position(rho, theta, phi); }

    void run();

    // Implement IInputHandler:
    void process_key(unsigned code, bool shift, bool ctrl, bool alt) override;
    void process_mouse_drag(short x, short y, short dx, short dy, bool shift, bool ctrl) override;
    void process_mouse_wheel(short x, short y, short dw, bool shift, bool ctrl) override;


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

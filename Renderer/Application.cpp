#include "Application.h"
#include "Stopwatch.h"
#include "matrices.h"
#include <time.h>
#include <sstream>
#include "Parallel\itask_executor.h"

using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;
using CrashAndSqueeze::Math::Real;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Parallel::TaskQueue;
using CrashAndSqueeze::Parallel::AbstractTask;
using CrashAndSqueeze::Parallel::IEventSet;

namespace
{
    const float       CAMERA_ROTATE_SPEED = 3.14f/Window::DEFAULT_WINDOW_SIZE; // when mouse moved to dx pixels, camera angle is changed to dx*CAMERA_ROTATE_SPEED
    const float       WHEEL_ZOOM_SPEED = 0.05f/WHEEL_DELTA; // when wheel is rotated to dw, camera rho is changed to dw*WHEEL_ZOOM_SPEED;

    const Real        HIT_ROTATE_STEP = DirectX::XM_PI/30.0;
    const Real        HIT_MOVE_STEP = 0.06;
    const Real        HIT_ROTATE_SPEED = 2*3.14f/Window::DEFAULT_WINDOW_SIZE;
    const Real        HIT_MOVE_SPEED = 3.0f/Window::DEFAULT_WINDOW_SIZE;
    const Real        HIT_MOVE_WHEEL_SPEED = HIT_MOVE_STEP/WHEEL_DELTA/2;

    const int         ROTATION_AXES_COUNT = 3;
    const Vector      ROTATION_AXES[ROTATION_AXES_COUNT] = {Vector(0,0,1), Vector(0,1,0), Vector(1,0,0)};

    const float       VERTEX_MASS = 1;
    const Real        CLUSTER_PADDING_COEFF = 0.2;

    const unsigned    EACH_MODEL_WORKER_THERAD_MAX_WAIT_MS = 1;

    // max number of models for which normals are generated in parallel (when updating on CPU)
    const unsigned    MAX_PARALLEL_MODELS_GEN_NORMALS = 100;

    const TCHAR *     HELP_TEXT = _T("Keyboard controls:\r\n")
                                  _T("~~~~~~~~~~~~~~~~~~\r\n")
                                  _T("Enter: hit the model,\r\n")
                                  _T("I/J/K/L: move hit area (yellow sphere),\r\n")
                                  _T("Arrows: rotate camera,\r\n")
                                  _T("+/-, PgUp/PgDn: zoom in/out,\r\n")
                                  _T("F2: show settings window,\r\n")
                                  _T("Esc: exit.\r\n\r\n")
                                  _T("Advanced:\r\n")
                                  _T("~~~~~~~~~\r\n")
                                  _T("Tab: switch between current, initial\r\n")
                                  _T("        and equilibrium state,\r\n")
                                  _T("Space: pause/continue emulation,\r\n")
                                  _T("S: emulate one step (when paused),\r\n")
                                  _T("F: toggle forces on/off,\r\n")
                                  _T("W: toggle wireframe on/off,\r\n");

}

// A task source for parallel execution of normals generation
class Application::NormalsGenerator : public CrashAndSqueeze::Parallel::ITaskExecutor {
private:
    class Task : public AbstractTask {
    private:
        AbstractModel * model;
        IEventSet * event_set;
        int event_index;
    protected:
        virtual void execute() override
        {
            model->generate_normals();
            event_set->set(event_index);
        }
    public:
        Task(AbstractModel * model, IEventSet * event_set, int event_index)
            : model(model), event_set(event_set), event_index(event_index) {}

        void set_event_set(IEventSet *new_event_set) {
            event_set = new_event_set;
        }
    };

    std::vector<Task*> tasks;
    CrashAndSqueeze::Parallel::IPrimFactory *prim_factory;
    TaskQueue task_queue;
    IEventSet * all_completed; // NB: is nullptr until the first model added
    bool enabled;
    Logger & logger;

public:
    NormalsGenerator(Logger &logger, CrashAndSqueeze::Parallel::IPrimFactory &prim_factory)
        : logger(logger), prim_factory(&prim_factory),
        task_queue(MAX_PARALLEL_MODELS_GEN_NORMALS, &prim_factory),
        all_completed(nullptr), enabled(false)
    {
    }

    // adds another model for which normals should be generated
    // NB: not reentrant! must be called only from main thread
    // returns true if OK, and false - if MAX_PARALLEL_MODELS_GEN_NORMALS size exceeded => need to switch to sequential
    bool add_model(AbstractModel* model)
    {
        if (tasks.size() >= MAX_PARALLEL_MODELS_GEN_NORMALS)
            return false;

        if (all_completed == nullptr)
        {
            // if it is the first model => create event set
            all_completed = prim_factory->create_event_set(1, true);
        }
        else
        {
            // TODO FIXME !!!!! REWRITE THIS FROM THE GROUND UP: move normal generation to Core, do this from inside update_vertices if asked, use the same task_queue
            // + this would allow to make complete and correct vector updating without the need of graphic_transform: vectors can be expressed as linear combination of normal and tangent, each other updated separately
            // 
            // + think about depending tasks: now they block the entire _WORKER_ because the wait is placed inside task.execute. This logic should be managed from inside task queue to allow maximum CPU usage


            // if not => replace existing to enlarge it
            all_completed->wait();
            prim_factory->destroy_event_set(all_completed);
            all_completed = prim_factory->create_event_set(tasks.size()+1, true);
            for (auto* task : tasks)
            {
                task->set_event_set(all_completed);
            }
        }
        Task * task = new Task(model, all_completed, tasks.size());
        tasks.push_back(task);
        return true;
    }

    // is this normals generator already used in worker threads?
    bool is_enabled() {
        return enabled;
    }

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
        abort();
    }

    void generate_normals_async()
    {
        if (all_completed != nullptr) {
            all_completed->wait();
            all_completed->unset();
        }
        // initiate all tasks again
        for (int i = 0; i < tasks.size(); ++i)
        {
            bool notify_workers = (i == tasks.size() - 1); // notify workers only after adding last task
            task_queue.push(tasks[i], notify_workers);
        }
    }

    void wait_for_normals() {
        if (all_completed != nullptr) {
            all_completed->wait();
        }
    }

    // is called from worker thread to wait until have any task (reentrant)
    virtual void wait_for_tasks() override
    {
        task_queue.wait_for_tasks();
    }

    virtual bool wait_for_tasks(unsigned milliseconds) override
    {
        return task_queue.wait_for_tasks(milliseconds);
    }

    virtual bool complete_next_task() override
    {
        AbstractTask *task;
        if (nullptr != (task = task_queue.pop()))
        {
            logger.add_message("Normals vvvv STARTED vvvvv");
            task->complete();
            logger.add_message("Normals ^^^^ FINISHED ^^^^");
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual void abort() override
    {
        task_queue.clear();
    }

    ~NormalsGenerator() {
        abort();
        if (all_completed != nullptr)
            prim_factory->destroy_event_set(all_completed);
        for (auto* task : tasks) {
            delete_pointer(task);
        }
    }
    private:
        DISABLE_COPY(NormalsGenerator);
};


Application::Application(Logger &logger) :
    window(Window::DEFAULT_WINDOW_SIZE, Window::DEFAULT_WINDOW_SIZE),
    renderer(window, &camera),
    emulation_enabled(true), emultate_one_step(true), forces_enabled(false),
    vertices_update_needed(false), impact_region(NULL), impact_happened(false),
    forces(NULL), logger(logger), impact_model(NULL), prim_factory(false),
    normals_generator(new NormalsGenerator(logger, prim_factory)),
    impact_axis(0), total_performance_reporter(logger, "total")
{
    sim_settings.set_defaults(); // TODO: load from config file
    global_settings.set_defaults();
    render_settigns.set_defaults();
    set_settings(sim_settings, global_settings, render_settigns);

    // TODO: is it good? Seems like it can affect performance (see MSDN for SetThreadAffinityMask)
    if(NULL == SetThreadAffinityMask(GetCurrentThread(), 0x0000001)) // restrict main thread to 1st processor so that QueryPerformanceCounter works correctly
    {
        throw AffinityError();
    }
}

tstring Application::get_text_info() const
{
    std::basic_ostringstream<TCHAR> big_text;
    big_text.precision(2);
    big_text <<
        _T("Crash-And-Squeeze version ") _T(CAS_VERSION) _T("\r\n")
        _T("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n")
        _T("Simulation: ") <<  (emulation_enabled ? _T("ON") : _T("OFF")) << _T("\r\n")
        _T("Show: ") << RenderSettings::SHOW_MODES_CAPTIONS[render_settigns.show_mode] << _T("\r\n\r\n")
        _T("Performance: ") << int(total_performance_reporter.get_last_fps()) << _T(" FPS ")
                            << std::fixed << total_performance_reporter.get_last_measurement()*1000 << _T(" ms/frame)\r\n")
        _T("Models:\r\n")
        _T("~~~~~~\r\n");
    for (auto& me: model_entities)
    {
        if (nullptr != me.physical_model)
        {
            big_text <<
                me.low_model->get_vertices_count() << _T(" low-vertices\r\n") <<
                me.high_model->get_vertices_count() << _T(" high-vertices\r\n") <<
                me.physical_model->get_clusters_num() << _T("=") <<
                    // TODO: get clusters_by_axes from each model separately (by now they all have the same clusters count)
                    global_settings.clusters_by_axes[0] << _T("x") <<
                    global_settings.clusters_by_axes[1] << _T("x") <<
                    global_settings.clusters_by_axes[2] << _T(" clusters\r\n\r\n");
        }
    }
    big_text << HELP_TEXT;

    return big_text.str().c_str();
}

IRenderer* Application::get_renderer()
{
    return &renderer;
}

PhysicalModel * Application::add_model(AbstractModel &high_model, bool physical, AbstractModel *low_model)
{
    ModelEntity model_entity = {NULL};

    model_entity.high_model = &high_model;

    if(physical)
    {
        if(NULL == low_model)
            throw NullPointerError();

        // lock not only for read but also for write, because PhysicalModel constructor will update cluster indices
        Vertex * high_vertices = high_model.lock_vertex_buffer(LOCK_READ_WRITE);
        Vertex * low_vertices = low_model->lock_vertex_buffer(LOCK_READ_WRITE);
        model_entity.physical_model =
            new PhysicalModel(low_vertices,
                              low_model->get_vertices_count(),
                              VERTEX_INFO,
                              
                              high_vertices,
                              high_model.get_vertices_count(),
                              VERTEX_INFO,
                              
                              global_settings.clusters_by_axes,
                              CLUSTER_PADDING_COEFF,

                              VERTEX_MASS,
                              NULL,

                              &prim_factory);
        model_entity.physical_model->set_simulation_params(sim_settings);
        
        high_model.unlock_vertex_buffer();
        low_model->unlock_vertex_buffer();

        model_entity.low_model = low_model;

        for (auto& thread: threads)
        {
            if (!thread.is_started())
                thread.start(model_entity.physical_model, EACH_MODEL_WORKER_THERAD_MAX_WAIT_MS, &logger);
            else
                thread.add_executor(model_entity.physical_model);
        }
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
        // if updating on CPU + quadratic extensions => enable parallel generation of normals for this high_model
        if (!global_settings.update_vertices_on_gpu && normals_generator != nullptr)
        {
            bool success = normals_generator->add_model(&high_model);
            if (success) {
                if (!normals_generator->is_enabled()) {
                    for (auto& thread : threads)
                    {
                        thread.add_executor(normals_generator);
                        normals_generator->enable();
                    }
                }
            }
            else 
            {
                // if limit of parallel models with normals exceeded => switch from parallel to sequential normals generation (bad)
                normals_generator->disable();
                delete_pointer(normals_generator); // destruct generator set the pointer to nullptr
            }
        }
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

        static const int BUFFER_SIZE = 128;
        char description[BUFFER_SIZE];
        static_assert(3 == GlobalSettings::AXES_COUNT, "the following logging code assumes AXES_COUNT==3");
        sprintf_s(description, BUFFER_SIZE, "%i low-vertices (mapped on %i hi-vertices) in %i=%ix%ix%i clusters on %i threads",
                                            low_model->get_vertices_count(), high_model.get_vertices_count(),
                                            global_settings.get_total_clusters_num(), global_settings.clusters_by_axes[0], global_settings.clusters_by_axes[1], global_settings.clusters_by_axes[2],
                                            THREADS_COUNT);
        model_entity.performance_reporter = new PerformanceReporter(logger, description);
    }
    else
    {
        model_entity.low_model = NULL;
        model_entity.physical_model = NULL;
        model_entity.performance_reporter = NULL;
    }

    model_entities.push_back( model_entity );
    return model_entity.physical_model;
}

void Application::set_forces(ForcesArray & forces)
{
    this->forces = & forces;
}

void Application::set_impact(::CrashAndSqueeze::Core::IRegion & region,
                             const Vector &velocity,
                             const Vector &rotation_center,
                             Model & model)
{
    if(NULL == impact_model)
    {
        impact_region = & region;
        impact_velocity = velocity;
        add_model(model);
        impact_model = & model;
        impact_rot_center = rotation_center;
    }
}

void Application::set_impact_rot_axis(int axis_id)
{
    // protection so that impact_axis is inside correct range
    impact_axis = (axis_id + ROTATION_AXES_COUNT)%ROTATION_AXES_COUNT;
}

void Application::move_impact(const Vector &vector)
{
    if(NULL != impact_model)
    {
        impact_model->move(math_vector_to_float3(vector));
        impact_region->move(vector);
    }
}

Vector rotate_vector(const Vector & vector, const Vector & rotation_axis, Real angle)
{
    Vector axis = rotation_axis.normalized();
    Vector direction = vector.normalized();
    Vector normal = ::CrashAndSqueeze::Math::cross_product(direction, axis).normalized();
    Vector direction_radial = ::CrashAndSqueeze::Math::cross_product(axis, normal).normalized();
    Real v_radial = vector.project_to(direction_radial);

    Real step = 2*v_radial*sin(angle/2);
    return vector - step*sin(angle/2)*direction_radial + step*cos(angle/2)*normal;
}

void Application::rotate_impact(const Real & angle, const Vector & rotation_axis)
{
    Vector old_pos = impact_region->get_center();
    Vector new_pos = rotate_vector(old_pos - impact_rot_center, rotation_axis, angle) + impact_rot_center;
    move_impact(new_pos - old_pos);
    impact_velocity = rotate_vector(impact_velocity, rotation_axis, angle);
}

void Application::move_impact_nearer(const Real & dist, const Vector & rotation_axis)
{
    Vector to_center = impact_rot_center - impact_region->get_center();
    Vector direction;
    to_center.project_to(rotation_axis, &direction);
    if( dist > 0 && direction.norm() < HIT_MOVE_STEP )
    {
        return;
    }
    move_impact(direction.normalized()*dist);
}

void Application::rotate_models(float phi)
{
    for (auto& model_entity: model_entities)
    {
        model_entity.high_model->rotate(phi);
        
        if(NULL != model_entity.low_model)
            model_entity.low_model->rotate(phi);
    }
}

void Application::set_show_mode(int new_show_mode)
{
    if (render_settigns.show_mode != new_show_mode)
    {
        render_settigns.show_mode = new_show_mode;
        vertices_update_needed = true;
    }
}

void Application::process_key(unsigned code, bool shift, bool ctrl, bool alt)
{
    UNREFERENCED_PARAMETER(ctrl);
    UNREFERENCED_PARAMETER(alt);

    switch( code )
    {
    case VK_ESCAPE:
        PostQuitMessage( 0 );
        break;
    case VK_UP:
        camera.move_up();
        break;
    case VK_DOWN:
        camera.move_down();
        break;
    case VK_PRIOR:
    case VK_ADD:
    case VK_OEM_PLUS:
        camera.move_nearer();
        break;
    case VK_NEXT:
    case VK_SUBTRACT:
    case VK_OEM_MINUS:
        camera.move_farther();
        break;
    case VK_LEFT:
        camera.move_clockwise();
        break;
    case VK_RIGHT:
        camera.move_counterclockwise();
        break;
    case 'I':
        move_impact(-HIT_MOVE_STEP*ROTATION_AXES[impact_axis]);
        break;
    case 'K':
        move_impact(HIT_MOVE_STEP*ROTATION_AXES[impact_axis]);
        break;
    case 'J':
        rotate_impact(HIT_ROTATE_STEP, ROTATION_AXES[impact_axis]);
        break;
    case 'L':
        rotate_impact(-HIT_ROTATE_STEP, ROTATION_AXES[impact_axis]);
        break;
    case 'U':
        move_impact_nearer(-HIT_MOVE_STEP, ROTATION_AXES[impact_axis]);
        break;
    case 'O':
        move_impact_nearer(HIT_MOVE_STEP, ROTATION_AXES[impact_axis]);
        break;
    case 'H':
        set_impact_rot_axis(impact_axis+1); // TODO: also rotate impact velocity // TODO: display impact velocity as arrow :)
        break;
    case '1':
        set_show_mode(0);
        break;
    case '2':
        set_show_mode(1);
        break;
    case '3':
        set_show_mode(2);
        break;
    case '4':
        set_show_mode(3);
        break;
    case VK_SPACE:
        // toggle emulation enabled
        process_command_emulation_on(!emulation_enabled);
        break;
    case 'F':
        forces_enabled = !forces_enabled;
        for(int i = 0; i < forces->size(); ++i)
            (*forces)[i]->toggle();
        break;
    case 'C':
        // toggle last
        if(0 != forces->size())
            (*forces)[forces->size() - 1]->toggle();
        break;
    case 'S':
        process_command_step();
        break;
    case 'T':
        renderer.toggle_alpha_test();
        break;
    case 'W':
        renderer.toggle_wireframe();
        break;
    case VK_TAB:
        set_show_mode( ( render_settigns.show_mode + (shift ? - 1 : 1) + RenderSettings::_SHOW_MODES_COUNT )%RenderSettings::_SHOW_MODES_COUNT );
        break;
    case VK_RETURN:
        impact_happened = true;
        break;
    case VK_F2:
        controls_window.show();
    }
}


void Application::process_mouse_drag(short x, short y, short dx, short dy, bool shift, bool ctrl)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(ctrl);

    if (shift)
    {
        move_impact(dx*HIT_MOVE_SPEED*ROTATION_AXES[impact_axis]);
        rotate_impact(dy*HIT_ROTATE_SPEED, ROTATION_AXES[impact_axis]);
    }
    else 
    {
        camera.change_phi(dx*CAMERA_ROTATE_SPEED);
        camera.change_theta(-dy*CAMERA_ROTATE_SPEED);
    }
}

void Application::process_mouse_wheel(short x, short y, short dw, bool shift, bool ctrl)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(ctrl);

    if (shift)
    {
        move_impact_nearer(HIT_MOVE_WHEEL_SPEED*dw, ROTATION_AXES[impact_axis]);
    }
    else
    {
        camera.change_rho(-dw*WHEEL_ZOOM_SPEED); // minus so that rotating up zooms in
    }
}


void Application::process_command_emulation_on(bool on /*= true*/)
{
    emulation_enabled = on;
}

void Application::process_command_step()
{
    emulation_enabled = false;
    emultate_one_step = true;
}

void Application::set_settings(const SimulationSettings &sim, const GlobalSettings &global, const RenderSettings &render)
{
    sim_settings = sim;
    for (auto &model_entity: model_entities)
    {
        if(NULL != model_entity.physical_model)
            model_entity.physical_model->set_simulation_params(sim_settings);
    }
    global_settings = global;
    render_settigns = render;
    renderer.set_wireframe(render_settigns.wireframe);
    set_show_mode(render_settigns.show_mode);
}

void Application::get_settings(SimulationSettings &sim, GlobalSettings &global, RenderSettings &render) const 
{
    sim = sim_settings;
    global = global_settings;
    render = render_settigns;
}

void Application::run()
{
    window.set_input_handler(this);
    window.show();
    window.update();
    controls_window.create(window, this, this, this);
    controls_window.show();
    
    Stopwatch stopwatch;
    Stopwatch total_stopwatch;
    PerformanceReporter render_performance_reporter(logger, "rendering");
    PerformanceReporter update_performance_reporter(logger, "updating");
    PerformanceReporter gen_normals_performance_reporter(logger, "generate_normals");
    PerformanceReporter internal_render_performance_reporter(logger, "swap_chain->Present");

    int physics_frames = 0;
    
    if(NULL == forces)
    {
        throw ForcesError();
    }
    for(int i = 0; i < forces->size(); ++i)
    {
        if(forces_enabled)
            (*forces)[i]->activate();
        else
            (*forces)[i]->deactivate();
    }
    
    // Enter the message loop
    MSG msg;
    ZeroMemory( &msg, sizeof( msg ) );
    while( msg.message != WM_QUIT )
    {
        if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            total_stopwatch.start();

            // ----- physics -----
            // TODO: variable dt
            double dt = 0.01;
            if (emulation_enabled || emultate_one_step)
            {
                simulate(dt);

                ++physics_frames;
                impact_happened = false;
                emultate_one_step = false;
                vertices_update_needed = true;
            }
            // ----- physics --> graphics -----
            if(vertices_update_needed)
            {
                update_vertices(update_performance_reporter, gen_normals_performance_reporter);
                vertices_update_needed = false;
            }
            // ----- graphics -----
            stopwatch.start();
            renderer.render(model_entities, internal_render_performance_reporter, global_settings.update_vertices_on_gpu, RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode);
            // (frame finished)

            render_performance_reporter.add_measurement(stopwatch.stop());
            total_performance_reporter.add_measurement(total_stopwatch.stop());
        }
    }

    // -- report performance results --

    for (auto& model_entity: model_entities)
    {
        if( NULL != model_entity.performance_reporter )
            model_entity.performance_reporter->report_results();
    }
    update_performance_reporter.report_results();
    gen_normals_performance_reporter.report_results();
    render_performance_reporter.report_results();
    internal_render_performance_reporter.report_results();
    total_performance_reporter.report_results();

    stop_threads();

    logger.dump_messages();
}

void Application::simulate(double dt) {
    // for each model entity: 
    // ------- STARTING STEP.... -------------
    for (auto& model_entity : model_entities)
    {
        PhysicalModel       * physical_model = model_entity.physical_model;

        if (NULL != physical_model)
        {
            Vector linear_velocity_change, angular_velocity_chage;
            model_entity.stopwatch.start();

            if (global_settings.update_vertices_on_gpu) {
                // as an optimization, when updating on GPU, step is computed in parallel with rendering, so we should wait for it before starting new (see below)
                if (false == physical_model->wait_for_step())
                {
                    throw PhysicsError();
                }
            }

            if (impact_happened && NULL != impact_region)
            {
                physical_model->hit(*impact_region, impact_velocity);
            }
            logger.add_message("Tasks --READY--");
            physical_model->compute_next_step_async(*forces, dt, NULL);
        }
    }
    // for each model entity: 
    // ------- ...REACT TO EVENTS.... -------------
    for (auto& model_entity : model_entities)
    {
        PhysicalModel       * physical_model = model_entity.physical_model;
        if (NULL != physical_model)
        {
            physical_model->react_to_events();
        }
    }
    // for each model entity: 
    // ------- ...FINISHING STEP -------------
    for (auto& model_entity : model_entities)
    {
        PhysicalModel       * physical_model = model_entity.physical_model;
        PerformanceReporter * performance_reporter = model_entity.performance_reporter;

        if (NULL != physical_model)
        {
            if (global_settings.update_vertices_on_gpu) {
                // as an optimization, when updating on GPU, we can wait only for cluster tasks (wait_for_clusters)
                // and continue to rendering (because only cluster matrices are needed to start rendering when updating on GPU).
                if (false == physical_model->wait_for_clusters())
                {
                    throw PhysicsError();
                }
                logger.add_message("Clusters ~~finished~~");
            }
            else {
                if (false == physical_model->wait_for_step())
                {
                    throw PhysicsError();
                }
                logger.add_message("Step **finished**");
            }

            double time = model_entity.stopwatch.stop();

            if (NULL != performance_reporter)
            {
                performance_reporter->add_measurement(time);
            }
        }
    }

}

void Application::update_vertices(PerformanceReporter &update_performance_reporter, PerformanceReporter &gen_normals_performance_reporter)
{
    // for each model entity:
    // ------- STARTING UPDATING.... -------------
    for (auto& model_entity : model_entities)
    {
        PhysicalModel * physical_model = model_entity.physical_model;

        if (NULL != physical_model)
        {
            AbstractModel *model;
            if (RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode)
                model = model_entity.high_model;
            else
                model = model_entity.low_model;

            if (!(global_settings.update_vertices_on_gpu && RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode))
            {
                Vertex *vertices = model->lock_vertex_buffer(LOCK_READ_WRITE);
                int vertices_count = model->get_vertices_count();

                // make sure that step is finished
                physical_model->wait_for_step();

                switch (render_settigns.show_mode)
                {
                case RenderSettings::SHOW_GRAPHICAL_VERTICES:
                    model_entity.stopwatch.start();
                    logger.add_message("Update tasks --READY--");
                    physical_model->update_vertices_async(vertices, VERTEX_INFO, 0, vertices_count);

                    break;
                case RenderSettings::SHOW_CURRENT_POSITIONS:
                    physical_model->update_current_positions(vertices, vertices_count, VERTEX_INFO);
                    break;
                case RenderSettings::SHOW_EQUILIBRIUM_POSITIONS:
                    physical_model->update_equilibrium_positions(vertices, vertices_count, VERTEX_INFO);
                    break;
                case RenderSettings::SHOW_INITIAL_POSITIONS:
                    physical_model->update_initial_positions(vertices, vertices_count, VERTEX_INFO);
                    break;
                }

            }
        }
    }
    // for each model entity:
    // ------- ...FINISHING UPDATING -------------
    for (auto& model_entity : model_entities)
    {
        PhysicalModel * physical_model = model_entity.physical_model;

        if (NULL != physical_model)
        {
            AbstractModel *model;
            if (RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode)
                model = model_entity.high_model;
            else
                model = model_entity.low_model;
            if (!(global_settings.update_vertices_on_gpu && RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode))
            {
                if (render_settigns.show_mode == RenderSettings::SHOW_GRAPHICAL_VERTICES)
                {
                    if (false == physical_model->wait_for_update())
                    {
                        throw PhysicsError();
                    }
                    update_performance_reporter.add_measurement(model_entity.stopwatch.stop());
                    logger.add_message("Update **finished**");
                }
                model->unlock_vertex_buffer();
            }
        }
    }

#if CAS_QUADRATIC_EXTENSIONS_ENABLED
    // if show_graphical + quadratic extensions:
    // ------- GENERATE NORMALS -------------
    if (RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode)
    {
        if (normals_generator != nullptr)
        {
            // parallel normals generation
            Stopwatch stopwatch;
            stopwatch.start();
            normals_generator->generate_normals_async();
            normals_generator->wait_for_normals();
            gen_normals_performance_reporter.add_measurement(stopwatch.stop());
        }
        else
        {
            // sequential normals generation
            for (auto& model_entity : model_entities)
            {
                if (nullptr != model_entity.physical_model)
                {
                    model_entity.stopwatch.start();
                    model_entity.high_model->generate_normals();
                    gen_normals_performance_reporter.add_measurement(model_entity.stopwatch.stop());
                }
            }
        }
    }
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

    // finally:
    // ------- ...NOTIFY SUBSCRIBERS -------------
    for (auto& model_entity : model_entities)
    {
        if (NULL != model_entity.physical_model)
        {
            if (RenderSettings::SHOW_GRAPHICAL_VERTICES == render_settigns.show_mode)
                model_entity.high_model->notify_subscriber();
            else
                model_entity.low_model->notify_subscriber();
        }
    }

}

void Application::stop_threads()
{
    for (auto& thread: threads)
    {
        thread.stop();
    }
    for (auto& model_entity: model_entities)
    {
        if(NULL != model_entity.physical_model)
            model_entity.physical_model->wait_for_step();
    }
}

void Application::delete_model_stuff()
{
    delete_pointer(normals_generator);
    for (auto& model_entity: model_entities)
    {
        delete_pointer(model_entity.physical_model);
        delete_pointer(model_entity.performance_reporter);
    }
}

Application::~Application()
{
    stop_threads();
    delete_model_stuff();
}

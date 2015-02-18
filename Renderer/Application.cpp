#include "Application.h"
#include "Stopwatch.h"
#include "matrices.h"
#include <time.h>

using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;
using CrashAndSqueeze::Math::Real;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Parallel::TaskQueue;
using CrashAndSqueeze::Parallel::AbstractTask;

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
    const int         CLUSTERS_BY_AXES[VECTOR_SIZE] = {2, 3, 4};
    const int         TOTAL_CLUSTERS_COUNT = CLUSTERS_BY_AXES[0]*CLUSTERS_BY_AXES[1]*CLUSTERS_BY_AXES[2];
    const Real        CLUSTER_PADDING_COEFF = 0.2;

    const TCHAR *     SHOW_MODES_CAPTIONS[Application::_SHOW_MODES_COUNT] = 
                      {
                          _T("Show: Graphical"),
                          _T("Show: Current"),
                          _T("Show: Equilibrium"),
                          _T("Show: Initial"),
                      };

    const TCHAR *     HELP_TEXT = _T("Welcome to Crash-And-Squeeze Demo!\n\n")
                                  _T("Keys:\n\n")
                                  _T("Enter: hit the model,\n")
                                  _T("I/J/K/L: move hit area (yellow sphere),\n")
                                  _T("Arrows: rotate camera,\n")
                                  _T("+/-, PgUp/PgDn: zoom in/out,\n")
                                  _T("F1: display/hide this help,\n")
                                  _T("Esc: exit.\n\n")
                                  _T("Advanced:\n\n")
                                  _T("Tab: switch between current, initial\n")
                                  _T("        and equilibrium state,\n")
                                  _T("Space: pause/continue emulation,\n")
                                  _T("S: emulate one step (when paused),\n")
                                  _T("F: toggle forces on/off,\n")
                                  _T("W: toggle wireframe on/off,\n")
                                  _T("T: toggle alpha test of/off.\n");


}


Application::Application(Logger &logger) :
    window(Window::DEFAULT_WINDOW_SIZE, Window::DEFAULT_WINDOW_SIZE),
    renderer(window, &camera),
    emulation_enabled(true), emultate_one_step(true), forces_enabled(false),
    vertices_update_needed(false), impact_region(NULL), impact_happened(false),
    forces(NULL), logger(logger), show_help(false), impact_model(NULL), prim_factory(false),
    impact_axis(0), is_updating_vertices_on_gpu(true)
{
    set_show_mode(SHOW_GRAPHICAL_VERTICES);
    // TODO: is it good? Seems like it can affect performance (see MSDN for SetThreadAffinityMask)
    SetThreadAffinityMask(GetCurrentThread(), 0x0000000); // restrict main thread to 1st processor so that QueryPerformanceCounter works correctly
}


const TCHAR* Application::get_text_info()
{
    if(show_help)
    {
        return HELP_TEXT;
    }
    else
    {
        const TCHAR * emulation_text = emulation_enabled ? _T("Emulation: ON") : _T("Emulation: OFF");
        _stprintf_s(text_buffer, _T("%s\n%s\nPress F1 for help"), SHOW_MODES_CAPTIONS[show_mode], emulation_text);
        return text_buffer;
    }
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
                              
                              CLUSTERS_BY_AXES,
                              CLUSTER_PADDING_COEFF,

                              &prim_factory,

                              NULL,
                              VERTEX_MASS);
        
        high_model.unlock_vertex_buffer();
        low_model->unlock_vertex_buffer();

        model_entity.low_model = low_model;

        for(int i = 0; i < THREADS_COUNT; ++i)
        {
            // TODO: Oops, it will fail for two or more physical models
            threads[i].start(model_entity.physical_model, &logger);
        }

        static const int BUFFER_SIZE = 128;
        char description[BUFFER_SIZE];
        sprintf_s(description, BUFFER_SIZE, "%i low-vertices (mapped on %i hi-vertices) in %i=%ix%ix%i clusters on %i threads",
                                            low_model->get_vertices_count(), high_model.get_vertices_count(),
                                            TOTAL_CLUSTERS_COUNT, CLUSTERS_BY_AXES[0], CLUSTERS_BY_AXES[1], CLUSTERS_BY_AXES[2],
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
    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        (*iter).high_model->rotate(phi);
        
        if(NULL != (*iter).low_model)
            (*iter).low_model->rotate(phi);
    }
}

void Application::set_show_mode(int new_show_mode)
{
    show_mode = new_show_mode;
    vertices_update_needed = true;
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
        impact_axis = (impact_axis+1)%ROTATION_AXES_COUNT; // TODO: also rotate impact velocity // TODO: display impact velocity as arrow :)
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
        emulation_enabled = !emulation_enabled;
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
        emultate_one_step = true;
        break;
    case 'T':
        renderer.toggle_alpha_test();
        break;
    case 'W':
        renderer.toggle_wireframe();
        break;
    case VK_TAB:
        set_show_mode( ( show_mode + (shift ? - 1 : 1) + _SHOW_MODES_COUNT )%_SHOW_MODES_COUNT );
        break;
    case VK_RETURN:
        impact_happened = true;
        break;
    case VK_F1:
        show_help = !show_help;
        break;
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

void Application::run()
{
    window.set_input_handler(this);
    window.show();
    window.update();
    
    Stopwatch stopwatch;
    Stopwatch total_stopwatch;
    PerformanceReporter render_performance_reporter(logger, "rendering");
    PerformanceReporter update_performance_reporter(logger, "updating");
    PerformanceReporter gen_normals_performance_reporter(logger, "generate_normals");
    PerformanceReporter total_performance_reporter(logger, "total");
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
            // physics
            
            // TODO: variable dt
            double dt = 0.01;
            if(emulation_enabled || emultate_one_step)
            {
                // for each model entity
                for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
                {
                    PhysicalModel       * physical_model       = (*iter).physical_model;
                    PerformanceReporter * performance_reporter = (*iter).performance_reporter;
                    
                    if( NULL != physical_model )
                    {
                        Vector linear_velocity_change, angular_velocity_chage;

                        stopwatch.start();
                        if(false == physical_model->wait_for_step())
                        {
                            throw PhysicsError();
                        }

                        logger.add_message("Step **finished**");

                        if(impact_happened && NULL != impact_region)
                        {
                            physical_model->hit(*impact_region, impact_velocity);
                            impact_happened = false;
                        }
                        physical_model->prepare_tasks(*forces, dt, NULL);
                        logger.add_message("Tasks --READY--");

                        physical_model->react_to_events();

                        if(false == physical_model->wait_for_clusters())
                        {
                            throw PhysicsError();
                        }

                        double time = stopwatch.stop();
                        logger.add_message("Clusters ~~finished~~");

                        if( NULL != performance_reporter )
                        {
                            performance_reporter->add_measurement(time);
                        }
                    }
                }
                ++physics_frames;
                emultate_one_step = false;
                vertices_update_needed = true;
            }

            if(vertices_update_needed)
            {
                // for each model entity
                for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
                {
                    PhysicalModel       * physical_model       = (*iter).physical_model;
                    
                    if( NULL != physical_model )
                    {
                        AbstractModel *model;
                        if(SHOW_GRAPHICAL_VERTICES == show_mode)
                            model = (*iter).high_model;
                        else
                            model = (*iter).low_model;

                        if( ! (is_updating_vertices_on_gpu && SHOW_GRAPHICAL_VERTICES == show_mode) )
                        {
                            Vertex *vertices = model->lock_vertex_buffer(LOCK_READ_WRITE);
                            int vertices_count = model->get_vertices_count();

                            switch(show_mode)
                            {
                            case SHOW_GRAPHICAL_VERTICES:
                                stopwatch.start();
                                physical_model->update_vertices(vertices, vertices_count, VERTEX_INFO);
                                update_performance_reporter.add_measurement(stopwatch.stop());
                                break;
                            case SHOW_CURRENT_POSITIONS:
                                physical_model->update_current_positions(vertices, vertices_count, VERTEX_INFO);
                                break;
                            case SHOW_EQUILIBRIUM_POSITIONS:
                                physical_model->update_equilibrium_positions(vertices, vertices_count, VERTEX_INFO);
                                break;
                            case SHOW_INITIAL_POSITIONS:
                                physical_model->update_initial_positions(vertices, vertices_count, VERTEX_INFO);
                                break;
                            }

                            model->unlock_vertex_buffer();
                            #if CAS_QUADRATIC_EXTENSIONS_ENABLED
                            stopwatch.start();
                            model->generate_normals();
                            gen_normals_performance_reporter.add_measurement(stopwatch.stop());
                            #endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
                            model->notify_subscriber();
                        }
                    }
                }

                vertices_update_needed = false;
            }
            
            // graphics
            renderer.set_text_to_draw(get_text_info());
            stopwatch.start();
            renderer.render(model_entities, internal_render_performance_reporter, is_updating_vertices_on_gpu, SHOW_GRAPHICAL_VERTICES == show_mode);
            render_performance_reporter.add_measurement(stopwatch.stop());
            total_performance_reporter.add_measurement(total_stopwatch.stop());
        }
    }

    // -- report performance results --

    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        if( NULL != (*iter).performance_reporter )
            (*iter).performance_reporter->report_results();
    }
    update_performance_reporter.report_results();
    gen_normals_performance_reporter.report_results();
    render_performance_reporter.report_results();
    internal_render_performance_reporter.report_results();
    total_performance_reporter.report_results();

    stop_threads();
}


void Application::stop_threads()
{
    for (int i = 0; i < THREADS_COUNT; ++i)
    {
        threads[i].stop();
    }
    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        if(NULL != (*iter).physical_model)
            (*iter).physical_model->wait_for_step();
    }
}

void Application::delete_model_stuff()
{
    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        if(NULL != (*iter).physical_model)
            delete (*iter).physical_model;

        if(NULL != (*iter).performance_reporter)
            delete (*iter).performance_reporter;
    }
}

Application::~Application()
{
    delete_model_stuff();
}

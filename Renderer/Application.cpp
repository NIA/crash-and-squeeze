#include "Application.h"
#include "Stopwatch.h"
#include "matrices.h"

using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Math::Matrix;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;
using CrashAndSqueeze::Math::Real;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Parallel::TaskQueue;
using CrashAndSqueeze::Parallel::AbstractTask;

const unsigned VECTORS_IN_MATRIX = sizeof(D3DXMATRIX)/sizeof(D3DXVECTOR4);

namespace
{
    const int         WINDOW_SIZE = 600;
    const D3DCOLOR    BACKGROUND_COLOR = D3DCOLOR_XRGB( 5, 5, 10 );
    const D3DCOLOR    TEXT_COLOR = D3DCOLOR_XRGB( 255, 255, 0 );
    const int         TEXT_HEIGHT = 20;
    const int         TEXT_MARGIN = 10;
    const int         TEXT_SPACING = 0;
    const int         TEXT_WIDTH = WINDOW_SIZE - 2*TEXT_MARGIN;
    const int         TEXT_LINE_HEIGHT = TEXT_HEIGHT + TEXT_SPACING;
    const bool        INITIAL_WIREFRAME_STATE = true;
    const D3DCOLOR    BLACK = D3DCOLOR_XRGB( 0, 0, 0 );
    const Real        ROTATE_STEP = D3DX_PI/30.0;
    const Real        MOVE_STEP = 0.06;
    const Vector      VERTICAL_AXIS(0,0,1);
    const float       VERTEX_MASS = 1;
    const int         CLUSTERS_BY_AXES[VECTOR_SIZE] = {2, 2, 4};
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
    
    const char * RELEASE_OR_DEBUG = 
#ifdef NDEBUG
        "Release"
#else
        "Debug"
#endif
        ;

    inline bool is_key_pressed(int virtual_key)
    {
        return ::GetAsyncKeyState(virtual_key) < 0;
    }

    struct MyRect : public RECT
    {
        MyRect(LONG x = 0, LONG y = 0, LONG w = 0, LONG h = 0) { left = x; top = y; right = x + w; bottom = y + h; }
        LONG width() { return right - left; }
        LONG height() { return bottom - top; }
    };

    void build_d3d_matrix(Matrix transformation, Vector pos, /*out*/ D3DXMATRIX &out_matrix)
    {
        const int LAST = VECTORS_IN_MATRIX - 1; // index of last row/col in D3DXMATRIX

        // copy transformation
        for(int i = 0; i < VECTOR_SIZE; ++i)
            for(int j = 0; j < VECTOR_SIZE; ++j)
                out_matrix.m[i][j] = static_cast<float>(transformation.get_at(i, j));

        // copy position
        for(int i = 0; i < VECTOR_SIZE; ++i)
            out_matrix.m[i][LAST] = static_cast<float>(pos[i]);

        // fill last line
        for(int j = 0; j < VECTOR_SIZE; ++j)
            out_matrix.m[LAST][j] = 0;
        
        out_matrix.m[LAST][LAST] = 1;
    }
}

namespace
{
    //---------------- SHADER CONSTANTS ---------------------------
    //    c0-c3 is the view matrix
    const unsigned    SHADER_REG_VIEW_MX = 0;
    //    c12 is directional light vector
    const unsigned    SHADER_REG_DIRECTIONAL_VECTOR = 12;
    const D3DXVECTOR3 SHADER_VAL_DIRECTIONAL_VECTOR  (0.5f, -1.0f, 0.3f);
    //    c13 is directional light color
    const unsigned    SHADER_REG_DIRECTIONAL_COLOR = 13;
    const D3DCOLOR    SHADER_VAL_DIRECTIONAL_COLOR = D3DCOLOR_XRGB(230, 230, 230);
    //    c14 is diffuse coefficient
    const unsigned    SHADER_REG_DIFFUSE_COEF = 14;
    const float       SHADER_VAL_DIFFUSE_COEF = 0.8f;
    //    c15 is ambient color
    const unsigned    SHADER_REG_AMBIENT_COLOR = 15;
    const D3DCOLOR    SHADER_VAL_AMBIENT_COLOR = D3DCOLOR_XRGB(80, 80, 80);
    //    c16 is point light color
    const unsigned    SHADER_REG_POINT_COLOR = 16;
    const D3DCOLOR    SHADER_VAL_POINT_COLOR = D3DCOLOR_XRGB(120, 250, 250);
    //    c17 is point light position
    const unsigned    SHADER_REG_POINT_POSITION = 17;
    const D3DXVECTOR3 SHADER_VAL_POINT_POSITION  (-1.6f, 0.0f, 0.8f);
    //    c18 are attenuation constants
    const unsigned    SHADER_REG_ATTENUATION = 18;
    const D3DXVECTOR3 SHADER_VAL_ATTENUATION  (1.0f, 0, 0.8f);
    //    c19 is specular coefficient
    const unsigned    SHADER_REG_SPECULAR_COEF = 19;
    const float       SHADER_VAL_SPECULAR_COEF = 0.3f;
    //    c20 is specular constant 'f'
    const unsigned    SHADER_REG_SPECULAR_F = 20;
    const float       SHADER_VAL_SPECULAR_F = 25.0f;
    //    c21 is eye position
    const unsigned    SHADER_REG_EYE = 21;
    //    c22-c25 is position and rotation of model matrix
    const unsigned    SHADER_REG_POS_AND_ROT_MX = 22;
    //    c26-c49 are 24 initial centers of mass for 24 clusters
    const unsigned    SHADER_REG_CLUSTER_INIT_CENTER = 26;
    //    c50-c121 are 24 3x4 cluster matrices => 72 vectors
    const unsigned    SHADER_REG_CLUSTER_MATRIX = 50;
    //    c122-c124 are ZEROS! (3x4 zero matrix)
    const D3DMATRIX   ZEROS = {0};
    //    c125-c196 are 24 3x4 cluster matrices for normal transformation
    const unsigned    SHADER_REG_CLUSTER_NORMAL_MATRIX = 125;
    //    c197-c199 are ZEROS! (3x4 zero matrix)
}

Application::Application(Logger &logger) :
    d3d(NULL), device(NULL), window(WINDOW_SIZE, WINDOW_SIZE), camera(1.8f, 1.1f, -1.2f), // Constants selected for better view of the scene
    directional_light_enabled(true), point_light_enabled(true), spot_light_enabled(false), ambient_light_enabled(true),
    emulation_enabled(true), forces_enabled(false), emultate_one_step(false), alpha_test_enabled(false),
    vertices_update_needed(false), impact_region(NULL), impact_happened(false), wireframe(INITIAL_WIREFRAME_STATE),
    forces(NULL), logger(logger), font(NULL), show_help(false), impact_model(NULL), prim_factory(false)
{

    try
    {
        init_device();
        set_show_mode(SHOW_GRAPHICAL_VERTICES);
        init_font();
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

void Application::init_device()
{
    d3d = Direct3DCreate9( D3D_SDK_VERSION );
    if( d3d == NULL )
        throw D3DInitError();

    // Set up the structure used to create the device
    D3DPRESENT_PARAMETERS present_parameters;
    ZeroMemory( &present_parameters, sizeof( present_parameters ) );
    present_parameters.Windowed = TRUE;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    present_parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    // Set default adapter and device settings
    UINT adapter_to_use = D3DADAPTER_DEFAULT;
    D3DDEVTYPE device_type = D3DDEVTYPE_HAL;
    // Look for 'NVIDIA PerfHUD' adapter
    // If it is present, override default settings
    for (UINT adapter = 0; adapter < d3d->GetAdapterCount(); ++adapter)
    {
        D3DADAPTER_IDENTIFIER9 identifier;
        if( FAILED( d3d->GetAdapterIdentifier(adapter, 0, &identifier) ) )
            throw D3DInitError();
        
        if (strstr(identifier.Description,"PerfHUD") != 0)
        {
            adapter_to_use = adapter;
            device_type = D3DDEVTYPE_REF;
            break;
        }
    }
    // Create the device
    if( FAILED( d3d->CreateDevice( adapter_to_use, device_type, window,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                   &present_parameters, &device ) ) )
        throw D3DInitError();
    
    // Configure alpha-test
    check_state( device->SetRenderState( D3DRS_ALPHAREF, (DWORD)0xffffffff ) );
    check_state( device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE ) );
    // Configure alpha-blending
    check_state( device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ) );
    check_state( device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ) );
    check_state( device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE ) );

    toggle_wireframe();
    set_alpha_test();
}

void Application::init_font()
{
    if( FAILED( D3DXCreateFont(device, TEXT_HEIGHT, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &font) ) )
        throw D3DXFontError();
}

void  Application::set_alpha_test()
{
    if(alpha_test_enabled)
    {
        check_state( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL) );
    }
    else
    {
        check_state( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS) );
    }
}


void Application::render(PerformanceReporter &internal_reporter)
{
    Stopwatch stopwatch;
    check_render( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, BACKGROUND_COLOR, 1.0f, 0 ) );
    
    // Begin the scene
    check_render( device->BeginScene() );
    // Setting constants
    set_shader_matrix( SHADER_REG_VIEW_MX,            camera.get_matrix());
    
    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        AbstractModel * display_model = (SHOW_GRAPHICAL_VERTICES == show_mode || NULL == (*iter).low_model) ? (*iter).high_model : (*iter).low_model;
        PhysicalModel * physical_model = (*iter).physical_model;

        // Set up
        ( display_model->get_shader() ).set();

        set_shader_matrix( SHADER_REG_POS_AND_ROT_MX, display_model->get_transformation() );
        
        if(NULL != physical_model)
        {
            // step of 3 vectors between consequent 3x4 matrices
            int step = VECTORS_IN_MATRIX-1;
            int clusters_num = physical_model->get_clusters_num();
            // for each cluster...
            for(int i = 0; i < clusters_num; ++i)
            {
                // ...set initial center of mass...
                D3DXVECTOR3 init_pos = math_vector_to_d3dxvector(physical_model->get_cluster_initial_center(i));
                set_shader_vector( SHADER_REG_CLUSTER_INIT_CENTER + i, init_pos);
                
                // ...and transformation matrices for positions...
                D3DXMATRIX cluster_matrix;
                build_d3d_matrix(physical_model->get_cluster_transformation(i), physical_model->get_cluster_center(i), cluster_matrix);
                set_shader_matrix3x4( SHADER_REG_CLUSTER_MATRIX + step*i, cluster_matrix);

                // ...and normals
                build_d3d_matrix(physical_model->get_cluster_normal_transformation(i), Vector::ZERO, cluster_matrix);
                set_shader_matrix3x4( SHADER_REG_CLUSTER_NORMAL_MATRIX + step*i, cluster_matrix);
            }
            set_shader_matrix3x4( SHADER_REG_CLUSTER_MATRIX + step*clusters_num, ZEROS );
            set_shader_matrix3x4( SHADER_REG_CLUSTER_NORMAL_MATRIX + step*clusters_num, ZEROS );
        }
        
        display_model->draw();
    }

    // End the scene
    check_render( device->EndScene() );
    
    stopwatch.start();
    // Present the backbuffer contents to the display
    check_render( device->Present( NULL, NULL, NULL, NULL ) );
    internal_reporter.add_measurement(stopwatch.get_time());
}

void Application::draw_text_info()
{
    if(show_help)
    {
        draw_text(HELP_TEXT, MyRect(TEXT_MARGIN, TEXT_MARGIN, TEXT_WIDTH, WINDOW_SIZE), TEXT_COLOR);
    }
    else
    {
        // Show mode
        int previous_text_bottom = TEXT_MARGIN;
        draw_text(SHOW_MODES_CAPTIONS[show_mode], MyRect(TEXT_MARGIN, previous_text_bottom, TEXT_WIDTH, TEXT_HEIGHT), TEXT_COLOR);
        previous_text_bottom += TEXT_LINE_HEIGHT;
        // Emulation enabled?
        const TCHAR * emulation_text = emulation_enabled ? _T("Emulation: ON") : _T("Emulation: OFF");
        draw_text(emulation_text, MyRect(TEXT_MARGIN, previous_text_bottom, TEXT_WIDTH, TEXT_HEIGHT), TEXT_COLOR);
        previous_text_bottom += TEXT_LINE_HEIGHT;
        // F1 for help
        draw_text(_T("Press F1 for help"), MyRect(TEXT_MARGIN, previous_text_bottom, TEXT_WIDTH, TEXT_HEIGHT), TEXT_COLOR);
    }
}

void Application::draw_text(const TCHAR * text, RECT rect, D3DCOLOR color, bool align_right)
{
    DWORD format_flags = 0;
    if(align_right)
        format_flags |= DT_RIGHT;

    if( 0 == font->DrawText(NULL, text, -1, &rect, format_flags, color) )
        throw RenderError();
}

IDirect3DDevice9 * Application::get_device()
{
    return device;
}

PhysicalModel * Application::add_model(AbstractModel &high_model, bool physical, AbstractModel *low_model)
{
    ModelEntity model_entity = {NULL};

    model_entity.high_model = &high_model;

    if(physical)
    {
        if(NULL == low_model)
            throw NullPointerError();

        Vertex * high_vertices = high_model.lock_vertex_buffer();
        Vertex * low_vertices = low_model->lock_vertex_buffer();
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

        model_entity.performance_reporter = new PerformanceReporter(logger, "clusters");
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
                             const Vector &rotation_center)
{
    if(NULL == impact_model)
    {
        impact_region = & region;
        impact_velocity = velocity;
        impact_rot_center = rotation_center;
    }
}

void Application::move_impact(const Vector &vector)
{
    if(NULL != impact_model)
    {
        impact_model->move(math_vector_to_d3dxvector(vector));
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

void Application::rotate_impact(const Vector & rotation_axis)
{
    Vector old_pos = impact_region->get_center();
    Vector new_pos = rotate_vector(old_pos - impact_rot_center, rotation_axis, ROTATE_STEP) + impact_rot_center;
    move_impact(new_pos - old_pos);
    impact_velocity = rotate_vector(impact_velocity, rotation_axis, ROTATE_STEP);
}

void Application::move_impact_nearer(Real dist, const Vector & rotation_axis)
{
    Vector to_center = impact_rot_center - impact_region->get_center();
    Vector direction;
    to_center.project_to(rotation_axis, &direction);
    if( dist > 0 && direction.norm() < MOVE_STEP )
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
        move_impact(Vector(0,0,-MOVE_STEP));
        break;
    case 'K':
        move_impact(Vector(0,0,MOVE_STEP));
        break;
    case 'J':
        rotate_impact(VERTICAL_AXIS);
        break;
    case 'L':
        rotate_impact(-VERTICAL_AXIS);
        break;
    case 'U':
        move_impact_nearer(-MOVE_STEP, VERTICAL_AXIS);
        break;
    case 'O':
        move_impact_nearer(MOVE_STEP, VERTICAL_AXIS);
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
        alpha_test_enabled = !alpha_test_enabled;
        set_alpha_test();
        break;
    case 'W':
        toggle_wireframe();
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

void Application::run(double duration_sec)
{
    window.show();
    window.update();
    
    Stopwatch stopwatch;
    Stopwatch total_stopwatch;
    PerformanceReporter render_performance_reporter(logger, "render  ");
    PerformanceReporter total_performance_reporter(logger, "total   ");
    PerformanceReporter internal_render_performance_reporter(logger, "present ");

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
    
    MSG msg;
    ZeroMemory( &msg, sizeof( msg ) );
    Stopwatch duration_stopwatch;
    duration_stopwatch.start();
    // Enter the message loop
    while( msg.message != WM_QUIT )
    {
        double time_elapsed = duration_stopwatch.get_time();
        if( duration_sec > 0 && time_elapsed >= duration_sec )
        {
            break;
        }

        if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
        {
            if( msg.message == WM_KEYDOWN )
            {
                process_key( static_cast<unsigned>( msg.wParam ), is_key_pressed(VK_SHIFT),
                             is_key_pressed(VK_CONTROL), is_key_pressed(VK_MENU) );
            }

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
                        if(false == physical_model->wait_for_step())
                        {
                            throw PhysicsError();
                        }

                        logger.add_message("Step **finished**");

                        // TODO: should move this after preparing tasks, to do this in parallel with clusters computing?
                        // Should then use a lock to avoid race condition between this and last task? (unlikely, because it
                        // will probably finish before, but possible)
                        if(impact_happened && NULL != impact_region)
                        {
                            physical_model->hit(*impact_region, impact_velocity);
                            impact_happened = false;
                        }

                        stopwatch.start();
                        physical_model->prepare_tasks(*forces, dt, NULL);
                        logger.add_message("Tasks --READY--");

                        physical_model->react_to_events();

                        if(false == physical_model->wait_for_clusters())
                        {
                            throw PhysicsError();
                        }

                        double time = stopwatch.get_time();
                        logger.add_message("Clusters ~~finished~~");

                        if( NULL != performance_reporter )
                        {
                            performance_reporter->add_measurement(time);
                        }
                    }
                }
                ++physics_frames;
                emultate_one_step = false;

                if(SHOW_GRAPHICAL_VERTICES != show_mode)
                    vertices_update_needed = true;
            }

            if(vertices_update_needed)
            {
                // for each model entity
                for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
                {
                    PhysicalModel       * physical_model       = (*iter).physical_model;
                    AbstractModel       * low_model            = (*iter).low_model;
                    
                    if( NULL != physical_model )
                    {
                        Vertex *low_vertices = low_model->lock_vertex_buffer();

                        switch(show_mode)
                        {
                        case SHOW_GRAPHICAL_VERTICES:
                            // update not needed, since it is made on GPU
                            break;
                        case SHOW_CURRENT_POSITIONS:
                            physical_model->update_current_positions(low_vertices, low_model->get_vertices_count(), VERTEX_INFO);
                            break;
                        case SHOW_EQUILIBRIUM_POSITIONS:
                            physical_model->update_equilibrium_positions(low_vertices, low_model->get_vertices_count(), VERTEX_INFO);
                            break;
                        case SHOW_INITIAL_POSITIONS:
                            physical_model->update_initial_positions(low_vertices, low_model->get_vertices_count(), VERTEX_INFO);
                            break;
                        }

                        low_model->unlock_vertex_buffer();
                    }
                }

                vertices_update_needed = false;
            }
            
            // graphics
            stopwatch.start();
            render(internal_render_performance_reporter);
            render_performance_reporter.add_measurement(stopwatch.get_time());
            total_performance_reporter.add_measurement(total_stopwatch.get_time());
        }
    }

    // -- report performance results --

    static const int BUFFER_SIZE = 512;
    char header[BUFFER_SIZE];

    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        if( NULL != iter->physical_model )
        {
            sprintf_s(header, BUFFER_SIZE,
                      "Performance for %i low-vertices (mapped on %i hi-vertices) in %i=%ix%ix%i clusters on %i threads [%s]",
                      iter->physical_model->get_vertices_num(), iter->high_model->get_vertices_count(),
                      TOTAL_CLUSTERS_COUNT, CLUSTERS_BY_AXES[0], CLUSTERS_BY_AXES[1], CLUSTERS_BY_AXES[2],
                      THREADS_COUNT, RELEASE_OR_DEBUG);
            logger.log("        [Renderer]", header);
        }
        if( NULL != (*iter).performance_reporter )
        {
            (*iter).performance_reporter->report_results();
        }
    }
    render_performance_reporter.report_results();
    internal_render_performance_reporter.report_results();
    total_performance_reporter.report_results();

    stop_threads();
}

void Application::toggle_wireframe()
{
    if( wireframe )
    {
        unset_wireframe();
    }
    else
    {
        set_wireframe();
    }
}

void Application::set_wireframe()
{
    wireframe = true;
    check_state( device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ) );
}

void Application::unset_wireframe()
{
    wireframe = false;
    check_state( device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) );
}

void Application::stop_threads()
{
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

void Application::release_interfaces()
{
    release_interface( d3d );
    release_interface( device );
    release_interface( font );
}

Application::~Application()
{
    delete_model_stuff();
    release_interfaces();
}

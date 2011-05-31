#include "Application.h"
#include "matrices.h"
#include <time.h>

using CrashAndSqueeze::Math::Matrix;
using CrashAndSqueeze::Math::VECTOR_SIZE;
using CrashAndSqueeze::Core::IndexArray;
using CrashAndSqueeze::Parallel::TaskQueue;
using CrashAndSqueeze::Parallel::AbstractTask;

const unsigned VECTORS_IN_MATRIX = sizeof(D3DXMATRIX)/sizeof(D3DXVECTOR4);

namespace
{
    const int         WINDOW_SIZE = 1000;
    const D3DCOLOR    BACKGROUND_COLOR = D3DCOLOR_XRGB( 255, 255, 255 );
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
    const int         ROTATION_AXES_COUNT = 3;
    const Vector      ROTATION_AXES[ROTATION_AXES_COUNT] = {Vector(0,0,1), Vector(0,1,0), Vector(1,0,0)};
    const float       VERTEX_MASS = 1;
    const int         CLUSTERS_BY_AXES[VECTOR_SIZE] = {2, 2, 6};
    const int         TOTAL_CLUSTERS_COUNT = CLUSTERS_BY_AXES[0]*CLUSTERS_BY_AXES[1]*CLUSTERS_BY_AXES[2];
    const Real        CLUSTER_PADDING_COEFF = 0.2;
    // a value of friction force, divided by mass
    // (you may think that it is a combination of constatns mu*g: F/m = mu*N/m = mu*m*g/m = mu*g)
    const Real        FRICTION_ACC_VALUE = 0.25;

    enum ShowMode
    {
        SHOW_GRAPHICAL_VERTICES,
        SHOW_CURRENT_POSITIONS,
        SHOW_EQUILIBRIUM_POSITIONS,
        SHOW_INITIAL_POSITIONS,
        _SHOW_MODES_COUNT
    };

    const TCHAR *     SHOW_MODES_CAPTIONS[_SHOW_MODES_COUNT] = 
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

    void build_d3d_matrix(const Matrix & transformation, Vector pos, /*out*/ D3DXMATRIX & out_matrix)
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

    //---------------- SHADER CONSTANTS ---------------------------
    //    c0-c3 is the view matrix
    const unsigned    SHADER_REG_VIEW_MX = 0;
    //    c12 is directional light vector
    const unsigned    SHADER_REG_DIRECTIONAL_VECTOR = 12;
    const D3DXVECTOR3 SHADER_VAL_DIRECTIONAL_VECTOR  (0.5f, 0.5f, 1.0f);
    //    c13 is directional light color
    const unsigned    SHADER_REG_DIRECTIONAL_COLOR = 13;
    const D3DCOLOR    SHADER_VAL_DIRECTIONAL_COLOR = D3DCOLOR_XRGB(230, 230, 230);
    //    c14 is diffuse coefficient
    const unsigned    SHADER_REG_DIFFUSE_COEF = 14;
    const float       SHADER_VAL_DIFFUSE_COEF = 1.0f;
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

void IntegrateRigidCallback::invoke(const Vector & linear_velocity_change, const Vector & angular_velocity_change)
{
    rigid_body->add_to_linear_velocity(linear_velocity_change);
    rigid_body->add_to_angular_velocity(angular_velocity_change);
}

Application::Application(Logger &logger) :
    d3d(NULL), device(NULL), window(WINDOW_SIZE, WINDOW_SIZE), camera(6.1f, 1.1f, -1.16858f), // Constants selected for better view of the scene
    directional_light_enabled(true), point_light_enabled(true), spot_light_enabled(false), ambient_light_enabled(true),
    emulation_enabled(true), forces_enabled(false), emultate_one_step(false), alpha_test_enabled(false),
    vertices_update_needed(false), impact_region(NULL), impact_happened(false), wireframe(INITIAL_WIREFRAME_STATE),
    forces(NULL), logger(logger), font(NULL), show_help(false), impact_model(NULL), post_transform(rotate_x_matrix(D3DX_PI/2)),
    impact_axis(0), is_updating_vertices_on_gpu(true)
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

void Application::draw_model(AbstractModel * model)
{
    // Set up
    ( model->get_shader() ).set();

    set_shader_matrix( SHADER_REG_POS_AND_ROT_MX, post_transform*model->get_transformation() );
        
    model->draw();
}

void Application::render(PerformanceReporter &internal_reporter)
{
    Stopwatch stopwatch;
    check_render( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, BACKGROUND_COLOR, 1.0f, 0 ) );
    
    // Begin the scene
    check_render( device->BeginScene() );
    // Setting constants
    D3DXVECTOR3 directional_vector;
    D3DXVec3Normalize(&directional_vector, &SHADER_VAL_DIRECTIONAL_VECTOR);

    D3DCOLOR ambient_color = ambient_light_enabled ? SHADER_VAL_AMBIENT_COLOR : BLACK;
    D3DCOLOR directional_color = directional_light_enabled ? SHADER_VAL_DIRECTIONAL_COLOR : BLACK;
    D3DCOLOR point_color = point_light_enabled ? SHADER_VAL_POINT_COLOR : BLACK;

    set_shader_matrix( SHADER_REG_VIEW_MX,            camera.get_matrix());
    set_shader_vector( SHADER_REG_DIRECTIONAL_VECTOR, directional_vector);
    set_shader_color(  SHADER_REG_DIRECTIONAL_COLOR,  directional_color);
    set_shader_float(  SHADER_REG_DIFFUSE_COEF,       SHADER_VAL_DIFFUSE_COEF);
    set_shader_color(  SHADER_REG_AMBIENT_COLOR,      ambient_color);
    set_shader_color(  SHADER_REG_POINT_COLOR,        point_color);
    set_shader_point(  SHADER_REG_POINT_POSITION,     SHADER_VAL_POINT_POSITION);
    set_shader_vector( SHADER_REG_ATTENUATION,        SHADER_VAL_ATTENUATION);
    set_shader_float(  SHADER_REG_SPECULAR_COEF,      SHADER_VAL_SPECULAR_COEF);
    set_shader_float(  SHADER_REG_SPECULAR_F,         SHADER_VAL_SPECULAR_F);
    set_shader_point(  SHADER_REG_EYE,                camera.get_eye());
    

    if( ! physical_models.empty() && NULL != impact_model )
    {
        PhysicalModelEntity *model_entity = physical_models.front(); // first model
        impact_model->attach_to( model_entity->get_displayed_model(show_mode) );
    }

    for (ModelEntities::iterator iter = physical_models.begin(); iter != physical_models.end(); ++iter )
    {
        PhysicalModelEntity * model_entity = *iter;
        AbstractModel * displayed_model   = model_entity->get_displayed_model(show_mode);
        PhysicalModel * physical_model    = model_entity->get_physical_model();

        // if updating displayed vertices on GPU then setup matrices for it
        if(is_updating_vertices_on_gpu)
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
        draw_model( displayed_model );
    }
    for (AbstractModels::iterator iter = visual_only_models.begin(); iter != visual_only_models.end(); ++iter)
    {
        draw_model( *iter );
    }

    // Draw text info
    draw_text_info();

    // End the scene
    check_render( device->EndScene() );
    
    stopwatch.start();
    // Present the backbuffer contents to the display
    check_render( device->Present( NULL, NULL, NULL, NULL ) );
    internal_reporter.add_measurement(stopwatch.stop());
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

void Application::add_visual_only_model(AbstractModel &model)
{
    visual_only_models.push_back(&model);
}

PhysicalModel * Application::add_physical_model(AbstractModel & high_model, AbstractModel & low_model)
{
    static const int BUFFER_SIZE = 128;
    char description[BUFFER_SIZE];
    sprintf_s(description, BUFFER_SIZE, "%i low-vertices (mapped on %i hi-vertices) in %i=%ix%ix%i clusters on %i threads",
                                        low_model.get_vertices_count(), high_model.get_vertices_count(),
                                        TOTAL_CLUSTERS_COUNT, CLUSTERS_BY_AXES[0], CLUSTERS_BY_AXES[1], CLUSTERS_BY_AXES[2],
                                        THREADS_COUNT);

    PhysicalModelEntity * model_entity = new PhysicalModelEntity(high_model,
                                                                 low_model,
                                                                 is_updating_vertices_on_gpu,
                                                                 description,
                                                                 logger);
    PhysicalModel * physical_model = model_entity->get_physical_model();
 
    physical_models.push_back( model_entity );

    for(int i = 0; i < THREADS_COUNT; ++i)
    {
        // TODO: Oops, it will fail for two or more physical models
        threads[i].start(physical_model, &logger);
    }

    return physical_model;
}

void Application::set_forces(ForcesArray & forces)
{
    this->forces = & forces;
}

void Application::set_impact(::CrashAndSqueeze::Core::IRegion & region,
                             const Vector &velocity,
                             const Vector &rotation_center,
                             AbstractModel & model)
{
    if(NULL == impact_model)
    {
        impact_region = & region;
        impact_velocity = velocity;
        add_visual_only_model(model);
        impact_model = & model;
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
        rotate_impact(ROTATION_AXES[impact_axis]);
        break;
    case 'L':
        rotate_impact(-ROTATION_AXES[impact_axis]);
        break;
    case 'U':
        move_impact_nearer(-MOVE_STEP, ROTATION_AXES[impact_axis]);
        break;
    case 'O':
        move_impact_nearer(MOVE_STEP, ROTATION_AXES[impact_axis]);
        break;
    case 'H':
        impact_axis = (impact_axis+1)%ROTATION_AXES_COUNT;
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

void Application::run()
{
    window.show();
    window.update();
    
    Stopwatch stopwatch;
    Stopwatch total_stopwatch;
    PerformanceReporter render_performance_reporter(logger, "rendering");
    PerformanceReporter total_performance_reporter(logger, "total");
    PerformanceReporter internal_render_performance_reporter(logger, "device->Present");

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
                for (ModelEntities::iterator iter = physical_models.begin(); iter != physical_models.end(); ++iter )
                {
                    PhysicalModelEntity * model_entity = *iter;

                    stopwatch.start();
                    model_entity->wait_for_deformation();
                    logger.add_message("Step **finished**");

                    // do some kinematics and interaction meanwhile
                    model_entity->compute_kinematics(dt);
                    if(impact_happened && NULL != impact_region)
                    {
                        model_entity->hit(*impact_region, impact_velocity);
                        impact_happened = false;
                    }

                    // then start next step
                    model_entity->compute_deformation(forces, dt);
                    logger.add_message("Clusters ~~finished~~");
                }
                ++physics_frames;
                emultate_one_step = false;
                vertices_update_needed = true;
            }

            if(vertices_update_needed)
            {
                // for each model entity
                for (ModelEntities::iterator iter = physical_models.begin(); iter != physical_models.end(); ++iter )
                {
                    PhysicalModelEntity * model_entity = *iter;
                    model_entity->update_geometry(show_mode);
                }

                vertices_update_needed = false;
            }
            
            // graphics
            stopwatch.start();
            render(internal_render_performance_reporter);
            render_performance_reporter.add_measurement(stopwatch.stop());
            total_performance_reporter.add_measurement(total_stopwatch.stop());
        }
    }

    // -- report performance results --

    for (ModelEntities::iterator iter = physical_models.begin(); iter != physical_models.end(); ++iter )
    {
        PhysicalModelEntity * model_entity = *iter;
        model_entity->report_performance();
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
    for (int i = 0; i < THREADS_COUNT; ++i)
    {
        threads[i].stop();
    }
    for (ModelEntities::iterator iter = physical_models.begin(); iter != physical_models.end(); ++iter )
    {
        PhysicalModelEntity * model_entity = *iter;
        model_entity->wait_for_deformation();
    }
}

void Application::delete_model_stuff()
{
    for (ModelEntities::iterator iter = physical_models.begin(); iter != physical_models.end(); ++iter )
    {
        PhysicalModelEntity * model_entity = *iter;
        delete model_entity;
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

PhysicalModelEntity::PhysicalModelEntity(AbstractModel & high_model,
                                         AbstractModel & low_model,
                                         bool is_updating_vertices_on_gpu,
                                         const char *perf_rep_desc,
                                         Logger & logger)
: high_model(&high_model), low_model(&low_model),
  velocities_changed_callback(&rigid_body),
  is_updating_vertices_on_gpu(is_updating_vertices_on_gpu), prim_factory(false)
{
    // Create physical model
    Vertex * high_vertices = high_model.lock_vertex_buffer();
    Vertex * low_vertices = low_model.lock_vertex_buffer();
    physical_model =
        new PhysicalModel(low_vertices,
                          low_model.get_vertices_count(),
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
    low_model.unlock_vertex_buffer();

    // create performance reporter
    performance_reporter = new PerformanceReporter(logger, perf_rep_desc);
    update_performance_reporter = new PerformanceReporter(logger, "updating");
}

void PhysicalModelEntity::wait_for_deformation()
{
    physics_stopwatch.start();
    if(false == physical_model->wait_for_step())
    {
        throw PhysicsError();
    }
}

void PhysicalModelEntity::hit(const ::CrashAndSqueeze::Core::IRegion &region, const Vector & velocity)
{
    physical_model->hit(region, velocity);
}

void PhysicalModelEntity::compute_kinematics(double dt)
{
    Vector friction_linear_acc = Vector::ZERO;
    Vector friction_angular_acc = Vector::ZERO;
    // friction
    if(FRICTION_ACC_VALUE > 0)
    {
        Vector linear_velocity = rigid_body.get_linear_velocity();
        Vector angular_velocity = rigid_body.get_angular_velocity();
        if( linear_velocity.norm() < FRICTION_ACC_VALUE*dt )
        {
            friction_linear_acc = - linear_velocity/dt;
        }
        else
        {
            friction_linear_acc = - linear_velocity.normalized()*FRICTION_ACC_VALUE;
        }
        // coef 1/8 is rather arbitrary, because it is true only for linear stick
        Real friction_angular_acc_value = 1.0/8*FRICTION_ACC_VALUE*physical_model->get_size(2);
        if( angular_velocity.norm() < friction_angular_acc_value*dt )
        {
            friction_angular_acc = - angular_velocity/dt;
        }
        else
        {
            friction_angular_acc = - angular_velocity.normalized()*friction_angular_acc_value;
        }
    }

    // do computation
    rigid_body.integrate(dt, friction_linear_acc, friction_angular_acc);
    
    // update model properties
    high_model->set_position(math_vector_to_d3dxvector(rigid_body.get_position()));
    D3DXMATRIX rotation;
    build_d3d_matrix(rigid_body.get_orientation(), Vector::ZERO, rotation);
    high_model->set_rotation(rotation);
}

void PhysicalModelEntity::compute_deformation(ForcesArray * forces, double dt)
{
    physical_model->prepare_tasks(*forces, dt, &velocities_changed_callback);

    physical_model->react_to_events();

    if(false == physical_model->wait_for_clusters())
    {
        throw PhysicsError();
    }
    performance_reporter->add_measurement(physics_stopwatch.stop());
}

AbstractModel * PhysicalModelEntity::get_displayed_model(int show_mode)
{
    AbstractModel *displayed_model;
    if(SHOW_GRAPHICAL_VERTICES == show_mode)
        displayed_model = high_model;
    else
        displayed_model = low_model;
    return displayed_model;
}

void PhysicalModelEntity::update_geometry(int show_mode)
{
    AbstractModel *displayed_model = get_displayed_model(show_mode);

    if( ! (is_updating_vertices_on_gpu && SHOW_GRAPHICAL_VERTICES == show_mode) )
    {
        Vertex *vertices = displayed_model->lock_vertex_buffer();
        int vertices_count = displayed_model->get_vertices_count();

        switch(show_mode)
        {
        case SHOW_GRAPHICAL_VERTICES:
            update_stopwatch.start();
            physical_model->update_vertices(vertices, vertices_count, VERTEX_INFO);
            update_performance_reporter->add_measurement(update_stopwatch.stop());
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

        displayed_model->unlock_vertex_buffer();
    }
}

void PhysicalModelEntity::report_performance()
{
    performance_reporter->report_results();
    update_performance_reporter->report_results();
}

PhysicalModelEntity::~PhysicalModelEntity()
{
    delete physical_model;
    delete performance_reporter;
    delete update_performance_reporter;
}

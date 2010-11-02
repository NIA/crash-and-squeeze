#include "Application.h"
#include "Stopwatch.h"
#include <time.h>

using CrashAndSqueeze::Core::ForcesArray;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;
using CrashAndSqueeze::Math::Real;

const unsigned VECTORS_IN_MATRIX = sizeof(D3DXMATRIX)/sizeof(D3DXVECTOR4);

namespace
{
    const int         WINDOW_SIZE = 600;
    const D3DCOLOR    BACKGROUND_COLOR = D3DCOLOR_XRGB( 5, 5, 10 );
    const bool        INITIAL_WIREFRAME_STATE = true;
    const D3DCOLOR    BLACK = D3DCOLOR_XRGB( 0, 0, 0 );
    const float       ROTATE_STEP = D3DX_PI/30.0f;
    const float       VERTEX_MASS = 1;
    const int         CLUSTERS_BY_AXES[VECTOR_SIZE] = {2, 2, 8};
    const int         TOTAL_CLUSTERS_COUNT = CLUSTERS_BY_AXES[0]*CLUSTERS_BY_AXES[1]*CLUSTERS_BY_AXES[2];
    const Real        CLUSTER_PADDING_COEFF = 0.2;

    //---------------- SHADER CONSTANTS ---------------------------
    //    c0-c3 is the view matrix
    const unsigned    SHADER_REG_VIEW_MX = 0;
    //    c12 is directional light vector
    const unsigned    SHADER_REG_DIRECTIONAL_VECTOR = 12;
    const D3DXVECTOR3 SHADER_VAL_DIRECTIONAL_VECTOR  (0, 1.0f, 0.8f);
    //    c13 is directional light color
    const unsigned    SHADER_REG_DIRECTIONAL_COLOR = 13;
    const D3DCOLOR    SHADER_VAL_DIRECTIONAL_COLOR = D3DCOLOR_XRGB(204, 178, 25);
    //    c14 is diffuse coefficient
    const unsigned    SHADER_REG_DIFFUSE_COEF = 14;
    const float       SHADER_VAL_DIFFUSE_COEF = 0.7f;
    //    c15 is ambient color
    const unsigned    SHADER_REG_AMBIENT_COLOR = 15;
    const D3DCOLOR    SHADER_VAL_AMBIENT_COLOR = D3DCOLOR_XRGB(13, 33, 13);
    //    c16 is point light color
    const unsigned    SHADER_REG_POINT_COLOR = 16;
    const D3DCOLOR    SHADER_VAL_POINT_COLOR = D3DCOLOR_XRGB(25, 153, 153);
    //    c17 is point light position
    const unsigned    SHADER_REG_POINT_POSITION = 17;
    const D3DXVECTOR3 SHADER_VAL_POINT_POSITION  (0.2f, -0.91f, -1.1f);
    //    c18 are attenuation constants
    const unsigned    SHADER_REG_ATTENUATION = 18;
    const D3DXVECTOR3 SHADER_VAL_ATTENUATION  (1.0f, 0, 0.5f);
    //    c19 is specular coefficient
    const unsigned    SHADER_REG_SPECULAR_COEF = 19;
    const float       SHADER_VAL_SPECULAR_COEF = 0.4f;
    //    c20 is specular constant 'f'
    const unsigned    SHADER_REG_SPECULAR_F = 20;
    const float       SHADER_VAL_SPECULAR_F = 15.0f;
    //    c21 is eye position
    const unsigned    SHADER_REG_EYE = 21;
    //    c22 is spot light position
    const unsigned    SHADER_REG_SPOT_POSITION = 22;
    const D3DXVECTOR3 SHADER_VAL_SPOT_POSITION  (1.5f, 1.5f, -1.3f);
    //    c23 is spot light color
    const unsigned    SHADER_REG_SPOT_COLOR = 23;
    const D3DCOLOR    SHADER_VAL_SPOT_COLOR = D3DCOLOR_XRGB(255, 0, 180);
    //    c24 is spot light direction
    const unsigned    SHADER_REG_SPOT_VECTOR = 24;
    const D3DXVECTOR3 SHADER_VAL_SPOT_VECTOR  (1.0f, 1.0f, -0.5f);
    D3DXVECTOR3 spot_vector(2.0f, -0.7f, -1.1f);
    //    c25 is 1/(IN - OUT)
    const float       SHADER_VAL_SPOT_INNER_ANGLE = D3DX_PI/16.0f;
    const float       SHADER_VAL_SPOT_OUTER_ANGLE = D3DX_PI/12.0f;
    const unsigned    SHADER_REG_SPOT_X_COEF = 25;
    //    c26 is OUT/(IN - OUT)
    const unsigned    SHADER_REG_SPOT_CONST_COEF = 26;
    //    c27-c30 is position and rotation of model matrix
    const unsigned    SHADER_REG_POS_AND_ROT_MX = 27;
}

Application::Application(Logger &logger) :
    d3d(NULL), device(NULL), window(WINDOW_SIZE, WINDOW_SIZE), camera(3.3f, 1.0f, 0.97f), // Constants selected for better view of cylinder
    directional_light_enabled(true), point_light_enabled(true), spot_light_enabled(true), ambient_light_enabled(true),
    emulation_enabled(true), forces_enabled(false), emultate_one_step(false), alpha_test_enabled(true),
    forces(NULL), logger(logger)
{

    try
    {
        init_device();
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
    // Create the device
    if( FAILED( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                                      D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                      &present_parameters, &device ) ) )
        throw D3DInitError();
    
    check_state( device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
	check_state( device->SetRenderState( D3DRS_ALPHAREF, (DWORD)0xffffffff ) );
    check_state( device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE ) );

    toggle_wireframe();
    set_alpha_test();
}

void  Application::set_alpha_test()
{
    if(alpha_test_enabled)
        check_state( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL) );
    else
        check_state( device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS) );
}


void Application::render()
{
    check_render( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, BACKGROUND_COLOR, 1.0f, 0 ) );
    
    // Begin the scene
    check_render( device->BeginScene() );
    // Setting constants
    D3DXVECTOR3 directional_vector;
    D3DXVec3Normalize(&directional_vector, &SHADER_VAL_DIRECTIONAL_VECTOR);
    D3DXVECTOR3 spot_vector;
    D3DXVec3Normalize(&spot_vector, &SHADER_VAL_SPOT_VECTOR);

    D3DCOLOR ambient_color = ambient_light_enabled ? SHADER_VAL_AMBIENT_COLOR : BLACK;
    D3DCOLOR directional_color = directional_light_enabled ? SHADER_VAL_DIRECTIONAL_COLOR : BLACK;
    D3DCOLOR point_color = point_light_enabled ? SHADER_VAL_POINT_COLOR : BLACK;
    D3DCOLOR spot_color = spot_light_enabled ? SHADER_VAL_SPOT_COLOR : BLACK;

    float in_cos = cos(SHADER_VAL_SPOT_INNER_ANGLE);
    float out_cos = cos(SHADER_VAL_SPOT_OUTER_ANGLE);
    _ASSERT( in_cos - out_cos != 0.0f );

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
    set_shader_point(  SHADER_REG_SPOT_POSITION,      SHADER_VAL_SPOT_POSITION);
    set_shader_color(  SHADER_REG_SPOT_COLOR,         spot_color);
    set_shader_vector( SHADER_REG_SPOT_VECTOR,        spot_vector);
    set_shader_float(  SHADER_REG_SPOT_X_COEF,        1/(in_cos - out_cos));
    set_shader_float(  SHADER_REG_SPOT_CONST_COEF,    out_cos/(in_cos - out_cos));
    
    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        // Set up
        ( (*iter).display_model->get_shader() ).set();

        set_shader_matrix( SHADER_REG_POS_AND_ROT_MX, (*iter).display_model->get_rotation_and_position() );
        
        // Draw
        (*iter).display_model->draw();
    }
    // End the scene
    check_render( device->EndScene() );
    
    // Present the backbuffer contents to the display
    check_render( device->Present( NULL, NULL, NULL, NULL ) );

}

IDirect3DDevice9 * Application::get_device()
{
    return device;
}

PhysicalModel * Application::add_model(Model &model, bool physical)
{
    ModelEntity model_entity = {NULL, NULL, NULL};

    // TODO: Null pointer
    model_entity.display_model = &model;

    if(physical)
    {
        Vertex * vertices = model.lock_vertex_buffer();
        model_entity.physical_model =
            new PhysicalModel(vertices,
                              model.get_vertices_count(),
                              VERTEX_INFO,
                              CLUSTERS_BY_AXES,
                              CLUSTER_PADDING_COEFF,
                              NULL,
                              VERTEX_MASS);
        
        model.unlock_vertex_buffer();

        static const int BUFFER_SIZE = 128;
        char description[BUFFER_SIZE];
        sprintf_s(description, BUFFER_SIZE, "%6i vertices in %4i=%ix%ix%i clusters",
                                            model.get_vertices_count(), TOTAL_CLUSTERS_COUNT,
                                            CLUSTERS_BY_AXES[0], CLUSTERS_BY_AXES[1], CLUSTERS_BY_AXES[2]);
        model_entity.performance_reporter = new PerformanceReporter(logger, description);
    }
    else
    {
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

void Application::rotate_models(float phi)
{
    for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        (*iter).display_model->rotate(phi);
    }
}

void Application::process_key(unsigned code)
{
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
    case 'A':
        rotate_models(-ROTATE_STEP);
        break;
    case 'D':
        rotate_models(ROTATE_STEP);
        break;
    case '1':
        directional_light_enabled = !directional_light_enabled;
        break;
    case '2':
        point_light_enabled = !point_light_enabled;
        break;
    case '3':
        spot_light_enabled = !spot_light_enabled;
        break;
    case '4':
        ambient_light_enabled = !ambient_light_enabled;
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
    case VK_TAB:
        alpha_test_enabled = !alpha_test_enabled;
        set_alpha_test();
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
                process_key( static_cast<unsigned>( msg.wParam ) );
            }

            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            total_stopwatch.start();
            // physics
            if(emulation_enabled || emultate_one_step)
            {
                // for each model entity
                for (ModelEntities::iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
                {
                    PhysicalModel       * physical_model       = (*iter).physical_model;
                    Model               * display_model        = (*iter).display_model;
                    PerformanceReporter * performance_reporter = (*iter).performance_reporter;
                    
                    if( NULL != physical_model )
                    {
                        stopwatch.start();
                        physical_model->compute_next_step(*forces);
                        double time = stopwatch.stop();

                        if( NULL != performance_reporter )
                        {
                            performance_reporter->add_measurement(time);
                        }
                        
                        Vertex *vertices = display_model->lock_vertex_buffer();
                        physical_model->update_vertices(vertices, display_model->get_vertices_count(), VERTEX_INFO);
                        display_model->unlock_vertex_buffer();
                    }
                }
                ++physics_frames;
            }

            emultate_one_step = false;
            
            // graphics
            stopwatch.start();
            render();
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
    render_performance_reporter.report_results();
    total_performance_reporter.report_results();
}

void Application::toggle_wireframe()
{
    static bool wireframe = !INITIAL_WIREFRAME_STATE;
    wireframe = !wireframe;

    if( wireframe )
    {
        check_state( device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ) );
    }
    else
    {
        check_state( device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) );
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
}

Application::~Application()
{
    delete_model_stuff();
    release_interfaces();
}

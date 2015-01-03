#include "Renderer.h"
#include "Stopwatch.h"
#include "matrices.h"
#include "Model.h"


using CrashAndSqueeze::Math::Matrix;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;

namespace
{
    const bool        INITIAL_WIREFRAME_STATE = true;
    const D3DCOLOR    BACKGROUND_COLOR = D3DCOLOR_XRGB( 255, 255, 255 );
    const D3DCOLOR    BLACK = D3DCOLOR_XRGB( 0, 0, 0 );
    const D3DCOLOR    TEXT_COLOR = D3DCOLOR_XRGB( 255, 255, 0 );
    const int         TEXT_HEIGHT = 20;
    const int         TEXT_MARGIN = 10;
    const int         TEXT_SPACING = 0;
    const int         TEXT_WIDTH = Window::DEFAULT_WINDOW_SIZE - 2*TEXT_MARGIN;
    const int         TEXT_LINE_HEIGHT = TEXT_HEIGHT + TEXT_SPACING;

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

    struct MyRect : public RECT
    {
        MyRect(LONG x = 0, LONG y = 0, LONG w = 0, LONG h = 0) { left = x; top = y; right = x + w; bottom = y + h; }
        LONG width() { return right - left; }
        LONG height() { return bottom - top; }
    };

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

const unsigned VECTORS_IN_MATRIX = sizeof(D3DXMATRIX)/sizeof(D3DXVECTOR4);

Renderer::Renderer(Window &window, Camera * camera) :
    d3d(NULL), device(NULL), font(NULL), camera(camera),
    alpha_test_enabled(false), wireframe(INITIAL_WIREFRAME_STATE), post_transform(rotate_x_matrix(D3DX_PI/2)),
    directional_light_enabled(true), point_light_enabled(true), spot_light_enabled(false), ambient_light_enabled(true),
    text_to_draw(NULL)
{
    try
    {
        init_device(window);
        init_font();
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

void Renderer::init_device(Window &window)
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

void Renderer::init_font()
{
    if( FAILED( D3DXCreateFont(device, TEXT_HEIGHT, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &font) ) )
        throw D3DXFontError();
}

void Renderer::draw_text(const TCHAR * text, RECT rect, D3DCOLOR color, bool align_right)
{
    DWORD format_flags = 0;
    if(align_right)
        format_flags |= DT_RIGHT;

    if( 0 == font->DrawText(NULL, text, -1, &rect, format_flags, color) )
        throw RenderError();
}

void Renderer::toggle_wireframe()
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

void Renderer::set_wireframe()
{
    wireframe = true;
    check_state( device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME ) );
}

void Renderer::unset_wireframe()
{
    wireframe = false;
    check_state( device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID ) );
}

void Renderer::set_alpha_test()
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

void Renderer::toggle_alpha_test()
{
    alpha_test_enabled = !alpha_test_enabled;
    set_alpha_test();
}


void Renderer::set_text_to_draw(const TCHAR * text)
{
    text_to_draw = text;
}

void Renderer::render(const ModelEntities &model_entities, PerformanceReporter &internal_reporter, bool is_updating_vertices_on_gpu,  bool show_high_model)
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

    set_shader_matrix( SHADER_REG_VIEW_MX,            camera->get_matrix());
    set_shader_vector( SHADER_REG_DIRECTIONAL_VECTOR, directional_vector);
    set_shader_color(  SHADER_REG_DIRECTIONAL_COLOR,  directional_color);
    set_shader_float(  SHADER_REG_DIFFUSE_COEF,       SHADER_VAL_DIFFUSE_COEF);
    set_shader_color(  SHADER_REG_AMBIENT_COLOR,      ambient_color);
    set_shader_color(  SHADER_REG_POINT_COLOR,        point_color);
    set_shader_point(  SHADER_REG_POINT_POSITION,     SHADER_VAL_POINT_POSITION);
    set_shader_vector( SHADER_REG_ATTENUATION,        SHADER_VAL_ATTENUATION);
    set_shader_float(  SHADER_REG_SPECULAR_COEF,      SHADER_VAL_SPECULAR_COEF);
    set_shader_float(  SHADER_REG_SPECULAR_F,         SHADER_VAL_SPECULAR_F);
    set_shader_point(  SHADER_REG_EYE,                camera->get_eye());

    for (ModelEntities::const_iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        AbstractModel * display_model = (show_high_model || NULL == (*iter).low_model) ? (*iter).high_model : (*iter).low_model;
        PhysicalModel       * physical_model       = (*iter).physical_model;

        // Set up
        for (int i = 0; i < display_model->get_shaders_count(); ++i)
        {
            display_model->get_shader(i).set();
        }

        set_shader_matrix( SHADER_REG_POS_AND_ROT_MX, post_transform*display_model->get_transformation() );

        if(NULL != physical_model)
        {
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
        }

        display_model->draw();

        // Unset
        for (int i = 0; i < display_model->get_shaders_count(); ++i)
        {
            display_model->get_shader(i).unset();
        }

    }

    // Draw text info
    if (text_to_draw != NULL)
    {
        draw_text(text_to_draw, MyRect(TEXT_MARGIN, TEXT_MARGIN, TEXT_WIDTH, Window::DEFAULT_WINDOW_SIZE), TEXT_COLOR);
    }

    // End the scene
    check_render( device->EndScene() );

    stopwatch.start();
    // Present the backbuffer contents to the display
    check_render( device->Present( NULL, NULL, NULL, NULL ) );
    internal_reporter.add_measurement(stopwatch.stop());
}

void Renderer::release_interfaces()
{
    release_interface( d3d );
    release_interface( device );
    release_interface( font );
}

Renderer::~Renderer(void)
{
    release_interfaces();
}

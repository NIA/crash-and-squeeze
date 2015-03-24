#include "Renderer.h"
#include "Stopwatch.h"
#include "matrices.h"
#include "Model.h"

using CrashAndSqueeze::Math::Matrix;
using CrashAndSqueeze::Math::Vector;
using CrashAndSqueeze::Math::VECTOR_SIZE;

using namespace DirectX;

// Library imports
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "DXGI.lib" )

const unsigned VECTORS_IN_MATRIX = sizeof(float4x4)/sizeof(float4);
const unsigned COLOR_COMPONENTS  = VECTORS_IN_MATRIX;

namespace
{
    const bool        INITIAL_WIREFRAME_STATE = false;
    const float       BACKGROUND_COLOR[COLOR_COMPONENTS] = { 1, 1, 1, 1 };
    const float       CLEAR_DEPTH = 1;
    const float4      TEXT_COLOR( 1, 1, 0, 1 );
    const int         TEXT_HEIGHT = 20;
    const int         TEXT_MARGIN = 10;
    const int         TEXT_SPACING = 0;
    const int         TEXT_WIDTH = Window::DEFAULT_WINDOW_SIZE - 2*TEXT_MARGIN;
    const int         TEXT_LINE_HEIGHT = TEXT_HEIGHT + TEXT_SPACING;

    // Creates 4x4 matrix from float3x3 transformation and float3 shift
    void build_d3d_matrix(float4x4 &out_matrix /*out*/, Matrix transformation, Vector pos)
    {
        const int LAST = VECTORS_IN_MATRIX - 1; // index of last row/col in matrix

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


    // constant values of shader constants:
    const float SHADER_VAL_DIFFUSE_COEF  = 1.0f;
    const float SHADER_VAL_SPECULAR_COEF = 0.3f;
    const float SHADER_VAL_SPECULAR_F    = 25.0f;

    const float3 SHADER_VAL_DIRECTIONAL_VECTOR (0.5f, 0.5f, 1.0f);
    const float4 SHADER_VAL_DIRECTIONAL_COLOR  (0.9f, 0.9f, 0.9f, 1);

    const float3 SHADER_VAL_POINT_POSITION (-1.6f, 0.0f, 0.8f);
    const float4 SHADER_VAL_POINT_COLOR    (0.5f, 0.98f, 0.98f, 1);
    const float3 SHADER_VAL_ATTENUATION    (1.0f, 0, 0.8f);

    const float4 SHADER_VAL_AMBIENT_COLOR (0.3f, 0.3f, 0.3f, 1);


    const float4      BLACK ( 0, 0, 0, 0 );
    const float       ZEROS_VALUES[VECTORS_IN_MATRIX*VECTORS_IN_MATRIX] = {0};
    const float4x4    ZEROS (ZEROS_VALUES);

    enum {
        WORLD_CONSTANTS_SLOT,    // = 0
        MODEL_CONSTANTS_SLOT,    // = 1
        LIGHTING_CONSTANTS_SLOT, // = 2
        _CONTANTS_SLOTS_COUNT
    };
}

Renderer::Renderer(Window &window, Camera * camera) :
    device(nullptr), context(nullptr), render_target_view(nullptr), swap_chain(nullptr),
    rs_wireframe_on(nullptr), rs_wireframe_off(nullptr), camera(camera),
    alpha_test_enabled(false), wireframe(INITIAL_WIREFRAME_STATE),
    directional_light_enabled(true), point_light_enabled(true), spot_light_enabled(false), ambient_light_enabled(true),
    text_to_draw(nullptr), world_constants(nullptr), model_constants(nullptr), lighting_constants(nullptr)
{
    try
    {
        XMStoreFloat4x4(&post_transform, rotate_x_matrix(XM_PI/2));

        init_device(window);
        init_buffers();
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
    // Set default adapter and driver type settings
    IDXGIAdapter* adapter_to_use = nullptr;
    D3D_DRIVER_TYPE driver_type = D3D_DRIVER_TYPE_HARDWARE;

    // Look for 'NVIDIA PerfHUD' adapter
    // If it is present, override default settings
    IDXGIFactory * dxgi_factory = nullptr;
    if (FAILED( CreateDXGIFactory(IID_PPV_ARGS(&dxgi_factory)) ) )
        throw RendererInitError("CreateDXGIFactory");
    unsigned i = 0;
    IDXGIAdapter * adapter = nullptr;
    while(dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC  desc;
        if( FAILED( adapter->GetDesc(&desc) ) )
            throw RendererInitError("IDXGIAdapter::GetDesc");

        if (wcsstr(desc.Description, L"PerfHUD") != 0)
        {
            adapter_to_use = adapter;
            driver_type = D3D_DRIVER_TYPE_REFERENCE;
            break;
        } else {
            release_interface(adapter);
        }
        ++i;
    }

    // TODO !!!!! Release all interfaces, including when throwing an exception!
    // Use ComPtr or CComPtr or something like these?

    // Create the device
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0 // we need at least feature level 10 for geometry shader
    };
    unsigned feature_levels_count = array_size(feature_levels);
    unsigned device_flags = 0;
#ifndef NDEBUG
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif //ifndef NDEBUG
    if (FAILED( D3D11CreateDevice(adapter_to_use,
                                  driver_type,
                                  nullptr, // no software module
                                  device_flags,
                                  feature_levels,
                                  feature_levels_count,
                                  D3D11_SDK_VERSION,
                                  &device,
                                  nullptr, // do not care which feature level was selected
                                  &context)))
        throw RendererInitError("D3D11CreateDevice");
    release_interface(adapter_to_use);

    // Create swap chain. Used defaults from https://hieroglyph3.codeplex.com/SourceControl/latest#trunk/Hieroglyph3/Source/SwapChainConfigDX11.cpp
    DXGI_SWAP_CHAIN_DESC swap_chain_desc;
    swap_chain_desc.BufferDesc.Width = window.get_width();
    swap_chain_desc.BufferDesc.Height = window.get_height();
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;

    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;

    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.OutputWindow = window;
    swap_chain_desc.Windowed = true;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Flags = 0;
    if ( FAILED(dxgi_factory->CreateSwapChain(device, &swap_chain_desc, &swap_chain) ) )
        throw RendererInitError("IDXGIFactory::CreateSwapChain");
    release_interface(dxgi_factory);

    // Create a render target view for a back buffer
    ID3D11Texture2D *back_buffer = nullptr;
    if ( FAILED( swap_chain->GetBuffer( 0, IID_PPV_ARGS(&back_buffer) ) ) )
        throw RendererInitError("IDXGISwapChain::GetBuffer");
    if ( FAILED( device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view) ) )
        throw RendererInitError("ID3D11Device::CreateRenderTargetView");
    release_interface(back_buffer);
    // Create a depth stencil view. Use defaults from the book Practical Rendering and Computation with Direct3D 11, page 22
    D3D11_TEXTURE2D_DESC depth_buffer_desc;
    depth_buffer_desc.Width = window.get_width();
    depth_buffer_desc.Height = window.get_height();
    depth_buffer_desc.MipLevels = 1;
    depth_buffer_desc.ArraySize = 1;
    depth_buffer_desc.Format = DXGI_FORMAT_D32_FLOAT;
    depth_buffer_desc.SampleDesc = swap_chain_desc.SampleDesc; // use the same defaults as we used above for swap chain
    depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_buffer_desc.CPUAccessFlags = 0;
    depth_buffer_desc.MiscFlags = 0;
    ID3D11Texture2D *depth_buffer = nullptr;
    if ( FAILED( device->CreateTexture2D(&depth_buffer_desc, nullptr, &depth_buffer) ) )
        throw RendererInitError("ID3D11Device::CreateTexture2D(depth_buffer)");;
    if ( FAILED( device->CreateDepthStencilView(depth_buffer, nullptr, &depth_stencil_view) ) )
        throw RendererInitError("ID3D11Device::CreateDepthStencilView");
    release_interface(depth_buffer);

    // Now set render target view and depth stencil view
    ID3D11RenderTargetView * rt_views[] = {render_target_view};
    context->OMSetRenderTargets(array_size(rt_views), rt_views, depth_stencil_view);

    // Set viewport
    D3D11_VIEWPORT vp;
    vp.Width  = static_cast<FLOAT>(window.get_width());
    vp.Height = static_cast<FLOAT>(window.get_height());
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    context->RSSetViewports( 1, &vp );

    // Configure alpha-test
    // TODO: [DX11] enable alpha test (no direct equivalent for D3DRS_ALPHAREF and D3DRS_ALPHATESTENABLE
#pragma WARNING(DX11 porting unfinished: alpha test)
    // Configure alpha-blending
    D3D11_BLEND_DESC blendDesc;
    blendDesc.AlphaToCoverageEnable  = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp   = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha =  D3D11_BLEND_ONE; // resulting alpha value is not important, so use defaults
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState * blendState = nullptr;
    check_state( device->CreateBlendState(&blendDesc, &blendState) );
    context->OMSetBlendState(blendState, nullptr, 0xffffff);
    release_interface(blendState);

    // Prepare states for wireframe toggling
    D3D11_RASTERIZER_DESC rs_desc;
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = D3D11_CULL_BACK;
    rs_desc.FrontCounterClockwise = false;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0;
    rs_desc.SlopeScaledDepthBias = 0;
    rs_desc.DepthClipEnable = true;
    rs_desc.ScissorEnable = false;
    rs_desc.MultisampleEnable = false;
    rs_desc.AntialiasedLineEnable = false;
    check_state( device->CreateRasterizerState(&rs_desc, &rs_wireframe_off) );
    rs_desc.FillMode = D3D11_FILL_WIREFRAME;
    check_state( device->CreateRasterizerState(&rs_desc, &rs_wireframe_on) );

    set_wireframe(INITIAL_WIREFRAME_STATE);
    set_alpha_test();
}

const Renderer::LightingConstants Renderer::LIGHT_CONSTS_INIT_DATA = {
    SHADER_VAL_DIRECTIONAL_COLOR,
    SHADER_VAL_POINT_COLOR,
    SHADER_VAL_AMBIENT_COLOR,
    SHADER_VAL_DIRECTIONAL_VECTOR,
    SHADER_VAL_DIFFUSE_COEF,
    SHADER_VAL_POINT_POSITION,
    SHADER_VAL_SPECULAR_COEF,
    SHADER_VAL_ATTENUATION,
    SHADER_VAL_SPECULAR_F,
};

void Renderer::init_buffers()
{
    world_constants    = new ConstantBuffer<WorldConstants>(this, nullptr, SET_FOR_VS | SET_FOR_PS, WORLD_CONSTANTS_SLOT, true);
    model_constants    = new ConstantBuffer<ModelConstants>(this, nullptr, SET_FOR_VS, MODEL_CONSTANTS_SLOT, true);
    lighting_constants = new ConstantBuffer<LightingConstants>(this, &LIGHT_CONSTS_INIT_DATA, SET_FOR_PS, LIGHTING_CONSTANTS_SLOT, true); // this buffer can possibly be made dynamic=false, 'cause most constants don't change (and the others can be *made* so)
}

void Renderer::init_font()
{
#pragma WARNING(DX11 porting unfinished: font)
}

ID3D11Device * Renderer::get_device() const
{
#ifndef NDEBUG
    if (nullptr == device)
        throw RendererInitError("-- get_device() while device not initialized --");
#endif //ifndef NDEBUG
    return device;
}

ID3D11DeviceContext * Renderer::get_context() const
{
#ifndef NDEBUG
    if (nullptr == context)
        throw RendererInitError("-- get_context() while context not initialized --");
#endif //ifndef NDEBUG
    return context;
}

void Renderer::draw_text(const TCHAR * /*text*/, RECT /*rect*/, float4 /*color*/, bool /*align_right*/ /*= false*/)
{
#pragma WARNING(DX11 porting unfinished: font)
    // TODO: probably use GDI+ ?
}

void Renderer::toggle_wireframe()
{
    set_wireframe( ! wireframe );
}

void Renderer::set_wireframe(bool enabled)
{
    wireframe = enabled;
    context->RSSetState(enabled ? rs_wireframe_on : rs_wireframe_off);
}

void Renderer::set_alpha_test()
{
#pragma WARNING(DX11 porting unfinished: alpha test)
    if(alpha_test_enabled)
    {
        // ...
    }
    else
    {
        // ...
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
    // Clear render target (NB: if there are multiple render targets, all of them should be cleared)
    context->ClearRenderTargetView(render_target_view, BACKGROUND_COLOR);
    context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, CLEAR_DEPTH, 0);

    // Setting constants
    float3 directional_vector;
    XMStoreFloat3(&directional_vector, XMVector3Normalize(XMLoadFloat3(&SHADER_VAL_DIRECTIONAL_VECTOR)));

    float4 ambient_color = ambient_light_enabled ? SHADER_VAL_AMBIENT_COLOR : BLACK;
    float4 directional_color = directional_light_enabled ? SHADER_VAL_DIRECTIONAL_COLOR : BLACK;
    float4 point_color = point_light_enabled ? SHADER_VAL_POINT_COLOR : BLACK;

    WorldConstants * world_consts = world_constants->lock(LOCK_OVERWRITE);
    world_consts->world = post_transform;
    world_consts->view  = camera->get_matrix();
    world_consts->eye   = camera->get_eye();
    world_constants->unlock();
    world_constants->set();

    LightingConstants * light_consts = lighting_constants->lock(LOCK_OVERWRITE);
    *light_consts = LIGHT_CONSTS_INIT_DATA;
    light_consts->direct_vec  = directional_vector;
    light_consts->direct_col  = directional_color;
    light_consts->point_col   = point_color;
    light_consts->ambient_col = ambient_color;
    // all other constans are left unchanged
    lighting_constants->unlock();
    lighting_constants->set();

    for (ModelEntities::const_iterator iter = model_entities.begin(); iter != model_entities.end(); ++iter )
    {
        AbstractModel * display_model = (show_high_model || nullptr == (*iter).low_model) ? (*iter).high_model : (*iter).low_model;
        PhysicalModel * physical_model = (*iter).physical_model;

        // Set up
        for (int i = 0; i < display_model->get_shaders_count(); ++i)
        {
            display_model->get_shader(i).set();
        }

        ModelConstants * model_consts = model_constants->lock(LOCK_OVERWRITE);
        ZeroMemory(model_consts, sizeof(*model_consts));
        model_consts->pos_and_rot = display_model->get_transformation();
        // if updating displayed vertices on GPU then setup matrices for it
        if(nullptr != physical_model && is_updating_vertices_on_gpu)
        {
            int clusters_num = physical_model->get_clusters_num();
#ifndef NDEBUG
            if(clusters_num > MAX_CLUSTERS_NUM)
                throw OutOfRangeError(RT_ERR_ARGS("clusters number of model is > MAX_CLUSTERS_NUM"));
#endif //ifndef NDEBUG
            // for each cluster...
            for(int i = 0; i < clusters_num; ++i)
            {
                // ...set initial center of mass...
                model_consts->clus_cm[i] = math_vector_to_float4(physical_model->get_cluster_initial_center(i));

                // ...and transformation matrices for positions...
                build_d3d_matrix(model_consts->clus_mx[i], physical_model->get_cluster_transformation(i), physical_model->get_cluster_center(i));

#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                // ...and quadratic transformation matrices for positions...
                build_d3d_matrix(model_consts->clus_mx_quad[i], physical_model->get_cluster_transformation_quad(i), Vector::ZERO);
                build_d3d_matrix(model_consts->clus_mx_mix[i],  physical_model->get_cluster_transformation_mix(i),  Vector::ZERO);
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED

                // ...and for normals
                build_d3d_matrix(model_consts->clus_nrm_mx[i], physical_model->get_cluster_normal_transformation(i), Vector::ZERO);
            }
            // TODO: do we need to set this zero matrix if we already did ZeroMemory(model_consts)?
            // Last zero matrix:
            model_consts->clus_mx[clusters_num]     = ZEROS;
            model_consts->clus_nrm_mx[clusters_num] = ZEROS;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            model_consts->clus_mx_quad[clusters_num]= ZEROS;
            model_consts->clus_nrm_mx[clusters_num] = ZEROS;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
        }

        model_constants->unlock();
        model_constants->set();

        display_model->draw();

        // Unset
        for (int i = 0; i < display_model->get_shaders_count(); ++i)
        {
            display_model->get_shader(i).unset();
        }

    }

    // Draw text info
    if (text_to_draw != nullptr)
    {
        draw_text(text_to_draw, MyRect(TEXT_MARGIN, TEXT_MARGIN, TEXT_WIDTH, Window::DEFAULT_WINDOW_SIZE), TEXT_COLOR);
    }

    stopwatch.start();
    // Present the back buffer contents to the display
    check_render( swap_chain->Present(0, 0) );
    internal_reporter.add_measurement(stopwatch.stop());
}

void Renderer::release_interfaces()
{
    delete_pointer(world_constants);
    delete_pointer(model_constants);
    delete_pointer(lighting_constants);

    release_interface( device );
    release_interface( context );
    release_interface( render_target_view );
    release_interface( swap_chain );
    release_interface( rs_wireframe_on );
    release_interface( rs_wireframe_off );
}

Renderer::~Renderer(void)
{
    context->ClearState();
    release_interfaces();
}

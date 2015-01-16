#include "Shader.h"
#include <D3Dcompiler.h>
#include <fstream>
#include <sstream>

namespace
{
    const char *ENTRY_POINT = "main";
    const char *VS_TARGET = "vs_4_0";
    const char *PS_TARGET = "ps_4_0";
    const char *GS_TARGET = "gs_4_0";
}

// Library imports
#pragma comment( lib, "D3DCompiler.lib" )

void AbstractShader::compile()
{
    ID3DBlob  * shader_code = nullptr;
    ID3DBlob  * shader_errors = nullptr;

    // Read all file as of http://insanecoding.blogspot.ru/2011/11/how-to-read-in-file-in-c.html
    std::ifstream in(filename);
    if ( ! in)
        throw ShaderCompileError("Failed to open file");
    std::ostringstream contents;
    contents << in.rdbuf();
    in.close();
    std::string file_contents = contents.str();

    if( FAILED( D3DCompile(
                    file_contents.c_str(),
                    file_contents.length(),
                    filename,
                    nullptr, // no defines
                    nullptr, // ID3DInclude not needed: don't use #include in shaders. If used, should have passed D3D_COMPILE_STANDARD_FILE_INCLUDE
                    ENTRY_POINT,
                    target,
                    0, 0, // no flags1, flags2
                    &shader_code,
                    &shader_errors) ) )
        throw ShaderCompileError(shader_errors);
    release_interface(shader_errors);

    if ( FAILED( create(shader_code->GetBufferPointer(), shader_code->GetBufferSize()) ) )
    {
        release_interface(shader_code);
        throw ShaderInitError();
    }
    release_interface(shader_code);
}

/************************************************************************/
/*                          Vertex shader                               */
/************************************************************************/

VertexShader::VertexShader(Renderer *renderer, const D3D11_INPUT_ELEMENT_DESC* vertex_desc, unsigned vertex_desc_num, const char *shader_filename)
    : AbstractShader(renderer, shader_filename, VS_TARGET), vertex_layout(nullptr), shader(nullptr),
      vertex_desc(vertex_desc), vertex_desc_num(vertex_desc_num)
{}

HRESULT VertexShader::create(const void * shader_code, unsigned code_size)
{
    if( FAILED( get_renderer()->get_device()->CreateInputLayout(vertex_desc, vertex_desc_num, shader_code, code_size, &vertex_layout) ) )
        throw VertexDeclarationInitError();
    return get_renderer()->get_device()->CreateVertexShader(shader_code, code_size, nullptr, &shader);
}

void VertexShader::set()
{
    if (nullptr == shader)
    {
        throw ShaderCompileError("Tried to set vertex shader that was not compiled");
    }
    get_renderer()->get_context()->IASetInputLayout(vertex_layout);
    get_renderer()->get_context()->VSSetShader(shader, nullptr, 0);
}

void VertexShader::unset()
{
    get_renderer()->get_context()->VSSetShader(nullptr, nullptr, 0);
}

void VertexShader::release_interfaces()
{
    release_interface( vertex_layout );
    release_interface( shader );
}

VertexShader::~VertexShader()
{
    release_interfaces();
}

/************************************************************************/
/*                           Pixel shader                               */
/************************************************************************/

PixelShader::PixelShader(Renderer *renderer, const char * shader_filename)
    : AbstractShader(renderer, shader_filename, PS_TARGET), shader(nullptr)
{}

HRESULT PixelShader::create(const void * shader_code, unsigned code_size)
{
    return get_renderer()->get_device()->CreatePixelShader(shader_code, code_size, nullptr, &shader);
}

void PixelShader::set()
{
    if (nullptr == shader)
    {
        throw ShaderCompileError("Tried to set pixel shader that was not compiled");
    }
    get_renderer()->get_context()->PSSetShader(shader, nullptr, 0);
}

void PixelShader::unset()
{
    get_renderer()->get_context()->PSSetShader(nullptr, nullptr, 0);
}
PixelShader::~PixelShader()
{
    release_interface(shader);
}

/************************************************************************/
/*                          Geometry shader                             */
/************************************************************************/

GeometryShader::GeometryShader(Renderer *renderer, const char * shader_filename)
    : AbstractShader(renderer, shader_filename, GS_TARGET), shader(nullptr)
{}

HRESULT GeometryShader::create(const void * shader_code, unsigned code_size)
{
    return get_renderer()->get_device()->CreateGeometryShader(shader_code, code_size, nullptr, &shader);
}

void GeometryShader::set()
{
    if (nullptr == shader)
    {
        throw ShaderCompileError("Tried to set geometry shader that was not compiled");
    }
    get_renderer()->get_context()->GSSetShader(shader, nullptr, 0);
}

void GeometryShader::unset()
{
    get_renderer()->get_context()->GSSetShader(nullptr, nullptr, 0);
}

GeometryShader::~GeometryShader()
{
    release_interface(shader);
}

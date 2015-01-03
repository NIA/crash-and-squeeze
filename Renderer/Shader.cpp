#include "Shader.h"

namespace
{
    const char *ENTRY_POINT = "main";
    const char *VS_PROFILE = "vs_2_0";
    const char *PS_PROFILE = "ps_2_0";
}

void AbstractShader::compile()
{
    ID3DXBuffer * shader_buffer = NULL;
    ID3DXBuffer * shader_errors = NULL;
    if( FAILED( D3DXCompileShaderFromFile( filename, NULL, NULL, ENTRY_POINT, profile, NULL, &shader_buffer, &shader_errors, NULL ) ) )
        throw ShaderCompileError(shader_errors);
    release_interface(shader_errors);

    if ( FAILED( create((DWORD*) shader_buffer->GetBufferPointer()) ) )
    {
        release_interface(shader_buffer);
        throw ShaderInitError();
    }
    release_interface(shader_buffer);
}

/************************************************************************/
/*                          Vertex shader                               */
/************************************************************************/

VertexShader::VertexShader(Renderer *renderer, const D3DVERTEXELEMENT9* vertex_declaration, const TCHAR *shader_filename)
: AbstractShader(renderer, shader_filename, VS_PROFILE), vertex_decl(NULL), shader(NULL)
{
    if( FAILED( renderer->get_device()->CreateVertexDeclaration(vertex_declaration, &vertex_decl) ) )
        throw VertexDeclarationInitError();
}

HRESULT VertexShader::create(const DWORD * pFunction)
{
    return renderer->get_device()->CreateVertexShader(pFunction, &shader);
}

void VertexShader::set()
{
    if (NULL == shader)
    {
        throw ShaderCompileError("Tried to set vertex shader that was not compiled");
    }
    check_render( renderer->get_device()->SetVertexDeclaration(vertex_decl) );
    check_render( renderer->get_device()->SetVertexShader(shader) );
}

void VertexShader::unset()
{
    check_render( renderer->get_device()->SetVertexShader(NULL) );
}

void VertexShader::release_interfaces()
{
    release_interface( vertex_decl );
    release_interface( shader );
}

VertexShader::~VertexShader()
{
    release_interfaces();
}

/************************************************************************/
/*                           Pixel shader                               */
/************************************************************************/

PixelShader::PixelShader(Renderer *renderer, const TCHAR * shader_filename)
    : AbstractShader(renderer, shader_filename, PS_PROFILE), shader(NULL)
{}

HRESULT PixelShader::create(const DWORD * pFunction)
{
    return renderer->get_device()->CreatePixelShader(pFunction, &shader);
}

void PixelShader::set()
{
    if (NULL == shader)
    {
        throw ShaderCompileError("Tried to set pixel shader that was not compiled");
    }
    check_render( renderer->get_device()->SetPixelShader(shader) );
}

void PixelShader::unset()
{
    check_render( renderer->get_device()->SetPixelShader(NULL) );
}
PixelShader::~PixelShader()
{
    release_interface(shader);
}

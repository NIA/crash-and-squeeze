#include "Shader.h"

namespace
{
    const char *ENTRY_POINT = "main";
    const char *PROFILE = "vs_2_0";
}

VertexShader::VertexShader(IDirect3DDevice9 *device, const D3DVERTEXELEMENT9* vertex_declaration, const TCHAR *shader_filename)
: device(device), vertex_decl(NULL), shader(NULL)
{
    try
    {
        if( FAILED( device->CreateVertexDeclaration(vertex_declaration, &vertex_decl) ) )
            throw VertexDeclarationInitError();

        ID3DXBuffer * shader_buffer = NULL;
        ID3DXBuffer * shader_errors = NULL;
        if( FAILED( D3DXCompileShaderFromFile( shader_filename, NULL, NULL, ENTRY_POINT, PROFILE, NULL, &shader_buffer, &shader_errors, NULL ) ) )
            throw VertexShaderCompileError(shader_errors);
        
        if( FAILED( device->CreateVertexShader( (DWORD*) shader_buffer->GetBufferPointer(), &shader ) ) )
        {
            release_interface(shader_errors);
            release_interface(shader_buffer);
            throw VertexShaderInitError();
        }
        release_interface(shader_buffer);
        release_interface(shader_errors);
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

void VertexShader::set()
{
    check_render( device->SetVertexDeclaration(vertex_decl) );
    check_render( device->SetVertexShader(shader) );
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

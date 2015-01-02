#pragma once

#include "main.h"

class AbstractShader
{
protected:
    IDirect3DDevice9 *device;
    const TCHAR *filename;
    const char *profile;

    // (should be implemented) creates shader from given compiled buffer
    virtual HRESULT create(const DWORD * pFunction) = 0;
public:
    AbstractShader(IDirect3DDevice9 * device, const TCHAR * shader_filename, const char * profile) :
        device(device), filename(shader_filename), profile(profile)
    {}
    // compiles shader from file and calls `create`
    void compile();
    // (should be implemented) sets this shader as current
    virtual void set() = 0;
    virtual void unset() = 0;
    virtual ~AbstractShader() {}
};

class VertexShader : public AbstractShader
{
private:
    IDirect3DVertexDeclaration9 *vertex_decl;   // vertex declaration
    IDirect3DVertexShader9      *shader;        // vertex shader
    
    // Deinitialization steps:
    void release_interfaces();
protected:
    virtual HRESULT create(const DWORD * pFunction) override;
public:
    VertexShader(IDirect3DDevice9 *device, const D3DVERTEXELEMENT9* vertex_declaration, const TCHAR *shader_filename);
    virtual void set() override;
    virtual void unset() override;
    virtual ~VertexShader();
};

class PixelShader : public AbstractShader
{
private:
    IDirect3DPixelShader9 *shader; // pixel shader

protected:
    virtual HRESULT create(const DWORD * pFunction);
public:
    PixelShader(IDirect3DDevice9 *device, const TCHAR * shader_filename);
    virtual void set();
    virtual void unset() override;
    virtual ~PixelShader();
};

/* TODO: TO BE IMPLEMENTED:
class GeometryShader : public AbstractShader
{
private:
};
*/

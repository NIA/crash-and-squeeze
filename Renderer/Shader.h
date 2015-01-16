#pragma once

#include "main.h"
#include "Renderer.h"

class AbstractShader
{
private:
    Renderer *renderer;
    const char *filename;
    const char *target;

protected:
    // creates shader from given compiled buffer (should be implemented in each subclass)
    virtual HRESULT create(const void * shader_code, unsigned code_size) = 0;

public:
    AbstractShader(Renderer * renderer, const char * shader_filename, const char * target) :
        renderer(renderer), filename(shader_filename), target(target)
    {}
    Renderer * get_renderer() const { return renderer; }
    // compiles shader from file and calls `create`
    void compile();
    // sets this shader as current (should be implemented in each subclass)
    virtual void set() = 0;
    // sets no current shader (should be implemented in each subclass)
    virtual void unset() = 0;
    virtual ~AbstractShader() {}
};

class VertexShader : public AbstractShader
{
private:
    ID3D11InputLayout   *vertex_layout;   // vertex declaration
    ID3D11VertexShader  *shader;        // vertex shader

    // for creating input layout: stored for later call of `create`
    const D3D11_INPUT_ELEMENT_DESC* vertex_desc;
    unsigned vertex_desc_num;
    
    // Deinitialization steps:
    void release_interfaces();
protected:
    virtual HRESULT create(const void * shader_code, unsigned code_size) override;
public:
    VertexShader(Renderer *renderer, const D3D11_INPUT_ELEMENT_DESC* vertex_desc, unsigned vertex_desc_num, const char *shader_filename);
    virtual void set() override;
    virtual void unset() override;
    virtual ~VertexShader();
};

class PixelShader : public AbstractShader
{
private:
    ID3D11PixelShader *shader; // pixel shader

protected:
    virtual HRESULT create(const void * shader_code, unsigned code_size) override;
public:
    PixelShader(Renderer *renderer, const char * shader_filename);
    virtual void set() override;
    virtual void unset() override;
    virtual ~PixelShader();
};

class GeometryShader : public AbstractShader
{
private:
    ID3D11GeometryShader *shader; // geometry shader

protected:
    virtual HRESULT create(const void * shader_code, unsigned code_size) override;

public:
    GeometryShader(Renderer *renderer, const char * shader_filename);
    virtual void set() override;
    virtual void unset() override;
    virtual ~GeometryShader();
};

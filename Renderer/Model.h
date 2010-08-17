#pragma once
#include "main.h"
#include "Vertex.h"
#include "Shader.h"

class Model
{
private:
    IDirect3DDevice9    *device;
    VertexShader        &shader;

    unsigned    vertices_count;
    unsigned    primitives_count;

    D3DPRIMITIVETYPE        primitive_type;
    IDirect3DVertexBuffer9  *vertex_buffer;
    IDirect3DIndexBuffer9   *index_buffer;
    unsigned vertex_size;

    D3DXVECTOR3 position;
    D3DXVECTOR3 rotation;
    D3DXMATRIX rotation_and_position;

    void update_matrix();

    void release_interfaces();

public:
    Model(  IDirect3DDevice9 *device,
            D3DPRIMITIVETYPE primitive_type,
            VertexShader &shader,
            unsigned vertex_size,
            const Vertex *vertices,
            unsigned vertices_count,
            const Index *indices,
            unsigned indices_count,
            unsigned primitives_count,
            D3DXVECTOR3 position,
            D3DXVECTOR3 rotation);
    
    VertexShader &get_shader();
    
    const D3DXMATRIX &get_rotation_and_position() const;
    void rotate(float phi);
    
    virtual void draw() const;

    virtual ~Model();
private:
    // No copying!
    Model(const Model&);
    Model &operator=(const Model&);
};


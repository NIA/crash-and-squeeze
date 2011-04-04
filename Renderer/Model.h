#pragma once
#include "main.h"
#include "Vertex.h"
#include "Shader.h"
#include "Collections/array.h"

class AbstractModel
{
private:
    IDirect3DDevice9    *device;
    VertexShader        &shader;

    D3DXVECTOR3 position;
    D3DXVECTOR3 rotation;
    D3DXMATRIX rotation_and_position;

    void update_matrix();
    
public:
    AbstractModel(  IDirect3DDevice9 *device,
            VertexShader &shader,
            D3DXVECTOR3 position,
            D3DXVECTOR3 rotation);
    
    VertexShader &get_shader() const;
    IDirect3DDevice9 *get_device() const;

    const D3DXMATRIX &get_rotation_and_position() const;
    void rotate(float phi);
    void move(D3DXVECTOR3 vector);
    
    virtual unsigned get_vertices_count() = 0;
    virtual Vertex * lock_vertex_buffer() = 0;
    virtual void unlock_vertex_buffer() = 0;
    
    virtual void draw() const = 0;
    virtual ~AbstractModel() {}
private:
    // No copying!
    AbstractModel(const AbstractModel&);
    AbstractModel &operator=(const AbstractModel&);
};

class Model : public AbstractModel
{
private:
    unsigned    vertices_count;
    unsigned    primitives_count;

    D3DPRIMITIVETYPE        primitive_type;
    IDirect3DVertexBuffer9  *vertex_buffer;
    IDirect3DIndexBuffer9   *index_buffer;

    void release_interfaces();

public:
    Model(  IDirect3DDevice9 *device,
            D3DPRIMITIVETYPE primitive_type,
            VertexShader &shader,
            const Vertex *vertices,
            unsigned vertices_count,
            const Index *indices,
            unsigned indices_count,
            unsigned primitives_count,
            D3DXVECTOR3 position,
            D3DXVECTOR3 rotation);

    virtual unsigned get_vertices_count() { return vertices_count; }
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();
    
    virtual void draw() const;

    void repaint_vertices(const ::CrashAndSqueeze::Collections::Array<int> &vertex_indices, D3DCOLOR color);

    virtual ~Model();
private:
    // No copying!
    Model(const Model&);
    Model &operator=(const Model&);
};

class MeshModel : public AbstractModel 
{
private:
    ID3DXMesh *mesh;
    DWORD materials_num;

    void release_interfaces();

public:
    MeshModel(IDirect3DDevice9 *device, VertexShader &shader,
              const TCHAR * mesh_file,
              D3DXVECTOR3 position, D3DXVECTOR3 rotation);

    virtual unsigned get_vertices_count();
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();
    
    virtual void draw() const;

    virtual ~MeshModel();
private:
    // No copying!
    MeshModel(const MeshModel&);
    MeshModel &operator=(const MeshModel&);
};
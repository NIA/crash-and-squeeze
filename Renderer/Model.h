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
    D3DXMATRIX rotation;
    float zoom;
    D3DXMATRIX transformation;

    bool draw_cw;
    bool draw_ccw;

    void update_matrix();

protected:
    virtual void pre_draw() const {}
    virtual void do_draw() const = 0;
    
public:
    AbstractModel(  IDirect3DDevice9 *device,
            VertexShader &shader,
            D3DXVECTOR3 position);
    
    VertexShader &get_shader() const;
    IDirect3DDevice9 *get_device() const;

    const D3DXMATRIX &get_transformation() const;
    
    void rotate(float phi);
    void move(D3DXVECTOR3 vector);
    
    void set_zoom(float zoom);
    void set_position(const D3DXVECTOR3 & new_pos);
    void set_rotation(const D3DXMATRIX & new_rot);
    
    virtual unsigned get_vertices_count() = 0;
    virtual Vertex * lock_vertex_buffer() = 0;
    virtual void unlock_vertex_buffer() = 0;

    void set_draw_cw(bool value) { draw_cw = value; }
    void set_draw_ccw(bool value) { draw_ccw = value; }
    
    void draw() const;
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

protected:
    virtual void pre_draw() const;
    virtual void do_draw() const;

public:
    Model(  IDirect3DDevice9 *device,
            D3DPRIMITIVETYPE primitive_type,
            VertexShader &shader,
            const Vertex *vertices,
            unsigned vertices_count,
            const Index *indices,
            unsigned indices_count,
            unsigned primitives_count,
            D3DXVECTOR3 position);

    virtual unsigned get_vertices_count() { return vertices_count; }
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

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

protected:
    virtual void do_draw() const;

public:
    MeshModel(IDirect3DDevice9 *device, VertexShader &shader,
              const TCHAR * mesh_file, const D3DCOLOR color,
              D3DXVECTOR3 position);

    virtual unsigned get_vertices_count();
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

    virtual ~MeshModel();
private:
    // No copying!
    MeshModel(const MeshModel&);
    MeshModel &operator=(const MeshModel&);
};

class PointModel : public AbstractModel
{
private:
    unsigned    points_count;
    IDirect3DVertexBuffer9  *vertex_buffer;

    void release_interfaces();

protected:
    virtual void pre_draw() const;
    virtual void do_draw() const;

public:
    PointModel(IDirect3DDevice9 *device, VertexShader &shader,
               const Vertex * src_vertices, unsigned src_vertices_count, unsigned step,
               D3DXVECTOR3 position);

    virtual unsigned get_vertices_count();
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

    virtual ~PointModel();
private:
    // No copying!
    PointModel(const PointModel&);
    PointModel &operator=(const PointModel&);
};
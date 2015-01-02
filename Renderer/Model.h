#pragma once
#include "main.h"
#include "Vertex.h"
#include "Shader.h"
#include "Collections/array.h"

class AbstractModel
{
private:
    IDirect3DDevice9    *device;
    ::CrashAndSqueeze::Collections::Array<AbstractShader*> shaders;
    AbstractModel       *subscriber;

    D3DXVECTOR3 position;
    D3DXVECTOR3 rotation;
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
            D3DXVECTOR3 position,
            D3DXVECTOR3 rotation);
    
    int get_shaders_count() const { return shaders.size(); }
    AbstractShader &get_shader(int index) const;
    void add_shader(AbstractShader &shader);
    IDirect3DDevice9 *get_device() const;

    const D3DXMATRIX &get_transformation() const;
    const D3DXVECTOR3 &get_position() { return position; }
    const D3DXVECTOR3 &get_rotation() { return rotation; }
    void rotate(float phi);
    void move(D3DXVECTOR3 vector);
    void set_zoom(float zoom);
    
    virtual unsigned get_vertices_count() = 0;
    virtual Vertex * lock_vertex_buffer() = 0;
    virtual void unlock_vertex_buffer() = 0;

    // Methods for making one model depend on updates of another
    // NB: currently there can be only one subscriber
    void add_subscriber(AbstractModel * subscriber);
    void notify_subscriber();
    virtual void on_notify() {} // override to handle notification

    void set_draw_cw(bool value) { draw_cw = value; }
    void set_draw_ccw(bool value) { draw_ccw = value; }
    
    void draw() const;

    struct Triangle
    {
        Index indices[VERTICES_PER_TRIANGLE];
        Index & operator[](unsigned i) { return indices[i]; }
    };
    // An iterator to loop through triangles of model
    class TriangleIterator
    {
    public:
        // Check that we did not reach the end
        virtual bool has_value() const = 0;
        // Get current triangle
        virtual Triangle operator*() const = 0;
        // Move to next triangle
        virtual void operator++() = 0;
        // Release resources that are held by this iterator
        virtual ~TriangleIterator() {};
    };
    // Returns iterator to loop through triangles of model.
    // Base implementation returns empty iterator,
    // should be overridden to implement proper logic
    virtual TriangleIterator* get_triangles();

    // Generates normals by averaging triangle normals, getting triangles from get_triagnles
    virtual void generate_normals();

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
    unsigned    indices_count;
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
            D3DXVECTOR3 position,
            D3DXVECTOR3 rotation);

    virtual unsigned get_vertices_count() { return vertices_count; }
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

    void repaint_vertices(const ::CrashAndSqueeze::Collections::Array<int> &vertex_indices, D3DCOLOR color);

    virtual TriangleIterator * get_triangles();

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
              D3DXVECTOR3 position, D3DXVECTOR3 rotation);

    virtual unsigned get_vertices_count();
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

    virtual TriangleIterator * get_triangles();

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
               D3DXVECTOR3 position, D3DXVECTOR3 rotation);

    virtual unsigned get_vertices_count();
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

    virtual ~PointModel();
private:
    // No copying!
    PointModel(const PointModel&);
    PointModel &operator=(const PointModel&);
};

class NormalsModel : public AbstractModel
{
private:
    AbstractModel * parent_model;
    unsigned        normals_count;
    IDirect3DVertexBuffer9  *vertex_buffer;
    
    // Options
    bool normalize_before_showing;
    float normal_length;

    void release_interfaces();

protected:
    virtual void pre_draw() const;
    virtual void do_draw() const;

    void update_normals();

public:
    NormalsModel(AbstractModel * parent_model, VertexShader &shader,
                 float normal_length, bool normalize_before_showing = true);

    virtual unsigned get_vertices_count();
    virtual Vertex * lock_vertex_buffer();
    virtual void unlock_vertex_buffer();

    virtual void on_notify();

    virtual ~NormalsModel();
};
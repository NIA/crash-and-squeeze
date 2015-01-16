#pragma once
#include "main.h"
#include "Vertex.h"
#include "Shader.h"
#include "Buffer.h"
#include "Collections/array.h"

class Renderer;

class ISubscriber
{
public:
    virtual void on_notify() = 0; // override to handle notification
};

class AbstractModel
{
private:
    Renderer    *renderer;
    ::std::vector<AbstractShader*> shaders;
    ISubscriber *subscriber;

    D3DXVECTOR3 position;
    D3DXVECTOR3 rotation;
    float zoom;
    D3DXMATRIX transformation;

    bool draw_cw;
    bool draw_ccw;

    void update_matrix();

protected:
    ID3D11Device * get_device() const;
    ID3D11DeviceContext * get_context() const;

    virtual void pre_draw() const {}
    virtual void do_draw() const = 0;

    
public:
    AbstractModel(Renderer *renderer,
                  VertexShader &shader,
                  D3DXVECTOR3 position = D3DXVECTOR3(0,0,0),
                  D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0));
    
    Renderer * get_renderer() const;
    
    int get_shaders_count() const { return shaders.size(); }
    AbstractShader &get_shader(int index) const;
    void add_shader(AbstractShader &shader);

    const D3DXMATRIX &get_transformation() const;
    const D3DXVECTOR3 &get_position() { return position; }
    const D3DXVECTOR3 &get_rotation() { return rotation; }
    void rotate(float phi);
    void move(D3DXVECTOR3 vector);
    void set_zoom(float zoom);
    
    virtual unsigned get_vertices_count() const = 0;
    virtual Vertex * lock_vertex_buffer() const = 0;
    virtual void unlock_vertex_buffer() const = 0;

    // Methods for making one model depend on updates of another
    // NB: currently there can be only one subscriber
    void add_subscriber(ISubscriber * subscriber);
    void notify_subscriber();

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
    virtual TriangleIterator* get_triangles() const;

    // Generates normals by averaging triangle normals, getting triangles from get_triagnles
    virtual void generate_normals();

    virtual ~AbstractModel() {}
private:
    DISABLE_COPY(AbstractModel)
};

class Model : public AbstractModel
{
private:
    D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
    VertexBuffer vertex_buffer;
    IndexBuffer  index_buffer;

    void release_interfaces();

protected:
    virtual void pre_draw() const override;
    virtual void do_draw() const override;

public:
    Model(  Renderer *renderer,
            D3D11_PRIMITIVE_TOPOLOGY primitive_topology,
            VertexShader &shader,
            const Vertex *vertices,
            unsigned vertices_count,
            const Index *indices,
            unsigned indices_count,
            bool dynamic = true,    // set to true if vertex data will be modified later, false for static data
            D3DXVECTOR3 position = D3DXVECTOR3(0,0,0),
            D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0));

    virtual unsigned get_vertices_count() const override;
    virtual Vertex * lock_vertex_buffer() const override;
    virtual void unlock_vertex_buffer() const override;

    void repaint_vertices(const ::CrashAndSqueeze::Collections::Array<int> &vertex_indices, D3DCOLOR color);

    virtual TriangleIterator * get_triangles() const override;

    virtual ~Model();
private:
    DISABLE_COPY(Model)
};

class MeshModel : public AbstractModel 
{
private:
    DWORD materials_num;

    void release_interfaces();

protected:
    virtual void do_draw() const override;

public:
    MeshModel(Renderer *renderer, VertexShader &shader,
              const TCHAR * mesh_file, const D3DCOLOR color,
              D3DXVECTOR3 position = D3DXVECTOR3(0,0,0),
              D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0));

    virtual unsigned get_vertices_count() const override;
    virtual Vertex * lock_vertex_buffer() const override;
    virtual void unlock_vertex_buffer() const override;

    virtual TriangleIterator * get_triangles() const override;

    virtual ~MeshModel();
private:
    DISABLE_COPY(MeshModel)
};

class PointModel : public AbstractModel
{
private:
    VertexBuffer *vertex_buffer;

    void release_interfaces();

protected:
    virtual void pre_draw() const override;
    virtual void do_draw() const override;

public:
    PointModel(Renderer *renderer, VertexShader &shader,
               const Vertex * src_vertices, unsigned src_vertices_count, unsigned step,
               bool dynamic = true, // set to true if vertex data will be modified later, false for static data
               D3DXVECTOR3 position = D3DXVECTOR3(0,0,0),
               D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0));

    virtual unsigned get_vertices_count() const override;
    virtual Vertex * lock_vertex_buffer() const override;
    virtual void unlock_vertex_buffer() const override;

    virtual ~PointModel();
private:
    // No copying!
    DISABLE_COPY(PointModel)
};

class NormalsModel : public AbstractModel, public ISubscriber
{
private:
    AbstractModel * parent_model;
    unsigned        normals_count;
    VertexBuffer    vertex_buffer;
    
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

    virtual unsigned get_vertices_count() const override;
    virtual Vertex * lock_vertex_buffer() const override;
    virtual void unlock_vertex_buffer() const override;

    virtual void on_notify() override;

    virtual ~NormalsModel();
private:
    DISABLE_COPY(NormalsModel);
};
#pragma once
#include "main.h"
#include "Vertex.h"
#include "Shader.h"
#include "Buffer.h"
#include "IRenderer.h"
#include "Collections/array.h"

#include <vector>

class ISubscriber
{
public:
    virtual void on_notify() = 0; // override to handle notification
};

class AbstractModel
{
private:
    IRenderer    *renderer;
    ::std::vector<AbstractShader*> shaders;
    ISubscriber *subscriber;

    float3 position;
    // Rotation is defined by three angles: of rotating around x,
    // then around y and then around z axis
    float3 rotation;
    float scale;
    float4x4 transformation;

    bool draw_cw;
    bool draw_ccw;

    void update_matrix();

protected:
    ID3D11Device * get_device() const;
    ID3D11DeviceContext * get_context() const;

    virtual void pre_draw() const {}
    virtual void do_draw() const = 0;

    
public:
    AbstractModel(IRenderer *renderer,
                  VertexShader &shader,
                  const float3 & position = float3(0,0,0),
                  const float3 & rotation = float3(0,0,0));
    
    IRenderer * get_renderer() const;
    
    int get_shaders_count() const { return shaders.size(); }
    AbstractShader &get_shader(int index) const;
    void add_shader(AbstractShader &shader);

    const float4x4 &get_transformation() const;
    const float3 &get_position() const { return position; }
    const float3 &get_rotation() const { return rotation; }
    void rotate(float phi);
    void move(const float3 & vector);
    void set_scale(float scale);
    
    virtual unsigned get_vertices_count() const = 0;
    virtual Vertex * lock_vertex_buffer(BufferLockType lock_type) const = 0;
    virtual void unlock_vertex_buffer() const = 0;
    virtual D3D11_PRIMITIVE_TOPOLOGY get_primitive_topology() const { return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED; }
    virtual unsigned get_indices_count() const { return 0; }
    virtual Index * lock_index_buffer(BufferLockType lock_type) const { return nullptr; }
    virtual void unlock_index_buffer() const {}

    // Methods for making one model depend on updates of another
    // NB: currently there can be only one subscriber
    void add_subscriber(ISubscriber * subscriber);
    void notify_subscriber();

    void set_draw_cw(bool value) { draw_cw = value; }
    void set_draw_ccw(bool value) { draw_ccw = value; }
    
    void draw() const;

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
    Model(  IRenderer *renderer,
            D3D11_PRIMITIVE_TOPOLOGY primitive_topology,
            VertexShader &shader,
            const Vertex *vertices,
            unsigned vertices_count,
            const Index *indices,
            unsigned indices_count,
            bool dynamic = true,    // set to true if vertex data will be modified later, false for static data
            const float3 & position = float3(0,0,0),
            const float3 & rotation = float3(0,0,0));

    virtual unsigned get_vertices_count() const override;
    virtual Vertex * lock_vertex_buffer(BufferLockType lock_type) const override;
    virtual void unlock_vertex_buffer() const override;
    virtual unsigned get_indices_count() const override;
    virtual Index * lock_index_buffer(BufferLockType lock_type) const override;
    virtual void unlock_index_buffer() const override;

    virtual D3D11_PRIMITIVE_TOPOLOGY get_primitive_topology() const override { return primitive_topology; }

    void repaint_vertices(const ::CrashAndSqueeze::Collections::Array<int> &vertex_indices, const float4 & color);

    virtual ~Model();
private:
    DISABLE_COPY(Model)
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
    PointModel(IRenderer *renderer, VertexShader &shader,
               const Vertex * src_vertices, unsigned src_vertices_count, unsigned step,
               bool dynamic = true, // set to true if vertex data will be modified later, false for static data
               const float3 & position = float3(0,0,0),
               const float3 & rotation = float3(0,0,0));

    virtual unsigned get_vertices_count() const override;
    virtual Vertex * lock_vertex_buffer(BufferLockType lock_type) const override;
    virtual void unlock_vertex_buffer() const override;

    virtual D3D11_PRIMITIVE_TOPOLOGY get_primitive_topology() const override;

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
    virtual Vertex * lock_vertex_buffer(BufferLockType lock_type) const override;
    virtual void unlock_vertex_buffer() const override;

    virtual void on_notify() override;

    virtual D3D11_PRIMITIVE_TOPOLOGY get_primitive_topology() const override;

    virtual ~NormalsModel();
private:
    DISABLE_COPY(NormalsModel);
};
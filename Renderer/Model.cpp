#include "Model.h"
#include "matrices.h"
#include <algorithm>

extern const unsigned VECTORS_IN_MATRIX;

using namespace DirectX;

// -- AbstractModel --

AbstractModel::AbstractModel(IRenderer *renderer, VertexShader &shader, const float3 & position, const float3 & rotation)
: renderer(renderer), position(position), rotation(rotation), scale(1), draw_cw(true), draw_ccw(false), subscriber(NULL)
{
        add_shader(shader);
        update_matrix();
}

AbstractShader &AbstractModel::get_shader(int index) const
{
#ifndef NDEBUG
    if (index >= get_shaders_count()) {
        throw OutOfRangeError();
    }
#endif //ifndef NDEBUG
    return *shaders[index];
}

void AbstractModel::add_shader(AbstractShader &shader)
{
    shader.compile();
    shaders.push_back(&shader);
}

ID3D11Device *AbstractModel::get_device() const
{
    return renderer->get_device();
}

ID3D11DeviceContext *AbstractModel::get_context() const
{
    return renderer->get_context();
}

IRenderer * AbstractModel::get_renderer() const
{
    return renderer;
}

void AbstractModel::update_matrix()
{
    XMStoreFloat4x4(&transformation, rotate_and_shift_matrix(rotation, position)*scale_matrix(scale));
}

void AbstractModel::rotate(float phi)
{
    rotation.z += phi;
    update_matrix();
}

void AbstractModel::move(const float3 & vector)
{
    position += vector;
    update_matrix();
}

void AbstractModel::set_scale(float scale)
{
    this->scale = scale;
    update_matrix();
}

const float4x4 &AbstractModel::get_transformation() const
{
    return transformation;
}

void AbstractModel::add_subscriber(ISubscriber * subscriber)
{
    this->subscriber = subscriber;
}
void AbstractModel::notify_subscriber()
{
    if (subscriber != nullptr)
        subscriber->on_notify();
}

void AbstractModel::draw() const
{
    if(!draw_cw && !draw_ccw)
        return; // nothing is drawn

    pre_draw();

    ID3D11RasterizerState * rs_state = nullptr;
    D3D11_RASTERIZER_DESC rs_desc;
    get_context()->RSGetState(&rs_state);
    // TODO: check for null? Currently we assume that RSSetState was pereviously called by Renderer
    rs_state->GetDesc(&rs_desc);

    D3D11_CULL_MODE desired_mode = D3D11_CULL_NONE;
    if(draw_ccw && ! draw_cw)
    {
        // set cull mode to show only ccw
        desired_mode = D3D11_CULL_FRONT;
    }
    else if(draw_cw && ! draw_ccw)
    {
        // set cull mode to show only cw
        desired_mode = D3D11_CULL_BACK;
    } // else => D3D11_CULL_NONE (assigned above)
    if (rs_desc.CullMode != desired_mode)
    {
        rs_desc.CullMode = desired_mode;
        release_interface(rs_state);
        // TODO: avoid creating state at each frame? But what if we change wireframe and other settings independently?
        get_device()->CreateRasterizerState(&rs_desc, &rs_state);
        get_context()->RSSetState(rs_state);
    }
    release_interface(rs_state);
    do_draw();
}

// -- Model --

Model::Model(   IRenderer *renderer, D3D11_PRIMITIVE_TOPOLOGY primitive_topology, VertexShader &shader,
                const Vertex *vertices, unsigned vertices_count, const Index *indices, unsigned indices_count,
                bool dynamic, const float3 & position, const float3 & rotation )
 
: AbstractModel(renderer, shader, position, rotation),
  primitive_topology(primitive_topology),
  vertex_buffer(renderer, vertices, vertices_count, dynamic),
  index_buffer( renderer,  indices, indices_count,  false) // index buffer is not dynamic anyway: no sense to modify it
{}

void Model::pre_draw() const
{
    vertex_buffer.set();
    index_buffer.set();
    get_context()->IASetPrimitiveTopology(primitive_topology);
}
    
void Model::do_draw() const
{
    get_context()->DrawIndexed(index_buffer.get_items_count(), 0, 0);
}

unsigned Model::get_vertices_count() const
{
    return vertex_buffer.get_items_count();
}

Vertex * Model::lock_vertex_buffer(BufferLockType lock_type) const
{
    return vertex_buffer.lock(lock_type);
}
void Model::unlock_vertex_buffer() const
{
    vertex_buffer.unlock();
}

unsigned Model::get_indices_count() const
{
    return index_buffer.get_items_count();
}

Index * Model::lock_index_buffer(BufferLockType lock_type) const
{
    return index_buffer.lock(lock_type);
}
void Model::unlock_index_buffer() const
{
    index_buffer.unlock();
}

void Model::repaint_vertices(const ::CrashAndSqueeze::Collections::Array<int> &vertex_indices, const float4 & color)
{
    Vertex *vertices = lock_vertex_buffer(LOCK_READ_WRITE);
    for(int i = 0; i < vertex_indices.size(); ++i)
    {
        int vertex_index = vertex_indices[i];
        vertices[vertex_index].color = color;
    }
    unlock_vertex_buffer();
}

void Model::release_interfaces()
{
    // Nothing to release in current implementation. This space was intentionally left blank.
}

Model::~Model()
{
    release_interfaces();
}

// -- PointModel --

PointModel::PointModel(IRenderer *renderer, VertexShader &shader,
                       const Vertex * src_vertices, unsigned src_vertices_count, unsigned int step,
                       bool dynamic, const float3 & position, const float3 & rotation)
: AbstractModel(renderer, shader, position, rotation), vertex_buffer(nullptr)
{
    _ASSERT(src_vertices != NULL);
    _ASSERT(step > 0);
    Vertex * points = nullptr;
    try
    {
        const unsigned points_count = src_vertices_count/step;
        // create new vertices for points
        points = new Vertex[points_count];
        for(unsigned i = 0; i < points_count; ++i)
        {
            points[i] = src_vertices[step*i];
        }

        vertex_buffer = new VertexBuffer(renderer, points, points_count, dynamic);

        delete_array(points);
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        delete_array(points);
        throw;
    }
}

unsigned PointModel::get_vertices_count() const
{
    return vertex_buffer->get_items_count();
}

Vertex * PointModel::lock_vertex_buffer(BufferLockType lock_type) const
{
    return vertex_buffer->lock(lock_type);
}

void PointModel::unlock_vertex_buffer() const
{
    vertex_buffer->unlock();
}

namespace
{
    // Use line strip to make point model better visible. Created seg are random, but who cares?
    // TODO: return to POINTLIST but use geometry shader/instancing to transform points to triangles or tetrahedrons
    const D3D11_PRIMITIVE_TOPOLOGY POINT_MODEL_TOPOLOGY = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
}

D3D11_PRIMITIVE_TOPOLOGY PointModel::get_primitive_topology() const
{
    return POINT_MODEL_TOPOLOGY;
}

void PointModel::pre_draw() const
{
    vertex_buffer->set();
    get_context()->IASetPrimitiveTopology(POINT_MODEL_TOPOLOGY);
}

void PointModel::do_draw() const
{
    get_context()->Draw(get_vertices_count(), 0);
}

void PointModel::release_interfaces()
{
    delete_pointer(vertex_buffer);
}

PointModel::~PointModel()
{
    release_interfaces();
}

// -- NormalsModel --

NormalsModel::NormalsModel(AbstractModel * parent_model, VertexShader &shader, float normal_length, bool normalize_before_showing /*= true*/)
    : AbstractModel(parent_model->get_renderer(), shader, parent_model->get_position(), parent_model->get_rotation()),
      parent_model(parent_model),
      normals_count(parent_model->get_vertices_count()),
      vertex_buffer(parent_model->get_renderer(), nullptr, normals_count*2, true, false), // the buffer is staging_backed = false because we only write to buffer, not read
      normal_length(normal_length),
      normalize_before_showing(normalize_before_showing)
{
    try
    {
        update_normals();
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

unsigned NormalsModel::get_vertices_count() const
{
    return normals_count*2;
}

Vertex * NormalsModel::lock_vertex_buffer(BufferLockType lock_type) const
{
    return vertex_buffer.lock(lock_type);
}

void NormalsModel::unlock_vertex_buffer() const
{
    vertex_buffer.unlock();
}

void NormalsModel::update_normals()
{
    Vertex * parent_vertices = parent_model->lock_vertex_buffer(LOCK_READ);
    Vertex * normal_vertices = lock_vertex_buffer(LOCK_OVERWRITE);

    for (unsigned i = 0; i < normals_count; ++i)
    {
        float3   pos    = parent_vertices[i].pos;
        XMVECTOR normal = XMLoadFloat3(&parent_vertices[i].normal);
        if (normalize_before_showing)
            normal = XMVector3Normalize(normal);
        normal *= normal_length;

        float4 color(1, 0, 0, 1); // TODO: option to visualize direction with color

        normal_vertices[2*i].pos   = pos;
        normal_vertices[2*i].color = color;
        pos += normal;
        normal_vertices[2*i + 1].pos   = pos;
        normal_vertices[2*i + 1].color = color;
    }
    
    unlock_vertex_buffer();
    parent_model->unlock_vertex_buffer();
}

void NormalsModel::on_notify()
{
    update_normals();
}

D3D11_PRIMITIVE_TOPOLOGY NormalsModel::get_primitive_topology() const
{
    return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
}

void NormalsModel::pre_draw() const
{
    vertex_buffer.set();
    get_context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}

void NormalsModel::do_draw() const
{
    get_context()->Draw(get_vertices_count(), 0);
}

void NormalsModel::release_interfaces()
{
    // Nothing to release in current implementation. This space was intentionally left blank.
}

NormalsModel::~NormalsModel()
{
    release_interfaces();
}

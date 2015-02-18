#include "Model.h"
#include "matrices.h"
#include <algorithm>

extern const unsigned VECTORS_IN_MATRIX;

using namespace DirectX;

// -- Triangle iterators
class EmptyTriangleIterator : public AbstractModel::TriangleIterator
{
public:
    virtual bool has_value() const { return false; }
    virtual AbstractModel::Triangle operator*() const { throw OutOfRangeError(); }
    virtual void operator++() { throw OutOfRangeError(); }
};

class IndexBufferTriangleIterator : public AbstractModel::TriangleIterator
{
private:
    const IndexBuffer & index_buffer;
    Index * indices;
    Index current_index;
    bool is_strip;
    bool swap_indices; // if is_strip, each even triangle should have indices swapped
public:
    IndexBufferTriangleIterator(const IndexBuffer & buffer, D3D_PRIMITIVE_TOPOLOGY prim_topology)
        : index_buffer(buffer), current_index(0), swap_indices(false)
    {
        indices = index_buffer.lock(LOCK_READ);
        is_strip = (prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    }
    virtual bool has_value() const
    {
        // if we have three indices (starting from current) available
        return current_index + VERTICES_PER_TRIANGLE - 1 < index_buffer.get_items_count();
    }
    virtual AbstractModel::Triangle operator*() const
    {
        #ifndef NDEBUG
        if ( ! has_value()) {
            throw OutOfRangeError();
        }
        #endif //ifndef NDEBUG

        AbstractModel::Triangle res;
        for (int i = 0; i < VERTICES_PER_TRIANGLE; ++i)
            res.indices[i] = indices[current_index + i];
        if (is_strip)
        {
            if (swap_indices)
                std::swap(res.indices[0], res.indices[1]); // each even triangle should have indices swapped in order to preserve orientation
        }
        return res;
    }
    virtual void operator++()
    {
        #ifndef NDEBUG
        if ( ! has_value()) {
            throw OutOfRangeError();
        }
        #endif //ifndef NDEBUG

        if (is_strip)
        {
            current_index += 1;
            swap_indices = !swap_indices;
        }
        else
        {
            current_index += VERTICES_PER_TRIANGLE;
        }
    }
    virtual ~IndexBufferTriangleIterator()
    {
        index_buffer.unlock();
    }
private:
    DISABLE_COPY(IndexBufferTriangleIterator);
};

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

AbstractModel::TriangleIterator * AbstractModel::get_triangles() const
{
    return new EmptyTriangleIterator();
}

void AbstractModel::generate_normals()
{
    unsigned vertices_count = get_vertices_count();
    Vertex * vertices = lock_vertex_buffer(LOCK_READ_WRITE);

    for (unsigned i = 0; i < vertices_count; ++i)
        vertices[i].set_normal(float3(0,0,0));

    TriangleIterator * triangle_iterator = get_triangles();
    for (TriangleIterator & t = *triangle_iterator; t.has_value(); ++t)
    {
        // Find normal of triangle
        Triangle triangle = *t;
        XMVECTOR positions[VERTICES_PER_TRIANGLE];
        for (unsigned i = 0; i < VERTICES_PER_TRIANGLE; ++i)
            positions[i] = XMLoadFloat3(&vertices[triangle[i]].pos);
        XMVECTOR v1 = positions[1] - positions[0];
        XMVECTOR v2 = positions[2] - positions[0];
        XMVECTOR n = XMVector3NormalizeEst(XMVector3Cross(v1, v2)); // use *Est version of XMVector3Normalize because accuracy is not critical for a normal

        // Add triangle normal to each vertex of this triangle to find average
        for (unsigned i = 0; i < VERTICES_PER_TRIANGLE; ++i)
            vertices[triangle[i]].normal += n;
    }
    // NB: here we should normalize all normals, but it is anyway done in shader

    // TODO: take into account that some normals can differ from average of triangles (somehow store this difference?)

    delete triangle_iterator;

    unlock_vertex_buffer();
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

AbstractModel::TriangleIterator * Model::get_triangles() const
{
    return new IndexBufferTriangleIterator(index_buffer, primitive_topology);
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

// -- MeshModel --
MeshModel::MeshModel(IRenderer *renderer, VertexShader &shader,
                     const TCHAR * mesh_file, const float4 & color,
                     const float3 & position, const float3 & rotation)
: AbstractModel(renderer, shader, position, rotation)
{
#pragma WARNING(DX11 porting unfinished: implement reading mesh from file: replace D3DX with DirectXTK or implement yourself)
    throw NotYetImplementedError(_T("MeshModel not yet implemented"));
}

unsigned MeshModel::get_vertices_count() const
{
    throw NotYetImplementedError(_T("MeshModel not yet implemented"));
}

Vertex * MeshModel::lock_vertex_buffer(BufferLockType lock_type) const
{
    throw NotYetImplementedError(_T("MeshModel not yet implemented"));
}

void MeshModel::unlock_vertex_buffer() const
{
    throw NotYetImplementedError(_T("MeshModel not yet implemented"));
}

void MeshModel::do_draw() const
{
    throw NotYetImplementedError(_T("MeshModel not yet implemented"));
}

AbstractModel::TriangleIterator * MeshModel::get_triangles() const
{
    throw NotYetImplementedError(_T("MeshModel not yet implemented"));
}

void MeshModel::release_interfaces()
{
    // Nothing to release in current implementation. This space was intentionally left blank.
}

MeshModel::~MeshModel()
{
    release_interfaces();
}

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

void PointModel::pre_draw() const
{
    vertex_buffer->set();
    get_context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
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

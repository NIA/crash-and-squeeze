#include "Model.h"
#include "matrices.h"

extern const unsigned VECTORS_IN_MATRIX;


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
    IDirect3DIndexBuffer9 * index_buffer;
    Index * indices;
    Index indices_count;
    Index current_index;
    //bool is_strip; TODO: support triangle strip primitive
public:
    IndexBufferTriangleIterator(IDirect3DIndexBuffer9 * buffer, Index indices_count)
        : index_buffer(buffer), indices_count(indices_count), current_index(0)
    {
        void * buffer_indices;
        if(FAILED( index_buffer->Lock(0, indices_count*sizeof(Index), &buffer_indices, D3DLOCK_READONLY) ))
            throw IndexBufferFillError();
        indices = reinterpret_cast<Index*>(buffer_indices);
    }
    virtual bool has_value() const
    {
        // if we have three indices (starting from current) available
        return current_index + VERTICES_PER_TRIANGLE - 1 < indices_count;
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
        return res;
    }
    virtual void operator++()
    {
        #ifndef NDEBUG
        if ( ! has_value()) {
            throw OutOfRangeError();
        }
        #endif //ifndef NDEBUG

        current_index += VERTICES_PER_TRIANGLE;
    }
    virtual ~IndexBufferTriangleIterator()
    {
        index_buffer->Unlock();
    }
};

// -- AbstractModel --

AbstractModel::AbstractModel(Renderer *renderer, VertexShader &shader, D3DXVECTOR3 position, D3DXVECTOR3 rotation)
: renderer(renderer), position(position), rotation(rotation), zoom(1), draw_cw(true), draw_ccw(false), subscriber(NULL)
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

IDirect3DDevice9 *AbstractModel::get_device() const
{
    return renderer->get_device();
}

Renderer * AbstractModel::get_renderer() const
{
    return renderer;
}

void AbstractModel::update_matrix()
{
    transformation = zoom*rotate_and_shift_matrix(rotation, position, zoom);
}

void AbstractModel::rotate(float phi)
{
    rotation.z += phi;
    update_matrix();
}

void AbstractModel::move(D3DXVECTOR3 vector)
{
    position += vector;
    update_matrix();
}

void AbstractModel::set_zoom(float zoom)
{
    this->zoom = zoom;
    update_matrix();
}

const D3DXMATRIX &AbstractModel::get_transformation() const
{
    return transformation;
}

void AbstractModel::add_subscriber(AbstractModel * subscriber)
{
    this->subscriber = subscriber;
}
void AbstractModel::notify_subscriber()
{
    if (subscriber != NULL)
        subscriber->on_notify();
}

void AbstractModel::draw() const
{
    if(draw_cw || draw_ccw)
    {
        pre_draw();
    }
    
    if(draw_ccw)
    {
        check_state( get_device()->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW ) );
        do_draw();
    }
    if(draw_cw)
    {
        check_state( get_device()->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
        do_draw();
    }
}

AbstractModel::TriangleIterator * AbstractModel::get_triangles()
{
    return new EmptyTriangleIterator();
}

void AbstractModel::generate_normals()
{
    unsigned vertices_count = get_vertices_count();
    Vertex * vertices = lock_vertex_buffer();

    for (unsigned i = 0; i < vertices_count; ++i)
        vertices[i].set_normal(D3DXVECTOR3(0,0,0));

    TriangleIterator * triangle_iterator = get_triangles();
    for (TriangleIterator & t = *triangle_iterator; t.has_value(); ++t)
    {
        // Find normal of triangle
        Triangle triangle = *t;
        D3DXVECTOR3 v1 = vertices[triangle[1]].pos - vertices[triangle[0]].pos;
        D3DXVECTOR3 v2 = vertices[triangle[2]].pos - vertices[triangle[0]].pos;
        D3DXVECTOR3 n;
        D3DXVec3Cross(&n, &v1, &v2);
        D3DXVec3Normalize(&n, &n);

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

Model::Model(   Renderer *renderer, D3DPRIMITIVETYPE primitive_type, VertexShader &shader,
                const Vertex *vertices, unsigned vertices_count, const Index *indices, unsigned indices_count,
                unsigned primitives_count, D3DXVECTOR3 position, D3DXVECTOR3 rotation )
 
: AbstractModel(renderer, shader, position, rotation), vertices_count(vertices_count), indices_count(indices_count),
  primitives_count(primitives_count),  primitive_type(primitive_type), vertex_buffer(NULL), index_buffer(NULL)
{
    _ASSERT(vertices != NULL);
    _ASSERT(indices != NULL);
    try
    {
        const unsigned vertices_size = vertices_count*sizeof(vertices[0]);
        const unsigned indices_size = indices_count*sizeof(indices[0]);

        if(FAILED( get_device()->CreateVertexBuffer( vertices_size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vertex_buffer, NULL ) ))
            throw VertexBufferInitError();
        

        if(FAILED( get_device()->CreateIndexBuffer( indices_size, D3DUSAGE_WRITEONLY, INDEX_FORMAT, D3DPOOL_DEFAULT, &index_buffer, NULL ) ))
            throw IndexBufferInitError();

        // fill the vertex buffer.
        VOID* vertices_to_fill;
        if(FAILED( vertex_buffer->Lock( 0, vertices_size, &vertices_to_fill, 0 ) ))
            throw VertexBufferFillError();
        memcpy( vertices_to_fill, vertices, vertices_size );
        vertex_buffer->Unlock();

        // fill the index buffer.
        VOID* indices_to_fill;
        if(FAILED( index_buffer->Lock( 0, indices_size, &indices_to_fill, 0 ) ))
            throw IndexBufferFillError();
        memcpy( indices_to_fill, indices, indices_size );
        index_buffer->Unlock();
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

void Model::pre_draw() const
{
    check_render( get_device()->SetStreamSource( 0, vertex_buffer, 0, sizeof(Vertex) ) );
    check_render( get_device()->SetIndices( index_buffer ) );
}
    
void Model::do_draw() const
{
    check_render( get_device()->DrawIndexedPrimitive( primitive_type, 0, 0, vertices_count, 0, primitives_count ) );
}

Vertex * Model::lock_vertex_buffer()
{
    void* vertices;
    if(FAILED( vertex_buffer->Lock( 0, vertices_count*sizeof(Vertex), &vertices, 0 ) ))
        throw VertexBufferLockError();
    return reinterpret_cast<Vertex*>(vertices);
}
void Model::unlock_vertex_buffer()
{
    vertex_buffer->Unlock();
}

AbstractModel::TriangleIterator * Model::get_triangles()
{
    return new IndexBufferTriangleIterator(index_buffer, indices_count);
}

void Model::repaint_vertices(const ::CrashAndSqueeze::Collections::Array<int> &vertex_indices, D3DCOLOR color)
{
    Vertex *vertices = lock_vertex_buffer();
    for(int i = 0; i < vertex_indices.size(); ++i)
    {
        int vertex_index = vertex_indices[i];
        vertices[vertex_index].color = color;
    }
    unlock_vertex_buffer();
}

void Model::release_interfaces()
{
    release_interface(vertex_buffer);
    release_interface(index_buffer);
}

Model::~Model()
{
    release_interfaces();
}

// -- MeshModel --
MeshModel::MeshModel(Renderer *renderer, VertexShader &shader,
                     const TCHAR * mesh_file, const D3DCOLOR color,
                     D3DXVECTOR3 position, D3DXVECTOR3 rotation)
: AbstractModel(renderer, shader, position, rotation), mesh(NULL)
{
    ID3DXMesh * temp_mesh;
    if( FAILED( D3DXLoadMeshFromX( mesh_file, D3DXMESH_SYSTEMMEM,
                                   get_device(), NULL,
                                   NULL, NULL, &materials_num,
                                   &temp_mesh ) ) )
    {
        throw MeshError();
    }

    if( FAILED( temp_mesh->CloneMesh( D3DXMESH_SYSTEMMEM, VERTEX_DECL_ARRAY, get_device(), &mesh ) ) )
    {
        throw MeshError();
    }
    release_interface(temp_mesh);

    Vertex *vertices = lock_vertex_buffer();
    for(unsigned i = 0; i < get_vertices_count(); ++i)
    {
        vertices[i].color = color;
    }
    unlock_vertex_buffer();
}

unsigned MeshModel::get_vertices_count()
{
    return mesh->GetNumVertices();
}

Vertex * MeshModel::lock_vertex_buffer()
{
    void* vertices;
    if(FAILED( mesh->LockVertexBuffer( 0, &vertices ) ))
        throw VertexBufferLockError();
    return reinterpret_cast<Vertex*>(vertices);
}

void MeshModel::unlock_vertex_buffer()
{
    mesh->UnlockVertexBuffer();
}

void MeshModel::do_draw() const
{
    for(unsigned i = 0; i < materials_num; ++i)
    {
        mesh->DrawSubset(i);
    }
}

AbstractModel::TriangleIterator * MeshModel::get_triangles()
{
    IDirect3DIndexBuffer9 * buffer;
    mesh->GetIndexBuffer(&buffer);
    // TODO: can we be sure that the mesh uses 32-bit index?!
    return new IndexBufferTriangleIterator(buffer, mesh->GetNumFaces()*VERTICES_PER_TRIANGLE);
}

void MeshModel::release_interfaces()
{
    release_interface(mesh);
}

MeshModel::~MeshModel()
{
    release_interfaces();
}

PointModel::PointModel(Renderer *renderer, VertexShader &shader,
                       const Vertex * src_vertices, unsigned src_vertices_count, unsigned int step,
                       D3DXVECTOR3 position, D3DXVECTOR3 rotation)
: AbstractModel(renderer, shader, position, rotation), vertex_buffer(NULL)
{
    _ASSERT(src_vertices != NULL);
    _ASSERT(step > 0);
    try
    {
        points_count = src_vertices_count/step;
        const unsigned buffer_size = points_count*sizeof(src_vertices[0]);

        if(FAILED( get_device()->CreateVertexBuffer( buffer_size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vertex_buffer, NULL ) ))
            throw VertexBufferInitError();

        // fill the vertex buffer.
        Vertex* points = lock_vertex_buffer();
        for(unsigned i = 0; i < points_count; ++i)
        {
            points[i] = src_vertices[step*i];
        }
        unlock_vertex_buffer();
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

unsigned PointModel::get_vertices_count()
{
    return points_count;
}

Vertex * PointModel::lock_vertex_buffer()
{
    void* vertices;
    if(FAILED( vertex_buffer->Lock( 0, points_count*sizeof(Vertex), &vertices, 0 ) ))
        throw VertexBufferLockError();
    return reinterpret_cast<Vertex*>(vertices);
}

void PointModel::unlock_vertex_buffer()
{
    vertex_buffer->Unlock();
}

void PointModel::pre_draw() const
{
    check_render( get_device()->SetStreamSource( 0, vertex_buffer, 0, sizeof(Vertex) ) );
}

void PointModel::do_draw() const
{
    check_render( get_device()->DrawPrimitive(D3DPT_POINTLIST, 0, points_count) );
}

void PointModel::release_interfaces()
{
    release_interface(vertex_buffer);
}

PointModel::~PointModel()
{
    release_interfaces();
}

NormalsModel::NormalsModel(AbstractModel * parent_model, VertexShader &shader, float normal_length, bool normalize_before_showing /*= true*/)
    : AbstractModel(parent_model->get_renderer(), shader, parent_model->get_position(), parent_model->get_rotation()),
      parent_model(parent_model),
      normals_count(parent_model->get_vertices_count()),
      normal_length(normal_length),
      normalize_before_showing(normalize_before_showing)
{
    try
    {
	    const unsigned buffer_size = get_vertices_count() * sizeof(Vertex);

        if(FAILED( get_device()->CreateVertexBuffer( buffer_size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vertex_buffer, NULL ) ))
            throw VertexBufferInitError();

        update_normals();
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

unsigned NormalsModel::get_vertices_count()
{
    return normals_count*2;
}

Vertex * NormalsModel::lock_vertex_buffer()
{
    void* vertices;
    if(FAILED( vertex_buffer->Lock( 0, get_vertices_count()*sizeof(Vertex), &vertices, 0 ) ))
        throw VertexBufferLockError();
    return reinterpret_cast<Vertex*>(vertices);
}

void NormalsModel::unlock_vertex_buffer()
{
    vertex_buffer->Unlock();
}

void NormalsModel::update_normals()
{
    Vertex * parent_vertices = parent_model->lock_vertex_buffer();
    Vertex * normal_vertices = lock_vertex_buffer();

    for (unsigned i = 0; i < normals_count; ++i)
    {
        D3DXVECTOR3 pos    = parent_vertices[i].pos;
        D3DXVECTOR3 normal = parent_vertices[i].normal;
        if (normalize_before_showing)
            D3DXVec3Normalize(&normal, &normal);
        D3DCOLOR    color = D3DCOLOR_XRGB(255, 0, 0); // TODO: option to visualize direction with color
        
            
        normal *= normal_length;
        normal_vertices[2*i].pos = pos;
        normal_vertices[2*i].color = color;
        normal_vertices[2*i + 1].pos = pos + normal;
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
    check_render( get_device()->SetStreamSource( 0, vertex_buffer, 0, sizeof(Vertex) ) );
}

void NormalsModel::do_draw() const
{
    check_render( get_device()->DrawPrimitive(D3DPT_LINELIST, 0, normals_count) );
}

void NormalsModel::release_interfaces()
{
    release_interface(vertex_buffer);
}

NormalsModel::~NormalsModel()
{
    release_interfaces();
}

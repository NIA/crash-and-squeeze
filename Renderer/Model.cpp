#include "Model.h"
#include "matrices.h"

extern const unsigned VECTORS_IN_MATRIX;

// -- AbstractModel --

AbstractModel::AbstractModel(IDirect3DDevice9 *device, VertexShader &shader, D3DXVECTOR3 position, D3DXVECTOR3 rotation)
: device(device), position(position), rotation(rotation), shader(shader)
{
        update_matrix();
}

VertexShader &AbstractModel::get_shader() const
{
    return shader;
}

IDirect3DDevice9 *AbstractModel::get_device() const
{
    return device;
}

void AbstractModel::update_matrix()
{
    rotation_and_position = rotate_and_shift_matrix(rotation, position);
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

const D3DXMATRIX &AbstractModel::get_rotation_and_position() const
{
    return rotation_and_position;
}

// -- Model --

Model::Model(   IDirect3DDevice9 *device, D3DPRIMITIVETYPE primitive_type, VertexShader &shader,
                const Vertex *vertices, unsigned vertices_count, const Index *indices, unsigned indices_count,
                unsigned primitives_count, D3DXVECTOR3 position, D3DXVECTOR3 rotation )
 
: AbstractModel(device, shader, position, rotation), vertices_count(vertices_count), primitives_count(primitives_count),
  primitive_type(primitive_type), vertex_buffer(NULL), index_buffer(NULL)
{
    _ASSERT(vertices != NULL);
    _ASSERT(indices != NULL);
    try
    {
        const unsigned vertices_size = vertices_count*sizeof(vertices[0]);
        const unsigned indices_size = indices_count*sizeof(indices[0]);

        if(FAILED( device->CreateVertexBuffer( vertices_size, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vertex_buffer, NULL ) ))
            throw VertexBufferInitError();
        

        if(FAILED( device->CreateIndexBuffer( indices_size, D3DUSAGE_WRITEONLY, INDEX_FORMAT, D3DPOOL_DEFAULT, &index_buffer, NULL ) ))
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

void Model::draw() const
{
    check_render( get_device()->SetStreamSource( 0, vertex_buffer, 0, sizeof(Vertex) ) );
    check_render( get_device()->SetIndices( index_buffer ) );
    check_render( get_device()->DrawIndexedPrimitive( primitive_type, 0, 0, vertices_count, 0, primitives_count ) );
}

Vertex * Model::lock_vertex_buffer()
{
    Vertex* vertices;
    if(FAILED( vertex_buffer->Lock( 0, vertices_count*sizeof(vertices[0]), reinterpret_cast<void**>(&vertices), 0 ) ))
        throw VertexBufferLockError();
    return vertices;
}
void Model::unlock_vertex_buffer()
{
    vertex_buffer->Unlock();
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
MeshModel::MeshModel(IDirect3DDevice9 *device, VertexShader &shader,
                     const TCHAR * mesh_file,
                     D3DXVECTOR3 position, D3DXVECTOR3 rotation)
: AbstractModel(device, shader, position, rotation), mesh(NULL)
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
}

unsigned MeshModel::get_vertices_count()
{
    return mesh->GetNumVertices();
}

Vertex * MeshModel::lock_vertex_buffer()
{
    Vertex* vertices;
    if(FAILED( mesh->LockVertexBuffer( 0, reinterpret_cast<void**>(&vertices) ) ))
        throw VertexBufferLockError();
    return vertices;
}

void MeshModel::unlock_vertex_buffer()
{
    mesh->UnlockVertexBuffer();
}

void MeshModel::draw() const
{
    for(unsigned i = 0; i < materials_num; ++i)
    {
        mesh->DrawSubset(i);
    }
}

void MeshModel::release_interfaces()
{
    release_interface(mesh);
}

MeshModel::~MeshModel()
{
    release_interfaces();
}

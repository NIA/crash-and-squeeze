#include "Buffer.h"
#include "Renderer.h"

Buffer::Buffer(Renderer * renderer, unsigned bind_flag, const void *buffer_data, unsigned items_count, unsigned item_size, bool dynamic) :
    renderer(renderer), buffer(nullptr), bind_flag(bind_flag), items_count(items_count), dynamic(dynamic)
{
    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.ByteWidth = items_count*item_size;
    desc.BindFlags = bind_flag;
    if (dynamic)
    {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags = 0;
    }
    D3D11_SUBRESOURCE_DATA data;
    D3D11_SUBRESOURCE_DATA *pData = nullptr; // if buffer_data is null, pData will also be null
    if (nullptr != buffer_data)
    {
        ZeroMemory(&data, sizeof(data));
        data.pSysMem = buffer_data;
        pData = &data;
    }
    if (FAILED( renderer->get_device()->CreateBuffer(&desc, pData, &buffer)))
        throw BufferInitError(bind_flag);
}

void * Buffer::lock() const
{
    if ( ! dynamic )
        throw BufferLockError(bind_flag); // cannot map immutable resource!

    D3D11_MAPPED_SUBRESOURCE mapped_subres;
    if(FAILED( renderer->get_context()->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subres ) ))
        throw BufferLockError(bind_flag);
    return mapped_subres.pData;
}

void Buffer::unlock() const
{
    renderer->get_context()->Unmap(buffer, 0);
}

Buffer::~Buffer()
{
    release_interface(buffer);
}

Vertex * VertexBuffer::lock() const
{
    return reinterpret_cast<Vertex*>(Buffer::lock());
}
void VertexBuffer::set() const
{
    static const unsigned stride = sizeof(Vertex);
    static const unsigned offset = 0;
    ID3D11Buffer * buffers[1] = { get_buffer() };
    get_renderer()->get_context()->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
}

Index * IndexBuffer::lock() const
{
    return reinterpret_cast<Index*>(Buffer::lock());
}

void IndexBuffer::set() const
{
    get_renderer()->get_context()->IASetIndexBuffer(get_buffer(), INDEX_FORMAT, 0);
}

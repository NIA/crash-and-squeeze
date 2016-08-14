#include "Buffer.h"

// Buffer implementation:
BufferImpl::BufferImpl(IRenderer * renderer, unsigned bind_flag, const void *buffer_data, unsigned items_count, unsigned item_size, bool dynamic, bool staging_backed) :
    renderer(renderer), buffer(nullptr), staging_buffer(nullptr), bind_flag(bind_flag), items_count(items_count),
    dynamic(dynamic), staging_backed(staging_backed), locked_buffer(nullptr), update_after_unlock(false)
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

    if (staging_backed)
    {
        desc.BindFlags = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        if (dynamic) desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
        if (FAILED( renderer->get_device()->CreateBuffer(&desc, pData, &staging_buffer)))
            throw BufferInitError(0);
    }
}

void * BufferImpl::lock(BufferLockType lock_type)
{
    if (nullptr != locked_buffer)
        // TODO: add error message to BufferLockError
        throw BufferLockError(bind_flag, "already locked"); // already locked

    D3D11_MAP map_type = D3D11_MAP_READ;
    switch (lock_type)
    {
    case LOCK_OVERWRITE:
        if (!dynamic)
            throw BufferLockError(bind_flag, "cannot LOCK_OVERWRITE: buffer is not writable (dynamic = false)"); // cannot write if not dynamic
        locked_buffer = buffer;
        update_after_unlock = false;
        map_type = D3D11_MAP_WRITE_DISCARD;
        break;
    case LOCK_READ:
        if (!staging_backed)
            throw BufferLockError(bind_flag, "cannot LOCK_READ: buffer is not readable (because not staging-backed)"); // cannot read if not staging_backed
        locked_buffer = staging_buffer;
        update_after_unlock = false;
        map_type = D3D11_MAP_READ;
        break;
    case LOCK_READ_WRITE:
        if (!staging_backed)
            throw BufferLockError(bind_flag, "cannot LOCK_READ_WRITE: buffer is not readable (because not staging-backed)"); // cannot read if not staging_backed
        if (!dynamic)
            throw BufferLockError(bind_flag, "cannot LOCK_READ_WRITE: buffer is not writable (dynamic = false)"); // cannot write if not dynamic
        locked_buffer = staging_buffer;
        update_after_unlock = true;
        map_type = D3D11_MAP_READ_WRITE;
        break;
    default:
        throw BufferLockError(bind_flag, "cannot lock: incorrect BufferLockType " + lock_type); // incorrect lock_type value
    }
    D3D11_MAPPED_SUBRESOURCE mapped_subres;
    HRESULT res = renderer->get_context()->Map(locked_buffer, 0, map_type, 0, &mapped_subres);
    if(FAILED( res ))
    {
        locked_buffer = nullptr;
        throw BufferLockError(bind_flag, "direct3d error code " + res);
    }
    return mapped_subres.pData;
}

void BufferImpl::unlock()
{
    if (nullptr == locked_buffer)
        return; // trying to unlock already unlocked buffer, ignore

    renderer->get_context()->Unmap(locked_buffer, 0);
    if (update_after_unlock)
    {
        renderer->get_context()->CopyResource(buffer, locked_buffer);
    }
    locked_buffer = nullptr;
}

BufferImpl::~BufferImpl()
{
    release_interface(buffer);
    release_interface(staging_buffer);
}

void VertexBuffer::set() const
{
    static const unsigned stride = sizeof(Vertex);
    static const unsigned offset = 0;
    ID3D11Buffer * buffers[] = { get_impl()->get_buffer() };
    get_impl()->get_renderer()->get_context()->IASetVertexBuffers(0, array_size(buffers), buffers, &stride, &offset);
}

void IndexBuffer::set() const
{
    get_impl()->get_renderer()->get_context()->IASetIndexBuffer(get_impl()->get_buffer(), INDEX_FORMAT, 0);
}

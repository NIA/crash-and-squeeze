#pragma once
#include "main.h"
#include "Vertex.h"

class Renderer;

enum BufferLockType
{
    // You can only overwrite _entire_ contents of buffer and cannot read.
    // This is the only lock type available with staging_backed = false.
    LOCK_OVERWRITE,
    // You can only read from buffer.
    // The buffer must be initialized with staging_backed = true
    LOCK_READ,
    // You can both read and write from buffer. The updated data will be transferred to GPU after `unlock`.
    // The buffer must be initialized with staging_backed = true
    LOCK_READ_WRITE,
};

// DO NOT USE this class directly, use Buffer<T> instead
class BufferImpl;

template <class T>
class Buffer
{
private:
    BufferImpl * impl;
protected:
    BufferImpl * get_impl() const { return impl; }
public:
    /*
     * Initializes buffer according to following parameters:
     *  renderer - the parent renderer
     *  bind_flag - how to bind this buffer (e.g. D3D11_BIND_VERTEX_BUFFER for vertex buffer, etc)
     *  buffer_data - initial data to fill buffer, can be nullptr.
     *  items_count - the size of initial data, ignored if buffer_data == nullptr
     *  dynamic - is the buffer going to be updated
     *  staging_backed - is an additional buffer with D3D11_USAGE_STAGING created to enable LOCK_READ_WRITE and LOCK_READ lock types (see Buffer::LockType enum)
     */
    Buffer(Renderer * renderer, unsigned bind_flag, const T *buffer_data, unsigned items_count, bool dynamic = true, bool staging_backed = true);
    unsigned get_items_count() const;

    // this method must be implemented in a subclass
    virtual void set() const = 0;
    
    T * lock(BufferLockType lock_type) const;
    void unlock() const;
    
    virtual ~Buffer() { delete impl; }
private:
    DISABLE_COPY_T(Buffer, T);
};

class VertexBuffer : public Buffer<Vertex>
{
public:
    /*
     * Initializes buffer according to following parameters:
     *  renderer - the parent renderer
     *  vertices - initial data to fill buffer, can be nullptr.
     *  vertices_count - count of vertices in `vertices`, ignored if vertices == nullptr
     *  dynamic - is the buffer going to be updated
     *  staging_backed - is an additional buffer with D3D11_USAGE_STAGING created to enable LOCK_READ_WRITE and LOCK_READ lock types (see Buffer::LockType enum)
     */
    VertexBuffer(Renderer * renderer, const Vertex * vertices, unsigned vertices_count, bool dynamic = true, bool staging_backed = true):
        Buffer(renderer, D3D11_BIND_VERTEX_BUFFER, vertices, vertices_count, dynamic, staging_backed)
    {}

    virtual void set() const override;
private:
    DISABLE_COPY(VertexBuffer);
};

class IndexBuffer : public Buffer<Index>
{
public:
    /*
     * Initializes buffer according to following parameters:
     *  renderer - the parent renderer
     *  indices - initial data to fill buffer, can be nullptr.
     *  indices_count - count of indices in `indices`, ignored if indices == nullptr
     *  dynamic - is the buffer going to be updated
     *  staging_backed - is an additional buffer with D3D11_USAGE_STAGING created to enable LOCK_READ_WRITE and LOCK_READ lock types (see Buffer::LockType enum)
     */
    IndexBuffer(Renderer * renderer, const Index * indices, unsigned indices_count, bool dynamic = true, bool staging_backed = true) :
        Buffer(renderer, D3D11_BIND_INDEX_BUFFER, indices, indices_count, dynamic, staging_backed)
    {}

    virtual void set() const override;
private:
    DISABLE_COPY(IndexBuffer);
};

enum ConstantBufferSetFlags
{
    SET_FOR_VS = 1 << 0,
    SET_FOR_PS = 1 << 1,
    SET_FOR_GS = 1 << 2
};

template <class BufferStruct>
class ConstantBuffer : public Buffer<BufferStruct>
{
private:
    unsigned start_slot;
    unsigned set_flags; // specifies which shaders this buffer will be set to, should be a combination of ConstantBufferSetFlags
public:
    /*
     * Initializes buffer according to following parameters:
     *  renderer - the parent renderer
     *  initial_data - initial data to fill buffer, can be nullptr.
     *  dynamic - is the buffer going to be updated
     *  staging_backed - is an additional buffer with D3D11_USAGE_STAGING created to enable LOCK_READ_WRITE and LOCK_READ lock types (see Buffer::LockType enum)
     *                   NB: by default staging_backed is false (as opposed to VertexBuffer/IndexBuffer) because constant buffer is usually updated with LOCK_OVERWRITE
     */
    ConstantBuffer(Renderer * renderer, const BufferStruct *initial_data, unsigned set_flags, unsigned start_slot, bool dynamic = true, bool staging_backed = false) :
        Buffer(renderer, D3D11_BIND_CONSTANT_BUFFER, initial_data, 1, dynamic, staging_backed),
        set_flags(set_flags), start_slot(start_slot)
    {}

    virtual void set() const override
    {
        ID3D11Buffer * buffers[] = {get_impl()->get_buffer()};
        if (set_flags & SET_FOR_VS)
            get_impl()->get_renderer()->get_context()->VSSetConstantBuffers(start_slot, 1, buffers);
        if (set_flags & SET_FOR_PS)
            get_impl()->get_renderer()->get_context()->PSSetConstantBuffers(start_slot, 1, buffers);
        if (set_flags & SET_FOR_GS)
            get_impl()->get_renderer()->get_context()->GSGetConstantBuffers(start_slot, 1, buffers);
    }

private:
    DISABLE_COPY_T(ConstantBuffer, BufferStruct)
};

// Buffer implementation

class BufferImpl
{
private:
    Renderer * renderer;

    ID3D11Buffer * buffer;
    unsigned bind_flag;
    unsigned items_count;
    bool dynamic;

private:
    ID3D11Buffer * staging_buffer;
    bool staging_backed;

    // fields used to control locking:
    ID3D11Buffer * locked_buffer;
    bool update_after_unlock;

public:
    BufferImpl(Renderer * renderer, unsigned bind_flag, const void *buffer_data, unsigned items_count, unsigned item_size, bool dynamic = true, bool staging_backed = false);

    Renderer * get_renderer() const { return renderer; }
    ID3D11Buffer * get_buffer() const { return buffer; }

    unsigned get_items_count() const { return items_count; }

    void * lock(BufferLockType lock_type);
    void unlock();

    virtual ~BufferImpl();
private:
    DISABLE_COPY(BufferImpl);
};

template<class T>
Buffer<T>::Buffer(Renderer * renderer, unsigned bind_flag, const T *buffer_data, unsigned items_count, bool dynamic = true, bool staging_backed = true)
    : impl(nullptr)
{
    impl = new BufferImpl(renderer, bind_flag, buffer_data, items_count, sizeof(T), dynamic, staging_backed);
}

template<class _> // template parameter is not used in this method
unsigned Buffer<_>::get_items_count() const { return impl->get_items_count(); }

template<class T>
T * Buffer<T>::lock(BufferLockType lock_type) const { return reinterpret_cast<T*>(impl->lock(lock_type)); }

template<class _> // template parameter is not used in this method
void Buffer<_>::unlock() const { impl->unlock(); }

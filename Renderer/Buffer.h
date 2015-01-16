#pragma once
#include "main.h"
#include "Vertex.h"

class Renderer;

class Buffer
{
private:
    Renderer * renderer;

    ID3D11Buffer * buffer;
    unsigned bind_flag;
    unsigned items_count;
    bool dynamic;

protected:
    ID3D11Buffer * get_buffer() const { return buffer; }
    Renderer * get_renderer() const { return renderer; }
public:
    Buffer(Renderer * renderer, unsigned bind_flag, const void *buffer_data, unsigned items_count, unsigned item_size, bool dynamic);
    unsigned get_items_count() const { return items_count; }

    virtual void set() const = 0;
    // TODO: add possibility to have a copy of buffer in a staging buffer and add corresponding lock- methods
    void * lock() const;
    void unlock() const;
    
    virtual ~Buffer();
private:
    DISABLE_COPY(Buffer);
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(Renderer * renderer, const Vertex * vertices, unsigned vertices_count, bool dynamic):
        Buffer(renderer, D3D11_BIND_VERTEX_BUFFER, vertices, vertices_count, sizeof(Vertex), dynamic)
    {}

    virtual void set() const override;
    // TODO: avoid hiding, have two different methods instead
    Vertex * lock() const; // NB: this method hides Buffer::lock
private:
    DISABLE_COPY(VertexBuffer);
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer(Renderer * renderer, const Index * indices, unsigned indices_count, bool dynamic) :
        Buffer(renderer, D3D11_BIND_INDEX_BUFFER, indices, indices_count, sizeof(Index), dynamic)
    {}
    virtual void set() const override;
    Index * lock() const; // NB: this method hides Buffer::lock
private:
    DISABLE_COPY(IndexBuffer);
};

enum SetFlags
{
    SET_FOR_VS = 1 << 0,
    SET_FOR_PS = 1 << 1,
    SET_FOR_GS = 1 << 2
};

template <class BufferStruct>
class ConstantBuffer : public Buffer
{
private:
    unsigned start_slot;
    unsigned set_flags; // specifies which shaders this buffer will be set to, should be a combination of SetFlags
public:

    ConstantBuffer(Renderer * renderer, const BufferStruct *initial_data, unsigned set_flags, unsigned start_slot, bool dynamic) :
        Buffer(renderer, D3D11_BIND_CONSTANT_BUFFER, initial_data, 1, sizeof(BufferStruct), dynamic),
        set_flags(set_flags), start_slot(start_slot)
    {}

    virtual void set() const override
    {
        ID3D11Buffer * buffers[] = {get_buffer()};
        if (set_flags & SET_FOR_VS)
            get_renderer()->get_context()->VSSetConstantBuffers(start_slot, 1, buffers);
        if (set_flags & SET_FOR_PS)
            get_renderer()->get_context()->PSSetConstantBuffers(start_slot, 1, buffers);
        if (set_flags & SET_FOR_GS)
            get_renderer()->get_context()->GSGetConstantBuffers(start_slot, 1, buffers);
    }

    BufferStruct * lock() // NB: this method hides Buffer::lock
    {
        return reinterpret_cast<BufferStruct*>(Buffer::lock());
    }

private:
    DISABLE_COPY_T(ConstantBuffer, BufferStruct)
};

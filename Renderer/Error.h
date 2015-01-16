#pragma once
#include <exception>
#include <tchar.h>
#include <cstring>
#include <d3d11.h>

class RuntimeError : public std::exception
{
private:
    // Message is a short line that is shown in a message box
    const TCHAR * message;
    // Log entry is a detailed error description that is written into log
    const char * log_entry;
protected:
    // sets log_entry = allocated string which is part1 + part2. If part2_length is given, it is used as length of part2, otherwise strlen(part2) is used
    void set_log_entry(const char * part1, const char * part2 = "", int part2_length = -1)
    {
        log_entry = build_log_entry(part1, part2, part2_length);
    }
public:
    RuntimeError(const TCHAR *message) :  message(message), log_entry(nullptr) {}
    const TCHAR *get_message() const { return message; }
    virtual const char *get_log_entry() const
    {
        if(nullptr != log_entry)
            return log_entry;
        else
            return "application crash";
    }
    virtual const char *what() const override { return get_log_entry(); }

    // returns newly allocated string which is part1 + part2. If part2_length is given, it is used as length of part2, otherwise strlen(part2) is used
    static const char * build_log_entry(const char * part1, const char * part2 = "", int part2_length = -1)
    {
        // TODO: switch to std::string to avoid this madness?
        if (nullptr == part1 || nullptr == part2)
            return nullptr;
        int total_size = strlen(part1) + (part2_length < 0 ? strlen(part2) : part2_length) + 1;
        char * log_entry_ = new char[total_size]; // TODO: is it good to allocate a new string always, even when only part1 is given?
        strcpy_s(log_entry_, total_size, part1);
        strcat_s(log_entry_, total_size, part2);
        return log_entry_;
    }
    virtual ~RuntimeError()
    {
        delete log_entry;
    }
};

class WindowInitError : public RuntimeError
{
public:
    WindowInitError() : RuntimeError( _T("Error while creating window") ) {}
};
class RendererInitError : public RuntimeError
{
public:
    RendererInitError(const char * stage = "D3D11CreateDevice")
        : RuntimeError( _T("Error while initializing Renderer, see log") )
    {
        set_log_entry("Renderer init failed at stage: ", stage);
    }
};
class VertexDeclarationInitError : public RuntimeError
{
public:
    VertexDeclarationInitError() : RuntimeError( _T("Error while creating vertex declaration") ) {}
};
class ShaderCompileError : public RuntimeError
{
public:
    ShaderCompileError(ID3DBlob *errors)
        : RuntimeError( _T("Error while compiling shader, see log") )
    {
        if(nullptr != errors)
        {
            set_log_entry("Cannot compile vertex shader. Errors:\n", reinterpret_cast<char*>(errors->GetBufferPointer()), errors->GetBufferSize());
            errors->Release(); // TODO: do not release manually, use ComPtr or CComPtr
        }
    }
    ShaderCompileError(const char * log_entry)
        : RuntimeError( _T("Error while compiling shader, see log") )
    {
        set_log_entry(log_entry);
    }
};
class ShaderInitError : public RuntimeError
{
public:
    ShaderInitError() : RuntimeError( _T("Error while creating shader") ) {}
};
class BufferError : public RuntimeError
{
public:
    BufferError(const char * log_entry_start, unsigned bind_flag = D3D11_BIND_VERTEX_BUFFER)
        : RuntimeError( _T("Error while working with D3D buffer, see log") )
    {
        const char * log_entry_end = " buffer";
        if (bind_flag & D3D11_BIND_VERTEX_BUFFER)
            log_entry_end = " vertex buffer";
        else if (bind_flag & D3D11_BIND_INDEX_BUFFER)
            log_entry_end = " index buffer";
        set_log_entry(log_entry_start, log_entry_end);
    }
};
class BufferInitError : public BufferError
{
public:
    BufferInitError(unsigned bind_flag = D3D11_BIND_VERTEX_BUFFER) : BufferError("Failed to create", bind_flag) {}
};
class BufferLockError : public BufferError
{
public:
   BufferLockError(unsigned bind_flag = D3D11_BIND_VERTEX_BUFFER) : BufferError( "Failed to lock", bind_flag ) {}
};
class RenderError : public RuntimeError
{
public:
    RenderError() : RuntimeError( _T("Error while rendering scene") ) {}
};
class RenderStateError : public RuntimeError
{
public:
    RenderStateError() : RuntimeError( _T("Error while setting render state") ) {}
};
class PhysicsError : public RuntimeError
{
public:
    PhysicsError() : RuntimeError( _T("Physics subsystem error, see log file for details") ) {}
};
class ForcesError : public RuntimeError
{
public:
    ForcesError() : RuntimeError( _T("Forces not set") ) {}
};
class PerformanceFrequencyError : public RuntimeError
{
public:
    PerformanceFrequencyError() : RuntimeError( _T("Frequency of Performance Counter is 0 (unsuppoted)") ) {}
};
class NullPointerError : public RuntimeError
{
public:
    NullPointerError(const TCHAR * message = _T("Unexpected NULL pointer")) : RuntimeError( message ) {}
};
class D3DXFontError : public RuntimeError
{
public:
    D3DXFontError() : RuntimeError( _T("Error while creating D3DX font") ) {}
};
class OutOfRangeError : public RuntimeError
{
public:
    OutOfRangeError(const TCHAR * message =  _T("Index out of range")) : RuntimeError( message ) {}
};
class ThreadError : public RuntimeError
{
public:
    ThreadError() : RuntimeError( _T("Error while creating thread") ) {}
};
class DeadLockError : public RuntimeError
{
public:
    DeadLockError() : RuntimeError( _T("Threads are dead-locked!") ) {}
};
class MeshError : public RuntimeError
{
public:
    MeshError() : RuntimeError( _T("Error while loading mesh from file") ) {}
};
class NotYetImplementedError : public RuntimeError
{
public:
    NotYetImplementedError(const TCHAR * message = _T("Feature not yet implemented")) : RuntimeError( message ) {}
};

inline void check_render( HRESULT res )
{
    if( FAILED(res) )
        throw RenderError();
}

inline void check_state( HRESULT res )
{
    if( FAILED(res) )
        throw RenderStateError();
}

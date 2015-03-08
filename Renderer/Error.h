#pragma once
#include <exception>
#include <tchar.h>
#include <string>
#include <d3d11.h>

// Define a handy alias `tstring`
namespace std
{
#if defined UNICODE || defined _UNICODE
    typedef wstring tstring;
#else // defined UNICODE || defined _UNICODE
    typedef string  tstring;
#endif // defined UNICODE || defined _UNICODE
}

class RuntimeError : public std::exception
{
private:
    // Message is a short line that is shown in a message box
    std::tstring message;
    // Log entry is a detailed error description that is written into log
    std::string log_entry;
public:
    RuntimeError(const std::tstring &message, const std::string &log_entry = "application crash") :  message(message), log_entry(log_entry) {}
    void set_log_entry(const std::string &str) { log_entry = str; }
    void set_log_entry(const char * str) { log_entry = str; }
    
    const std::tstring & get_message() const { return message; }
    virtual const std::string & get_log_entry() const { return log_entry; }
    virtual const char *what() const override { return get_log_entry().c_str(); }

    virtual ~RuntimeError() {}
};

// A helper to easily call RuntimeError with same string literal argument for message and log_entry:
// like RuntimeError(RT_ERR_ARGS("some string")) instead of RuntimeError(_T("some string"), "some string")
#define RT_ERR_ARGS(str) _T(str), str

class WindowInitError : public RuntimeError
{
public:
    WindowInitError() : RuntimeError( RT_ERR_ARGS("Error while creating window") ) {}
};
class RendererInitError : public RuntimeError
{
public:
    RendererInitError(const std::string & stage = "D3D11CreateDevice")
        : RuntimeError( _T("Error while initializing Renderer, see log"), "Renderer init failed at stage: " + stage ) {}
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
            set_log_entry("Cannot compile vertex shader. Errors:\n" + std::string(reinterpret_cast<char*>(errors->GetBufferPointer()), errors->GetBufferSize()));
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
    ShaderInitError() : RuntimeError( RT_ERR_ARGS("Error while creating shader") ) {}
};
class BufferError : public RuntimeError
{
public:
    BufferError(const char * log_entry_start, unsigned bind_flag = D3D11_BIND_VERTEX_BUFFER)
        : RuntimeError( _T("Error while working with D3D buffer, see log") )
    {
        std::string log_entry_end = " buffer";
        if (bind_flag & D3D11_BIND_VERTEX_BUFFER)
            log_entry_end = " vertex buffer";
        else if (bind_flag & D3D11_BIND_INDEX_BUFFER)
            log_entry_end = " index buffer";
        else if (bind_flag & D3D11_BIND_CONSTANT_BUFFER)
            log_entry_end = " constant buffer";
        else if (bind_flag == 0)
            log_entry_end = " staging (copy) buffer";
        set_log_entry(log_entry_start + log_entry_end);
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
    RenderError() : RuntimeError( RT_ERR_ARGS("Error while rendering scene") ) {}
};
class RenderStateError : public RuntimeError
{
public:
    RenderStateError() : RuntimeError( RT_ERR_ARGS("Error while setting render state") ) {}
};
class PhysicsError : public RuntimeError
{
public:
    PhysicsError() : RuntimeError( RT_ERR_ARGS("Physics subsystem error, see log file for details") ) {}
};
class ForcesError : public RuntimeError
{
public:
    ForcesError() : RuntimeError( RT_ERR_ARGS("Forces not set") ) {}
};
class PerformanceFrequencyError : public RuntimeError
{
public:
    PerformanceFrequencyError() : RuntimeError( RT_ERR_ARGS("Frequency of Performance Counter is 0 (unsuppoted)") ) {}
};
class NullPointerError : public RuntimeError
{
public:
    NullPointerError(const TCHAR * message = _T("Unexpected NULL pointer"), char * log_entry = "Unexpected NULL pointer") : RuntimeError( message, log_entry ) {}
};
class D3DXFontError : public RuntimeError
{
public:
    D3DXFontError() : RuntimeError( RT_ERR_ARGS("Error while creating D3DX font") ) {}
};
class OutOfRangeError : public RuntimeError
{
public:
    OutOfRangeError(const TCHAR * message =  _T("Index out of range"), const char * log_entry = "Index out of range") : RuntimeError( message, log_entry ) {}
};
class ThreadError : public RuntimeError
{
public:
    ThreadError() : RuntimeError( RT_ERR_ARGS("Error while creating thread") ) {}
};
class DeadLockError : public RuntimeError
{
public:
    DeadLockError() : RuntimeError( RT_ERR_ARGS("Threads are dead-locked!") ) {}
};
class MeshError : public RuntimeError
{
public:
    MeshError(const char * filename, const std::string & log_entry_start = "Failed to load mesh from file", unsigned line_no = 0)
        : RuntimeError( std::tstring(_T("Error while loading mesh from file")) )
    {
        if (line_no != 0)
            set_log_entry(log_entry_start + " " + filename + " (line " + std::to_string(line_no) + ")");
        else
            set_log_entry(log_entry_start);
    }
};
class AffinityError : public RuntimeError
{
public:
    AffinityError() : RuntimeError( RT_ERR_ARGS("Failed to set thread affinity mask") ) {}
};
class NotYetImplementedError : public RuntimeError
{
public:
    NotYetImplementedError(const TCHAR * message = _T("Feature not yet implemented"), const char * log_entry = "Feature not yet implemented") : RuntimeError( message, log_entry ) {}
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

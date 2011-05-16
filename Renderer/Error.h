#pragma once
#include <exception>
#include <tchar.h>
#include <cstring>
#include <d3dx9.h>

class RuntimeError : public std::exception
{
private:
    const TCHAR * message;
public:
    RuntimeError(const TCHAR *message) :  message(message) {}
    const TCHAR *get_message() const { return message; }
    virtual const char *get_log_entry() const { return "application crash"; }
    virtual ~RuntimeError() {}
};

class WindowInitError : public RuntimeError
{
public:
    WindowInitError() : RuntimeError( _T("Error while creating window") ) {}
};
class D3DInitError : public RuntimeError
{
public:
    D3DInitError() : RuntimeError( _T("Error while initializing D3D device") ) {}
};
class VertexDeclarationInitError : public RuntimeError
{
public:
    VertexDeclarationInitError() : RuntimeError( _T("Error while creating vertex declaration") ) {}
};
class VertexShaderCompileError : public RuntimeError
{
private:
    char *log_entry;
public:
    VertexShaderCompileError(ID3DXBuffer *errors)
        : RuntimeError( _T("Error while compiling vertex shader") ), log_entry(NULL)
    {
        if(NULL != errors)
        {
            const char* header = "Cannot compile vertex shader. Errors:\n";
            int total_size = strlen(header) + errors->GetBufferSize();
            log_entry = new char[total_size];
            strcpy_s(log_entry, total_size, header);
            strcat_s(log_entry, total_size, reinterpret_cast<char*>(errors->GetBufferPointer()));
            errors->Release();
        }
    }
    virtual const char *get_log_entry() const
    {
        if(NULL != log_entry)
            return log_entry;
        else
            return RuntimeError::get_log_entry();
    }
    virtual ~VertexShaderCompileError()
    {
        delete log_entry;
    }
};
class VertexShaderInitError : public RuntimeError
{
public:
    VertexShaderInitError() : RuntimeError( _T("Error while creating vertex shader") ) {}
};
class VertexBufferInitError : public RuntimeError
{
public:
    VertexBufferInitError() : RuntimeError( _T("Error while creating vertex buffer") ) {}
};
class IndexBufferInitError : public RuntimeError
{
public:
    IndexBufferInitError() : RuntimeError( _T("Error while creating index buffer") ) {}
};
class VertexBufferFillError : public RuntimeError
{
public:
    VertexBufferFillError() : RuntimeError( _T("Error while filling vertex buffer") ) {}
};
class IndexBufferFillError : public RuntimeError
{
public:
    IndexBufferFillError() : RuntimeError( _T("Error while filling index buffer") ) {}
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
class VertexBufferLockError : public RuntimeError
{
public:
    VertexBufferLockError() : RuntimeError( _T("Error while locking vertex buffer for update") ) {}
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
    NullPointerError() : RuntimeError( _T("Unexpected NULL pointer") ) {}
};
class D3DXFontError : public RuntimeError
{
public:
    D3DXFontError() : RuntimeError( _T("Error while creating D3DX font") ) {}
};
class OutOfRangeError : public RuntimeError
{
public:
    OutOfRangeError() : RuntimeError( _T("Index out of range") ) {}
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
class ThreadsCountError : public RuntimeError
{
public:
    ThreadsCountError() : RuntimeError( _T("Invalid threads count given") ) {}
};
class AffinityError : public RuntimeError
{
public:
    AffinityError() : RuntimeError( _T("Failed to set thread affinity mask") ) {}
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

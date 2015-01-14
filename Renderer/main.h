#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#undef ERROR // because WinGDI.h defines this and it's awful!
#include <cstdlib>
#include <ctime>
#include <crtdbg.h>
#include "Error.h"
#include <fstream>
#include "Math/vector.h"

// usage: #pragma WARNING(FIXME: Code removed because...) - taken from http://goodliffe.blogspot.ru/2009/07/c-how-to-say-warning-to-visual-studio-c.html
#define WARNING(desc) message(__FILE__ "(" _STRINGIZE(__LINE__) ") : warning: " #desc)

#define DISABLE_COPY(ClassName) ClassName(const ClassName &); ClassName &operator=(const ClassName &); // Copying forbidden!

// a helper to release D3D interface if it is not NULL
template<class Iface> inline void release_interface(Iface *& iface)
{
    if( iface != nullptr )
    {
        iface->Release();
        iface = nullptr;
    }
}

// a helper to call delete on pointer to an item (not array !) if it is not NULL
template<class Type> void delete_pointer(Type *& pointer)
{
    if( pointer != nullptr)
    {
        delete pointer;
        pointer = nullptr;
    }
}

// a helper to call delete[] on pointer to an array(!) if it is not NULL
template<class Type> void delete_array(Type *& dynamic_array)
{
    if( dynamic_array != nullptr)
    {
        delete[] dynamic_array;
        dynamic_array = nullptr;
    }
}

// a helper to find out a size of an array defined with `array[]={...}' without doing `sizeof(array)/sizeof(array[0])'
template<size_t SIZE, class T> inline size_t array_size(T (&array)[SIZE])
{
    UNREFERENCED_PARAMETER(array);
    return SIZE;
}

typedef DWORD Index;

// a helper for filling index buffers
inline void add_triangle( Index i1, Index i2, Index i3, Index *indices, DWORD &current_index, Index offset = 0 )
{
    indices[current_index++] = i1 + offset;
    indices[current_index++] = i2 + offset;
    indices[current_index++] = i3 + offset;
}

inline D3DXVECTOR3 math_vector_to_d3dxvector(const CrashAndSqueeze::Math::Vector &v)
{
    return D3DXVECTOR3(static_cast<float>(v[0]),
                       static_cast<float>(v[1]),
                       static_cast<float>(v[2]));
}

inline CrashAndSqueeze::Math::Vector d3dxvector_to_math_vector(const D3DXVECTOR3 &v)
{
    return CrashAndSqueeze::Math::Vector(static_cast<CrashAndSqueeze::Math::Real>(v.x),
                                         static_cast<CrashAndSqueeze::Math::Real>(v.y),
                                         static_cast<CrashAndSqueeze::Math::Real>(v.z));
}

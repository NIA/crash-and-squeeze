#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#undef ERROR // because WinGDI.h defines this and it's awful!
#undef min
#undef max
#include "Error.h"
#include "Math/vector.h"

// usage: #pragma WARNING(FIXME: Code removed because...) - taken from http://goodliffe.blogspot.ru/2009/07/c-how-to-say-warning-to-visual-studio-c.html
#define WARNING(desc) message(__FILE__ "(" _STRINGIZE(__LINE__) ") : warning: " #desc)

#define DISABLE_COPY(ClassName) ClassName(const ClassName &); ClassName &operator=(const ClassName &); // Copying forbidden!
#define DISABLE_COPY_T(ClassName, T) ClassName(const ClassName<T> &); ClassName<T> &operator=(const ClassName<T> &); // Copying forbidden!

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

typedef unsigned Index;
typedef DirectX::XMFLOAT3   float3;
typedef DirectX::XMFLOAT4   float4;
typedef DirectX::XMFLOAT4X4 float4x4;

// TODO: move the following vector operations to another header (and maybe above typedefs too)

// performs a+=b by converting a to XMVECTOR (and back)
inline float3 & operator+=(float3 & a /*in/out*/, const DirectX::XMVECTOR & b)
{
    DirectX::XMStoreFloat3(&a, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&a), b));
    return a;
}

// performs a+=b by converting a and b to XMVECTOR (and back)
inline float3 & operator+=(float3 & a /*in/out*/, const float3 & b)
{
    a += DirectX::XMLoadFloat3(&b);
    return a;
}

// performs a + b by converting a and b to XMVECTOR (and back)
inline float3 operator+(const float3 &a, const float3 &b)
{
    float3 res = a;
    res += b;
    return res;
}

// performs a - b by converting a and b to XMVECTOR (and back)
inline float3 operator-(const float3 &a, const float3 &b)
{
    float3 res;
    DirectX::XMStoreFloat3(&res, DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&a), DirectX::XMLoadFloat3(&b)));
    return res;
}

// performs v * a by converting v and a to XMVECTOR (and back)
inline float3 operator*(const float3 &v, const float a)
{
    float3 res;
    DirectX::XMStoreFloat3(&res, DirectX::XMVectorMultiply(DirectX::XMLoadFloat3(&v), DirectX::XMVectorReplicate(a)));
    return res;
}

inline bool operator==(const float3 & v1, const float3 & v2)
{
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

// a helper for filling index buffers
inline void add_triangle( Index i1, Index i2, Index i3, Index *indices, Index &current_index, Index offset = 0 )
{
    indices[current_index++] = i1 + offset;
    indices[current_index++] = i2 + offset;
    indices[current_index++] = i3 + offset;
}

inline float3 math_vector_to_float3(const CrashAndSqueeze::Math::Vector &v)
{
    return float3(static_cast<float>(v[0]),
                  static_cast<float>(v[1]),
                  static_cast<float>(v[2]));
}

inline float4 math_vector_to_float4(const CrashAndSqueeze::Math::Vector &v)
{
    return float4(static_cast<float>(v[0]),
                  static_cast<float>(v[1]),
                  static_cast<float>(v[2]),
                  0);
}


inline CrashAndSqueeze::Math::Vector float3_to_math_vector(const float3 &v)
{
    return CrashAndSqueeze::Math::Vector(static_cast<CrashAndSqueeze::Math::Real>(v.x),
                                         static_cast<CrashAndSqueeze::Math::Real>(v.y),
                                         static_cast<CrashAndSqueeze::Math::Real>(v.z));
}

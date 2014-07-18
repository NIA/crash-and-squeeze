#pragma once
#include "Logging/logger.h"
#include "Collections/array.h"

// ** Options (preprocessor switches) **

// Set to 0 for more conservative graphical vertices update:
// use equilibrium transformation (R*Sp) instead of total deformation (A)
// to hide some oscillations (looks more realistic for metal-like objects)
#define CAS_GRAPHICAL_TRANSFORM_TOTAL    1

namespace CrashAndSqueeze
{
    namespace Core
    {
        // floating type for storing graphical vertex components in vertex buffer
        typedef float VertexFloat;
        // floating type for defining mass of physical vertex
        typedef double MassFloat;
        // integer type for storing cluster index of graphical vertex in vertex buffer
        typedef unsigned char ClusterIndex;

        typedef Collections::Array<int> IndexArray;

        template<class T>
        inline void ignore_unreferenced(T parameter) { parameter; }

        inline const void *add_to_pointer(const void *pointer, int offset)
        {
            return reinterpret_cast<const void*>( reinterpret_cast<const char*>(pointer) + offset );
        }
        inline void *add_to_pointer(void *pointer, int offset)
        {
            return reinterpret_cast<void*>( reinterpret_cast<char*>(pointer) + offset );
        }
    }
}

#pragma once
#include "Logging/logger.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        typedef float VertexFloat;
        typedef double MassFloat;

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

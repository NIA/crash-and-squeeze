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
    }
}

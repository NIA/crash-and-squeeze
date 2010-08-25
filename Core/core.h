#pragma once
#include "Logging/logger.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        typedef float VertexFloat;
        typedef double MassFloat;

        template<class T>
        inline void ignore_unreferenced(T parameter) { parameter; }
    }
}

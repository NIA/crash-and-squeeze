#pragma once
#include "vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        struct PhysicalVertex
        {
            // position of vertex in global coordinate system
            Math::Vector pos;
            double mass;
            Math::Vector velocity;
            // TODO: thread-safe cluster addition: velocity_additions[]...
        };
    }
}
#pragma once
#include "Core/core.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        struct PhysicalVertex
        {
            // position of vertex in global coordinate system
            Math::Vector pos;
            Math::Real mass;
            Math::Vector velocity;
            // TODO: thread-safe cluster addition: velocity_additions[]...
            
            PhysicalVertex( Math::Vector pos,
                            Math::Real mass,
                            Math::Vector velocity = Math::Vector(0,0,0) )
                : pos(pos), mass(mass), velocity(velocity) {}
            
            PhysicalVertex()
                : mass(0) {}
        };
    }
}

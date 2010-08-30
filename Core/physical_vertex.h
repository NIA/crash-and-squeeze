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
            // Number of clusters which include current vertex
            int including_clusters_num;
            // TODO: thread-safe cluster addition: velocity_additions[], velocity_addition_coeffs[]...
            Math::Vector velocity_addition;
            // specifies, which part of velocity_addition will be actually added to velocity
            // (while the whole velocity_addition is used for position correction)
            Math::Real velocity_addition_coeff;
            
            PhysicalVertex( Math::Vector pos,
                            Math::Real mass,
                            Math::Vector velocity = Math::Vector(0,0,0) )
                : pos(pos), mass(mass), velocity(velocity), velocity_addition(Math::Vector::ZERO),
                  velocity_addition_coeff(1), including_clusters_num(0) {}
            
            PhysicalVertex()
                : mass(0), pos(Math::Vector::ZERO), velocity(Math::Vector::ZERO), velocity_addition(Math::Vector::ZERO),
                  velocity_addition_coeff(1), including_clusters_num(0) {}
        };
    }
}

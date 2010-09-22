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
            int nearest_cluster_index;
            // TODO: thread-safe cluster addition: velocity_additions[], velocity_addition_coeffs[]...
            Math::Vector velocity_addition;
            
            static const int NOT_IN_A_CLUSTER = -2;

            PhysicalVertex( Math::Vector pos,
                            Math::Real mass,
                            Math::Vector velocity = Math::Vector(0,0,0) )
                : pos(pos), mass(mass), velocity(velocity), velocity_addition(Math::Vector::ZERO),
                  including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER) {}
            
            PhysicalVertex()
                : mass(0), pos(Math::Vector::ZERO), velocity(Math::Vector::ZERO), velocity_addition(Math::Vector::ZERO),
                  including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER) {}
        };
    }
}

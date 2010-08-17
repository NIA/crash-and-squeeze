#pragma once
#include "vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class Cluster
        {
        private:
            // array of vertices belonging to the cluster,
            // represented as indices in model's vertex array
            int *vertex_indices;
            // number of vertices in vertex_indices
            int vertices_num;
            
            // center of mass of vertices
            Math::Vector center_of_mass;
            // array, containing initial offsets of positions of vertices
            // referenced in vertex_indices from cluster's center of mass
            Math::Vector *initial_vertex_offset_positions;
            
            // TODO: thread-safe cluster addition: PhysicalVertexMappingInfo...
        };
    }
}
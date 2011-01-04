#pragma once
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An interface of model, having vertices and clusters
        class IModel
        {
        public:
            // returns number of vertices
            virtual int get_vertices_num() const = 0;
            // returns equilibrium position, moved so that to match immovable initial positions
            virtual Math::Vector get_vertex_equilibrium_pos(int index) const = 0;
            // returns initial position of vertex (constant)
            virtual Math::Vector get_vertex_initial_pos(int index) const = 0;
        };
    }
}

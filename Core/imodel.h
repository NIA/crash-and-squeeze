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
            virtual int get_vertices_num() const = 0;
            virtual const Math::Vector & get_vertex_equilibrium_pos(int index) const = 0;
            virtual const Math::Vector & get_vertex_initial_pos(int index) const = 0;
        };
    }
}

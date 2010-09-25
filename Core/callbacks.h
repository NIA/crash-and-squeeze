#pragma once
#include "Math/floating_point.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    class Math::Vector;

    namespace Core
    {
        // Callback to notify about cluster deformation
        typedef void (* ClusterDeformationCallback)(const Math::Vector &pos, const Math::Vector &size, Math::Real value);

        typedef Collections::Array<int> IndexArray;

        // Callback to notify about deformation of some area, defined by a "shape" - a set of vertices
        typedef void (* ShapeDeformationCallback)(const IndexArray &vertex_indices, Math::Real value, void * extra_data);
    }
}

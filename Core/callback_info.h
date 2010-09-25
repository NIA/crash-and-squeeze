#pragma once
#include "Math/floating_point.h"
#include "Core/callbacks.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        struct IndexWithWeight
        {
            int index;
            Math::Real weight;

            IndexWithWeight() : index(0), weight(0) {}
            IndexWithWeight(int index, Math::Real weight) : index(index), weight(weight) {}
        };
        typedef Collections::Array<IndexWithWeight> IndexWithWeightArray;

        struct CallbackInfo
        {
            
            ShapeDeformationCallback callback;
            const IndexArray *vertex_indices;
            void *extra_data;
            Math::Real threshold;
            
            static const int INITIAL_ALLOCATED_CLUSTERS = 10;
            IndexWithWeightArray clusters_with_weights;

            CallbackInfo() : clusters_with_weights(INITIAL_ALLOCATED_CLUSTERS),
                             vertex_indices(NULL),
                             threshold(1)
            {}
        };
        typedef Collections::Array<CallbackInfo> CallbackInfoArray;
    }
}

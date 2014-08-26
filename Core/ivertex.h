#pragma once
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract vertex, providing common functionality for both
        // PhysicalVertex and GraphicalVertex: it has position and
        // can be included into clusters.
        class IVertex
        {
        public:
            virtual const Math::Vector & get_pos() const = 0;

            virtual void include_to_one_more_cluster(int cluster_index, Math::Real weight) = 0;
            virtual int get_including_clusters_num() const = 0;
            virtual bool check_in_cluster() = 0;
        };
    }
}

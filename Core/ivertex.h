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

            // Adds vertex to another cluster with given weight. Weight may be ignored by vertex implementation (PhysicalVertex)
            virtual void include_to_one_more_cluster(int cluster_index, Math::Real weight) = 0;
            // Returns total number of clusters this vertex belongs to
            virtual int get_including_clusters_num() const = 0;
            // Returns the index of i'th cluster this vertex belongs to. Can be identically 0 if the vertex doesn't store these indices (PhysicalVertex)
            virtual ClusterIndex get_including_cluster_index(int i) const = 0;
            // An assertion that checks if the vertex belongs to _any_ cluster
            virtual bool check_in_cluster() = 0;
        };
    }
}

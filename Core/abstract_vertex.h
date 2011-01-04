#pragma once
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract vertex, providing common functionality for both
        // PhysicalVertex and GraphicalVertex: it has position and
        // can be included into clusters.
        class AbstractVertex
        {
        private:
            // Number of clusters which include current vertex
            int including_clusters_num;
            int nearest_cluster_index;
        
        protected:
            bool check_in_cluster();

        public:
            static const int NOT_IN_A_CLUSTER = -2;

            AbstractVertex() : including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER) {}

            virtual const Math::Vector & get_pos() const = 0;

            // accessor to including_clusters_num
            void include_to_one_more_cluster() { ++including_clusters_num; }
            int get_including_clusters_num() const { return including_clusters_num; }

            // accessors to nearest_cluster_index
            bool set_nearest_cluster_index(int index);
            int get_nearest_cluster_index() const;
        };
    }
}

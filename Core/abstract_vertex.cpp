#include "Core/abstract_vertex.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Core
    {
        bool AbstractVertex::set_nearest_cluster_index(int index)
        {
            if(NOT_IN_A_CLUSTER != nearest_cluster_index)
            {
                Logger::error("in AbstractVertex::set_nearest_cluster_index: already set", __FILE__, __LINE__);
                return false;
            }
            if(index < 0)
            {
                Logger::error("in AbstractVertex::set_nearest_cluster_index: invalid index", __FILE__, __LINE__);
                return false;
            }

            nearest_cluster_index = index;
            return true;
        }

        int AbstractVertex::get_nearest_cluster_index() const
        {
            if(NOT_IN_A_CLUSTER == nearest_cluster_index)
            {
                Logger::error("in AbstractVertex::get_nearest_cluster_index: internal error: vertex doesn't belong to any cluster", __FILE__, __LINE__);
                return 0;
            }
            return nearest_cluster_index;
        }

        bool AbstractVertex::check_in_cluster()
        {
            if(0 == including_clusters_num)
            {
                Logger::error("internal error: AbstractVertex::check_in_cluster failed: vertex with incorrect zero value of including_clusters_num", __FILE__, __LINE__);
                return false;
            }
            return true;
        }
    }
}

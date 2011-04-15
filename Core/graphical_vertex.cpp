#include "Core/graphical_vertex.h"

namespace CrashAndSqueeze
{
    using Math::Real;
    using Math::Vector;
    using Math::VECTOR_SIZE;
    using Logging::Logger;

    namespace Core
    {
        namespace
        {
            void get_by_offset(const void *src_vertex, int offset, /*out*/ Vector & out_vector)
            {
                const VertexFloat *src_data = reinterpret_cast<const VertexFloat*>( add_to_pointer(src_vertex, offset) );
                VertexInfo::vertex_floats_to_vector(src_data, out_vector);
            }
        }
        
        GraphicalVertex::GraphicalVertex()
            : including_clusters_num(0)
        {
            pos = Vector::ZERO;
        }

        GraphicalVertex::GraphicalVertex(const VertexInfo &vertex_info, const void *src_vertex)
            : including_clusters_num(0)
        {
            get_by_offset(src_vertex, vertex_info.get_point_offset(0), pos);
        }

        const Vector & GraphicalVertex::get_pos() const
        {
            return pos;
        }
        
        void GraphicalVertex::include_to_one_more_cluster(int cluster_index)
        {
            if(VertexInfo::CLUSTER_INDICES_NUM == including_clusters_num)
            {
                Logger::error("in GraphicalVertex::include_to_one_more_cluster: cannot include to one more clusters, cluster indices limit exceeded", __FILE__, __LINE__);
                return;
            }
            cluster_indices[including_clusters_num] = static_cast<ClusterIndex>(cluster_index);
            ++including_clusters_num;
        }

        bool GraphicalVertex::check_in_cluster()
        {
            if(0 == including_clusters_num)
            {
                Logger::error("internal error: GraphicalVertex doesn't belong to any cluster", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        ClusterIndex GraphicalVertex::get_including_cluster_index(int index)
        {
            if(index >= VertexInfo::CLUSTER_INDICES_NUM)
            {
                Logger::error("in GraphicalVertex::get_including_cluster_index: index out of range", __FILE__, __LINE__);
                return 0;
            }
            return cluster_indices[index];
        }
    }
}

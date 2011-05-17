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
            points[0] = Vector::ZERO;
        }

        GraphicalVertex::GraphicalVertex(const VertexInfo &vertex_info, const void *src_vertex)
            : including_clusters_num(0)
        {
            points_num = vertex_info.get_points_num();
            vectors_num = vertex_info.get_vectors_num();

            for(int i = 0; i < points_num; ++i)
            {
                get_by_offset(src_vertex, vertex_info.get_point_offset(i), points[i]);
            }

            for(int i = 0; i < vectors_num; ++i)
            {
                get_by_offset(src_vertex, vertex_info.get_vector_offset(i), vectors[i]);
                vectors_orthogonality[i] = vertex_info.is_vector_orthogonal(i);
            }
        }

        bool GraphicalVertex::check_point_index(int index) const
        {
            if(index < 0 || index >= points_num)
            {
                Logger::error("in GraphicalVertex::check_point_index: invalid point index", __FILE__, __LINE__);
                return false;
            }
            return true;
        }
        
        bool GraphicalVertex::check_vector_index(int index) const
        {
            if(index < 0 || index >= vectors_num)
            {
                Logger::error("in GraphicalVertex::check_vector_index: invalid vector index", __FILE__, __LINE__);
                return false;
            }
            return true;
        }


        const Vector & GraphicalVertex::get_point(int index) const
        {
        #ifndef NDEBUG
            if(false == check_point_index(index))
                return Vector::ZERO;
            else
        #endif //ifndef NDEBUG
                return points[index];
        }

        const Vector & GraphicalVertex::get_vector(int index) const
        {
        #ifndef NDEBUG
            if(false == check_vector_index(index))
                return Vector::ZERO;
            else
        #endif //ifndef NDEBUG
                return vectors[index];
        }

        bool GraphicalVertex::is_vector_orthogonal(int index) const
        {
        #ifndef NDEBUG
            if(false == check_vector_index(index))
                return false;
            else
        #endif //ifndef NDEBUG
                return vectors_orthogonality[index];
        }

        const Vector & GraphicalVertex::get_pos() const
        {
            return get_point(0);
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

        ClusterIndex GraphicalVertex::get_including_cluster_index(int index) const
        {
        #ifndef NDEBUG
            if(index >= VertexInfo::CLUSTER_INDICES_NUM)
            {
                Logger::error("in GraphicalVertex::get_including_cluster_index: index out of range", __FILE__, __LINE__);
                return 0;
            }
        #endif //ifndef NDEBUG
            return cluster_indices[index];
        }
    }
}

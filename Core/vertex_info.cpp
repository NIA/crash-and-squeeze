#include "Core/vertex_info.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    
    namespace Core
    {
        void VertexInfo::set_vertex_size(int vertex_size)
        {
            if( vertex_size <= 0 )
            {
                Logger::error("vertex_size given for VertexInfo is less than or equal to zero", __FILE__, __LINE__);
                this->vertex_size = 0;
            }
            else
            {
                this->vertex_size = vertex_size;
            }
        }

        int VertexInfo::get_max_valid_offset() const
        {
            return vertex_size - Math::VECTOR_SIZE*sizeof(VertexFloat);
        }

        int VertexInfo::get_max_valid_cluster_indices_offset() const
        {
            return vertex_size - CLUSTER_INDICES_NUM*sizeof(ClusterIndex);
        }

        void VertexInfo::add_offset(Collections::Array<int> & arr, int offset)
        {
            if( MAX_COMPONENT_NUM == arr.size() )
            {
                Logger::error("in VertexInfo::add_offset: too much point/vector offsets to VertexInfo, maximum is VertexInfo::MAX_COMPONENT_NUM", __FILE__, __LINE__);
            }
            else
            {
                if( offset < 0 || offset > get_max_valid_offset()  )
                {
                    Logger::error("in VertexInfo::add_offset: invalid offset: it should be >= 0 and leave enough space for three VertexFloat values", __FILE__, __LINE__);
                }
                else
                {
                    arr.push_back(offset);
                }
            }
        }

        void VertexInfo::set_cluster_indices_offset(int offset)
        {
            if( offset < 0 || offset > get_max_valid_cluster_indices_offset()  )
            {
                Logger::error("in VertexInfo::set_cluster_indices_offset: invalid offset: it should be >= 0 and leave enough space for CLUSTER_INDICES_NUM ClusterIndex values", __FILE__, __LINE__);
            }
            else
            {
                cluster_indices_offset = offset;
            }
        }

        void VertexInfo::add_point(int offset)
        {
            add_offset(points_offsets, offset);
        }

        void VertexInfo::add_vector(int offset, bool orthogonal)
        {
            add_offset(vectors_offsets, offset);
            vectors_orthogonality.push_back(orthogonal);
        }

        int VertexInfo::get_point_offset(int index) const
        {
            return points_offsets[index];
        }

        int VertexInfo::get_vector_offset(int index) const
        {
            return vectors_offsets[index];
        }

        bool VertexInfo::is_vector_orthogonal(int index) const
        {
            return vectors_orthogonality[index];
        }
    }
}

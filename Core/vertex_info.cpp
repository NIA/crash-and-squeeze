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
                    Logger::error("in VertexInfo::add_offset: invalid offset: it should be >= 0 and leave enough space for three VertexFloat structures", __FILE__, __LINE__);
                }
                else
                {
                    arr.push_back(offset);
                }
            }
        }

        void VertexInfo::add_point(int offset)
        {
            add_offset(points_offsets, offset);
        }

        void VertexInfo::add_vector(int offset)
        {
            add_offset(vectors_offsets, offset);
        }

        int VertexInfo::get_point_offset(int index) const
        {
            return points_offsets[index];
        }

        int VertexInfo::get_vector_offset(int index) const
        {
            return vectors_offsets[index];
        }
    }
}

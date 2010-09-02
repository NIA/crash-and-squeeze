#include "Core/vertex_info.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    using namespace Math;
    using Logging::logger;
    
    namespace Core
    {
        namespace
        {
            // -- helpers for accessing variable-length array --

            void add_item(int array[], int item, int & items_number, int max_item_number, int min_item, int max_item,
                          const char *too_much_error_message, const char *invalid_item_error_message)
            {
                if( max_item_number == items_number )
                {
                    Logging::logger.error(too_much_error_message, __FILE__, __LINE__);
                }
                else
                {
                    if( item < min_item || item > max_item  )
                    {
                        Logging::logger.error(invalid_item_error_message, __FILE__, __LINE__);
                    }
                    else
                    {
                        array[items_number++] = item;
                    }
                }
            }

            int get_item(const int array[], int index, int items_number, const char *error_message)
            {
                if( index > items_number - 1 )
                {
                    Logging::logger.error(error_message, __FILE__, __LINE__);
                    return 0;
                }
                else
                {
                    return array[index];
                }
            }
        }

        void VertexInfo::set_vertex_size(int size)
        {
            if( size <= 0 )
            {
                Logging::logger.error("vertex_size given for VertexInfo is less than or equal to zero", __FILE__, __LINE__);
                vertex_size = 0;
            }
            else
            {
                vertex_size = size;
            }
        }

        int VertexInfo::get_max_valid_offset() const
        {
            return vertex_size - Math::VECTOR_SIZE*sizeof(VertexFloat);
        }

        void VertexInfo::add_point(int offset)
        {
            add_item(points_offsets, offset, points_num, MAX_COMPONENT_NUM, 0, get_max_valid_offset(),
                     "trying to add too much points to VertexInfo, maximum is VertexInfo::MAX_COMPONENT_NUM",
                     "invalid point offset given to VertexInfo::add_point: it should be >= 0 and leave enough space for three `VertexFloat`s");
        }

        void VertexInfo::add_vector(int offset)
        {
            add_item(vectors_offsets, offset, vectors_num, MAX_COMPONENT_NUM, 0, get_max_valid_offset(),
                     "trying to add too much vectors to VertexInfo, maximum is VertexInfo::MAX_COMPONENT_NUM",
                     "invalid vector offset given to VertexInfo::add_vector: it should be >= 0 and leave enough space for three `VertexFloat`s");
        }

        int VertexInfo::get_point_offset(int index) const
        {
            return get_item(points_offsets, index, points_num,
                            "index out of range in VertexInfo::get_point_offset");
        }

        int VertexInfo::get_vector_offset(int index) const
        {
            return get_item(vectors_offsets, index, vectors_num,
                            "index out of range in VertexInfo::get_vector_offset");
        }
    }
}
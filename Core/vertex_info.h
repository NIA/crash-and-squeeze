#pragma once
#include "Core/core.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {

        class VertexInfo
        {
        public:
            // max number of points and vectors
            // associated with each vertex
            static const int MAX_COMPONENT_NUM = 10;
        
        private:
            // sizeof vertex structure
            int vertex_size;
            
            // Determines where points associated with each vertex (its position etc.)
            // are placed in vertex structure by specifying their offsets in structure.
            // The first element of array is treated as a position of vertex
            int points_offsets[MAX_COMPONENT_NUM];

            // Number of points associated with each vertex
            // (i.e. number of significant elements in points_offsets).
            // Minimum is 1 (position only), maximum is MAX_COMPONENT_NUM
            int points_num;
            
            // Determines where vectors associated with each vertex (surface normal etc.)
            // are placed in vertex structure by specifying their offsets in structure.
            int vectors_offsets[MAX_COMPONENT_NUM];
            
            // Number of vectors associated with each vertex
            // (i.e. number of significant elements in vectors_offsets)
            // Minimum is 0, maximum is MAX_COMPONENT_NUM
            int vectors_num;

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

            int get_item(const int array[], int index, int items_number, const char *error_message) const
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

            void set_vertex_size(int size)
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

            int get_max_valid_offset() const
            {
                return vertex_size - Math::VECTOR_SIZE*sizeof(VertexFloat);
            }

        public:
            // There are constructors for most common cases.
            // For other cases use one of them, and then add more
            // with VertexInfo::add_point and VertexInfo::add_vector

            // constructor for one point and no vectors associated with vertex
            VertexInfo(int vertex_size, int position_offset)
                : points_num(0), vectors_num(0)
            {
                set_vertex_size(vertex_size);
                add_point(position_offset);
            }
            
            // constructor for one point and one vector associated with vertex
            VertexInfo(int vertex_size, int position_offset, int vector_offset)
                : points_num(0), vectors_num(0)
            {
                set_vertex_size(vertex_size);
                add_point(position_offset);
                add_vector(vector_offset);
            }

            void add_point(int offset)
            {
                add_item(points_offsets, offset, points_num, MAX_COMPONENT_NUM, 0, get_max_valid_offset(),
                         "trying to add too much points to VertexInfo, maximum is VertexInfo::MAX_COMPONENT_NUM",
                         "invalid point offset given to VertexInfo::add_point: it should be >= 0 and leave enough space for three `VertexFloat`s");
            }

            void add_vector(int offset)
            {
                add_item(vectors_offsets, offset, vectors_num, MAX_COMPONENT_NUM, 0, get_max_valid_offset(),
                         "trying to add too much vectors to VertexInfo, maximum is VertexInfo::MAX_COMPONENT_NUM",
                         "invalid vector offset given to VertexInfo::add_vector: it should be >= 0 and leave enough space for three `VertexFloat`s");
            }

            int get_vertex_size() const { return vertex_size; }
            int get_points_num() const { return points_num; }
            int get_vectors_num() const { return vectors_num; }
            int get_point_offset(int index) const
            {
                return get_item(points_offsets, index, points_num, "index out of range in VertexInfo::get_point_offset");
            }
            int get_vector_offset(int index) const
            {
                return get_item(vectors_offsets, index, vectors_num, "index out of range in VertexInfo::get_vector_offset");
            }
        
        private:
            VertexInfo(); // no default constructor
        };
    }
}
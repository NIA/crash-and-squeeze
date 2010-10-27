#pragma once
#include "Core/core.h"
#include "Collections/array.h"

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
            Collections::Array<int> points_offsets;
            
            // Determines where vectors associated with each vertex (surface normal etc.)
            // are placed in vertex structure by specifying their offsets in structure.
            Collections::Array<int> vectors_offsets;

            int get_max_valid_offset() const;

            // Common initialization for all constructors
            void set_vertex_size(int vertex_size);
            
            // Common code for adding offset either to points_offsets or vectors_offsets
            void add_offset(Collections::Array<int> & arr, int offset);

        public:
            // There are constructors for most common cases.
            // For other cases use one of them, and then add more
            // with VertexInfo::add_point and VertexInfo::add_vector

            // constructor for one point and no vectors associated with vertex
            VertexInfo(int vertex_size, int position_offset)
                : points_offsets(MAX_COMPONENT_NUM), vectors_offsets(MAX_COMPONENT_NUM)
            {
                set_vertex_size(vertex_size);
                add_point(position_offset);
            }
            
            // constructor for one point and one vector associated with vertex
            VertexInfo(int vertex_size, int position_offset, int vector_offset)
                : points_offsets(MAX_COMPONENT_NUM), vectors_offsets(MAX_COMPONENT_NUM)
            {
                set_vertex_size(vertex_size);
                add_point(position_offset);
                add_vector(vector_offset);
            }

            void add_point(int offset);

            void add_vector(int offset);

            int get_vertex_size() const { return vertex_size; }
            
            int get_points_num() const { return points_offsets.size(); }
            int get_point_offset(int index) const;
            
            int get_vectors_num() const { return vectors_offsets.size(); }
            int get_vector_offset(int index) const;
        
        private:
            VertexInfo(); // no default constructor
            // No copying!
            VertexInfo(const VertexInfo &);
            VertexInfo & operator=(const VertexInfo &);
        };
    }
}

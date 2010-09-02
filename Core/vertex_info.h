#pragma once
#include "Core/core.h"

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

            void set_vertex_size(int size);
            int get_max_valid_offset() const;

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

            void add_point(int offset);

            void add_vector(int offset);

            int get_vertex_size() const { return vertex_size; }
            
            int get_points_num() const { return points_num; }
            int get_point_offset(int index) const;
            
            int get_vectors_num() const { return vectors_num; }
            int get_vector_offset(int index) const;
        
        private:
            VertexInfo(); // no default constructor
        };
    }
}
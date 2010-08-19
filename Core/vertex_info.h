#pragma once
#include "Core/core.h"

namespace CrashAndSqueeze
{
    namespace Core
    {

        struct VertexInfo
        {
            // max number of points and vectors
            // associated with each vertex
            static const int MAX_COMPONENT_NUM = 10;

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

            // constructor for one point and no vectors associated with vertex
            VertexInfo(int vertex_size, int position_offset)
                : vertex_size(vertex_size), points_num(1), vectors_num(0)
            {
                points_offsets[0] = position_offset;
            }
            
            // constructor for one point and one vector associated with vertex
            VertexInfo(int vertex_size, int position_offset, int vector_offset)
                : vertex_size(vertex_size), points_num(1), vectors_num(1)
            {
                points_offsets[0] = position_offset;
                vectors_offsets[0] = vector_offset;
            }
        };
    }
}
#pragma once
#include "Core/core.h"
#include "Math/vector.h"
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
            static const int CLUSTER_INDICES_NUM = 8;
        
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
            
            // Only normal and tangent to the surface vectors can be transformed properly,
            // and the way this two kinds are transformed is different.
            // So this determines whether the vector is normal (true) or tangent (false).
            Collections::Array<bool> vectors_orthogonality;

            int cluster_indices_offset;

            int get_max_valid_offset() const;
            int get_max_valid_cluster_indices_offset() const;

            // Common initialization for all constructors
            void set_vertex_size(int vertex_size);
            void set_cluster_indices_offset(int offset);
            
            // Common code for adding offset either to points_offsets or vectors_offsets
            void add_offset(Collections::Array<int> & arr, int offset);

        public:
            // There are constructors for most common cases.
            // For other cases use one of them, and then add more
            // with VertexInfo::add_point and VertexInfo::add_vector

            // constructor for one point and no vectors associated with vertex
            VertexInfo(int vertex_size, int position_offset, int cluster_indices_offset)
                : points_offsets(MAX_COMPONENT_NUM), vectors_offsets(MAX_COMPONENT_NUM)
            {
                set_vertex_size(vertex_size);
                set_cluster_indices_offset(cluster_indices_offset);
                add_point(position_offset);
            }
            
            // constructor for one point and one vector associated with vertex
            VertexInfo(int vertex_size, int position_offset, int vector_offset, bool is_vector_orthogonal, int cluster_indices_offset)
                : points_offsets(MAX_COMPONENT_NUM), vectors_offsets(MAX_COMPONENT_NUM)
            {
                set_vertex_size(vertex_size);
                set_cluster_indices_offset(cluster_indices_offset);
                add_point(position_offset);
                add_vector(vector_offset, is_vector_orthogonal);
            }

            void add_point(int offset);

            void add_vector(int offset, bool orthogonal);

            int get_vertex_size() const { return vertex_size; }
            int get_cluster_indices_offset() const { return cluster_indices_offset; }
            
            int get_points_num() const { return points_offsets.size(); }
            int get_point_offset(int index) const;
            
            int get_vectors_num() const { return vectors_offsets.size(); }
            int get_vector_offset(int index) const;
            bool is_vector_orthogonal(int index) const;

            static void vertex_floats_to_vector(const VertexFloat * vertex_floats, /*out*/ Math::Vector & out_vector)
            {
                for(int i = 0; i < Math::VECTOR_SIZE; ++i)
                {
                    out_vector[i] = static_cast<Math::Real>(vertex_floats[i]);
                }
            }
            static void vector_to_vertex_floats(const Math::Vector & vector, /*out*/ VertexFloat * out_vertex_floats)
            {
                for(int i = 0; i < Math::VECTOR_SIZE; ++i)
                {
                    out_vertex_floats[i] = static_cast<VertexFloat>(vector[i]);
                }
            }
        
        private:
            VertexInfo(); // no default constructor
            // No copying!
            VertexInfo(const VertexInfo &);
            VertexInfo & operator=(const VertexInfo &);
        };
    }
}

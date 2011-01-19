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
            : points_num(1), vectors_num(0)
        {
            points[0] = Vector::ZERO;
        }

        GraphicalVertex::GraphicalVertex(const VertexInfo &vertex_info, const void *src_vertex)
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

        void GraphicalVertex::set_point(int index, const Math::Vector & value)
        {
        #ifndef NDEBUG
            if(false == check_point_index(index))
                return;
        #endif //ifndef NDEBUG
            
            points[index] = value;
        }

        void GraphicalVertex::add_part_to_point(int index, const Math::Vector & addition)
        {
        #ifndef NDEBUG
            if(false == check_point_index(index) || false == check_in_cluster())
                return;
        #endif //ifndef NDEBUG

            points[index] += addition/get_including_clusters_num();
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

        void GraphicalVertex::set_vector(int index, const Math::Vector & value)
        {
        #ifndef NDEBUG
            if(false == check_vector_index(index))
                return;
        #endif //ifndef NDEBUG

            vectors[index] = value;
        }

        void GraphicalVertex::add_part_to_vector(int index, const Math::Vector & addition)
        {
        #ifndef NDEBUG
            if(false == check_vector_index(index) || false == check_in_cluster())
                return;
        #endif //ifndef NDEBUG

            vectors[index] += addition/get_including_clusters_num();
        }

        const Vector & GraphicalVertex::get_pos() const
        {
            return get_point(0);
        }
    }
}

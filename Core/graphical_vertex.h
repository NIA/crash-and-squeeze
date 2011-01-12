#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Core/abstract_vertex.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class GraphicalVertex : public AbstractVertex
        {
        private:
            Math::Vector points[VertexInfo::MAX_COMPONENT_NUM];
            int points_num;

            Math::Vector vectors[VertexInfo::MAX_COMPONENT_NUM];
            bool vectors_orthogonality[VertexInfo::MAX_COMPONENT_NUM];
            int vectors_num;

            bool check_point_index(int index) const;
            bool check_vector_index(int index) const;

        public:
            GraphicalVertex();
            GraphicalVertex(const VertexInfo &vertex_info, const void *src_vertex);

            int get_points_num() const { return points_num; }
            const Math::Vector & get_point(int index) const;
            void set_point(int index, const Math::Vector & value);
            void add_part_to_point(int index, const Math::Vector & addition);
            
            int get_vectors_num() const { return vectors_num; }
            const Math::Vector & get_vector(int index) const;
            bool is_vector_orthogonal(int index) const;
            void set_vector(int index, const Math::Vector & value);
            void add_part_to_vector(int index, const Math::Vector & addition);

            // Implement AbstractVertex
            virtual const Math::Vector & get_pos() const;
        };
    }
}

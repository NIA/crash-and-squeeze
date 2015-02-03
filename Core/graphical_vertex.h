#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Core/ivertex.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class GraphicalVertex : public IVertex
        {
        private:
            Math::Vector points[VertexInfo::MAX_COMPONENT_NUM];
            int points_num;

            Math::Vector vectors[VertexInfo::MAX_COMPONENT_NUM];
            bool vectors_orthogonality[VertexInfo::MAX_COMPONENT_NUM];
            int vectors_num;

            ClusterIndex cluster_indices[VertexInfo::CLUSTER_INDICES_NUM];
            // Weights for computing weighted average of positions from different clusters: to smooth borders between clusters
            Math::Real cluster_weights[VertexInfo::CLUSTER_INDICES_NUM];
            int including_clusters_num;

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

            // -- Implement IVertex --
            virtual const Math::Vector & get_pos() const;
            // Adds vertex to another cluster with given weight.
            virtual void include_to_one_more_cluster(int cluster_index, Math::Real weight = 1);
            // Returns total number of clusters this vertex belongs to
            virtual int get_including_clusters_num() const { return including_clusters_num; }
            // Returns the index of i'th cluster this vertex belongs to
            ClusterIndex get_including_cluster_index(int index) const;
            // An assertion that checks if the vertex belongs to _any_ cluster
            virtual bool check_in_cluster();
            // and add something special:

            // make sure all cluster weights sum up to 1
            void normalize_weights();
            Math::Real get_cluster_weight(int index) const;
        };
    }
}

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
            Math::Vector pos;
            ClusterIndex cluster_indices[VertexInfo::CLUSTER_INDICES_NUM];
            int including_clusters_num;

        public:
            GraphicalVertex();
            GraphicalVertex(const VertexInfo &vertex_info, const void *src_vertex);

            // Implement IVertex
            virtual const Math::Vector & get_pos() const;
            virtual void include_to_one_more_cluster(int cluster_index);
            virtual int get_including_clusters_num() const { return including_clusters_num; }
            virtual bool check_in_cluster();
            // and add something special
            ClusterIndex get_including_cluster_index(int index) const;
        };
    }
}

#include "Core/reactions.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;

    namespace Core
    {
        namespace
        {
            bool compare_by_cluster_pointer(const ClusterWithWeight &a, const ClusterWithWeight &b)
            {
                return a.cluster == b.cluster;
            }
        }

        bool ShapeDeformationReaction::link_with_model(const IModel &model)
        {
            if(linked)
            {
                Logger::error("in ShapeDeformationReaction::link_with_model: already linked");
                return false;
            }

            int vertices_num = shape_vertex_indices.size();
            for(int i = 0; i < vertices_num; ++i)
            {
                int vertex_index = shape_vertex_indices[i];
                int cluster_index = model.get_vertex(vertex_index).nearest_cluster_index;
                const Cluster * cluster = &model.get_cluster(cluster_index);
                
                // if there is a record for the cluster, increment its weight; otherwise add a new record
                ClusterWithWeight &cww = clusters_with_weights.find_or_add( ClusterWithWeight(cluster, 0),
                                                                            compare_by_cluster_pointer );
                cww.weight += 1.0/vertices_num;
            }

            clusters_with_weights.freeze();
            linked = true;
            return true;
        }

        void ShapeDeformationReaction::invoke_if_needed()
        {
            Real weighed_relative_deformation = 0;
            for(int i = 0; i < clusters_with_weights.size(); ++i)
            {
                Real relative_deformation = clusters_with_weights[i].cluster->get_relative_plastic_deformation();
                weighed_relative_deformation += relative_deformation * clusters_with_weights[i].weight;
            }

            if(weighed_relative_deformation > threshold)
            {
                invoke(weighed_relative_deformation);
            }
        }
    }
}

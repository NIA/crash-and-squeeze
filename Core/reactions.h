#pragma once
#include "Math/floating_point.h"
#include "Collections/array.h"
#include "Core/imodel.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        typedef Collections::Array<int> IndexArray;

        // An internal structure for linking shape of ShapeDeformationAction to clusters
        struct ClusterWithWeight
        {
            const Cluster *cluster;
            Math::Real weight;

            ClusterWithWeight() : cluster(0), weight(0) {}
            ClusterWithWeight(const Cluster * cluster, Math::Real weight) : cluster(cluster), weight(weight) {}
        };
        typedef Collections::Array<ClusterWithWeight> ClusterWithWeightArray;

        // An abstract reaction to shape deformation. To implement your own reaction,
        // inherit your class from this and override invoke(), then pass an instance
        // of your class to Model::add_shape_deformation_reaction.
        class ShapeDeformationReaction
        {
        private:
            const IndexArray &shape_vertex_indices;
            Math::Real threshold;

            bool linked;
            ClusterWithWeightArray clusters_with_weights;

        protected:
            const IndexArray &get_shape_vertex_indices() { return shape_vertex_indices; }
            Math::Real get_threshold() { return threshold; }

        public:
            // When creating an instance of the reaction, the shape have to be defined by indices of
            // vertices that form the shape, and the threshold of relative deformation have to be specified
            ShapeDeformationReaction(const IndexArray &shape_vertex_indices, Math::Real threshold)
                : shape_vertex_indices(shape_vertex_indices), threshold(threshold), linked(false)
            {}
            
            // A function called internally when the action is registered in Model
            bool link_with_model(const IModel &model);
            void invoke_if_needed();
            
            virtual void invoke(Math::Real value) = 0;
        };
        typedef Collections::Array<ShapeDeformationReaction *> ShapeDeformationReactions;
    }
}

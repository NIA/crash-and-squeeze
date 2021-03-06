#pragma once
#include "Math/floating_point.h"
#include "Core/core.h"
#include "Core/imodel.h"
#include "Core/regions.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class Enabled
        {
        private:
            bool enabled;
        public:
            Enabled() : enabled(true) {}
            virtual ~Enabled() {}

            bool is_enabled() { return enabled; }
            void enable() { enabled = true; }
            void disable() { enabled = false; }
        };

        typedef Enabled Reaction; // Currently Reaction is simply = Enabled, but in future we can make it a more sensible class

        // Reaction that is invoked based on entire state of IModel
        class ModelReaction : public Reaction {
        public:
            virtual void invoke_if_needed(const IModel &model) = 0;
        };

        // An abstract reaction to shape deformation: is invoked when at least one
        // vertex of the shape goes farther than `threshold_distance' from its initial position.
        // The shape is defined by an array of indices of vertices that form the shape.
        //
        // To implement your own reaction, inherit your class from this
        // and override invoke(), then pass an instance of your class
        // to Model::add_shape_deformation_reaction.
        //
        // If two or more vertices from shape exceed threshold distance, the index
        // of vertex with the greatest distance is passed to invoke().
        class ShapeDeformationReaction : public ModelReaction
        {
        private:
            const IndexArray & shape_vertex_indices;
            Math::Real threshold_distance;

        public:
            ShapeDeformationReaction(const IndexArray &shape_vertex_indices, Math::Real threshold_distance);
            
            void invoke_if_needed(const IModel &model);
            
            // override this to use
            virtual void invoke(int vertex_index, Math::Real distance) = 0;
            
            const IndexArray &get_shape_vertex_indices() const { return shape_vertex_indices; }
            Math::Real get_threshold_distance() const { return threshold_distance; }
        
        private:
            // no assignment operator due to a reference in class fields
            ShapeDeformationReaction & operator=(const ShapeDeformationReaction &);
        };

        
        // An abstract reaction to entering or leaving the region: is invoked when at least one
        // vertex of the shape enters the region (or when it leaves, if `reaction_on_entering' is false).
        // The shape is defined by an array of indices of vertices that form the shape.
        //
        // To implement your own reaction, inherit your class from this
        // and override invoke(), then pass an instance of your class
        // to Model::add_region_reaction.
        class RegionReaction : public ModelReaction
        {
        private:
            const IndexArray & shape_vertex_indices;
            const IRegion & region;
            bool reaction_on_entering;

        public:
            RegionReaction(const IndexArray & shape_vertex_indices, const IRegion & region, bool reaction_on_entering);

            void invoke_if_needed(const IModel &model);
            
            // override this to use
            virtual void invoke(int vertex_index) = 0;

            const IndexArray & get_shape_vertex_indices() const { return shape_vertex_indices; }
            const IRegion & get_region() const { return region; }
            bool reacts_on_entering() const { return reaction_on_entering; }
        
        private:
            // no assignment operator due to a reference in class fields
            RegionReaction & operator=(const RegionReaction &);
        };

        // An abstract reaction to hitting model: is invoked when at least one
        // vertex of the shape is affected by hit.
        // The shape is defined by an array of indices of vertices that form the shape.
        //
        // To implement your own reaction, inherit your class from this
        // and override invoke(), then pass an instance of your class
        // to Model::add_hit_reaction.
        class HitReaction : public Reaction
        {
        private:
            const IndexArray & shape_vertex_indices;
            Math::Real velocity_threshold;

        public:
            HitReaction(const IndexArray & shape_vertex_indices, Math::Real velocity_threshold);

            void invoke_if_needed(const IndexArray & hit_vertex_indices, const Math::Vector &hit_velocity);
            
            // override this to use
            virtual void invoke(int vertex_index, const Math::Vector &velocity) = 0;

            const IndexArray & get_shape_vertex_indices() const { return shape_vertex_indices; }
            Math::Real get_velocity_threshold() const { return velocity_threshold; }
        
        private:
            // no assignment operator due to a reference in class fields
            HitReaction & operator=(const HitReaction &);
        };

        // An abstract reaction to stretch: is invoked when the distance between two vertices
        // exceeds `threshold_distance'.
        //
        // To implement your own reaction, inherit your class from this
        // and override invoke(), then pass an instance of your class
        // to Model::add_stretch_reaction.
        // TODO: test this class
        class StretchReaction : public ModelReaction
        {
        private:
            // TODO: maybe not just two vector indices, but kind of array of pairs?
            int index1;
            int index2;
            Math::Real dist_threshold;

        public:
            StretchReaction(int index1, int index2, Math::Real dist_threshold)
                : index1(index1), index2(index2), dist_threshold(dist_threshold)
            {}
            
            void invoke_if_needed(const IModel &model);

            int get_vertex1_index() const { return index1; }
            int get_vertex2_index() const { return index2; }
            Math::Real get_dist_threshold() { return dist_threshold; }

            // override this to use
            virtual void invoke(Math::Real distance) = 0;
        };
    }
}

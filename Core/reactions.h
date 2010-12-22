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

            bool is_enabled() { return enabled; }
            void enable() { enabled = true; }
            void disable() { enabled = false; }
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
        class ShapeDeformationReaction : public Enabled
        {
        private:
            const IndexArray & shape_vertex_indices;
            Math::Real threshold_distance;

        public:
            ShapeDeformationReaction(const IndexArray &shape_vertex_indices, Math::Real threshold_distance)
                : shape_vertex_indices(shape_vertex_indices), threshold_distance(threshold_distance) {}
            
            void invoke_if_needed(const IModel &model);
            
            // override this to use
            virtual void invoke(int vertex_index, Math::Real distance) = 0;
            
            const IndexArray &get_shape_vertex_indices() const { return shape_vertex_indices; }
            Math::Real get_threshold_distance() const { return threshold_distance; }
        
        private:
            // no assigment operator due to a reference in class fields
            ShapeDeformationReaction & operator=(const ShapeDeformationReaction &);
        };

        
        // An abstract reaction to entering or leaving the region: is invoked when at least one
        // vertex of the shape enters the region (or when it leaves, if `reaction_on_entering' is false).
        // The shape is defined by an array of indices of vertices that form the shape.
        //
        // To implement your own reaction, inherit your class from this
        // and override invoke(), then pass an instance of your class
        // to Model::add_region_reaction.
        class RegionReaction : public Enabled
        {
        private:
            const IndexArray & shape_vertex_indices;
            const IRegion & region;
            bool reaction_on_entering;

        public:
            RegionReaction(const IndexArray & shape_vertex_indices, const IRegion & region, bool reaction_on_entering)
                : shape_vertex_indices(shape_vertex_indices), region(region), reaction_on_entering(reaction_on_entering) {}

            void invoke_if_needed(const IModel &model);
            
            // override this to use
            virtual void invoke(int vertex_index) = 0;

            const IndexArray & get_shape_vertex_indices() const { return shape_vertex_indices; }
            const IRegion & get_region() const { return region; }
            bool reacts_on_entering() const { return reaction_on_entering; }
        
        private:
            // no assigment operator due to a reference in class fields
            RegionReaction & operator=(const RegionReaction &);
        };

        // An abstract reaction to hitting model: is invoked when at least one
        // vertex of the shape is affected by hit.
        // The shape is defined by an array of indices of vertices that form the shape.
        //
        // To implement your own reaction, inherit your class from this
        // and override invoke(), then pass an instance of your class
        // to Model::add_hit_reaction.
        class HitReaction : public Enabled
        {
        private:
            const IndexArray & shape_vertex_indices;
            Math::Real velocity_threshold;

        public:
            HitReaction(const IndexArray & shape_vertex_indices, Math::Real velocity_threshold)
                : shape_vertex_indices(shape_vertex_indices), velocity_threshold(velocity_threshold) {}

            void invoke_if_needed(const IndexArray & hit_vertex_indices, const Math::Vector &hit_velocity);
            
            // override this to use
            virtual void invoke(int vertex_index, const Math::Vector &velocity) = 0;

            const IndexArray & get_shape_vertex_indices() const { return shape_vertex_indices; }
            Math::Real get_velocity_threshold() const { return velocity_threshold; }
        
        private:
            // no assigment operator due to a reference in class fields
            HitReaction & operator=(const HitReaction &);
        };
    }
}

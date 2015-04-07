#include "Core/reactions.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;

    namespace Core
    {

        ShapeDeformationReaction::ShapeDeformationReaction(const IndexArray &shape_vertex_indices, Math::Real threshold_distance)
            : shape_vertex_indices(shape_vertex_indices), threshold_distance(threshold_distance)
        {
            if ( 0 == shape_vertex_indices.size() )
            {
                Logger::warning("in ShapeDeformationReaction: creating reaction with empty shape, disabling", __FILE__, __LINE__);
                disable();
            }
        }

        void ShapeDeformationReaction::invoke_if_needed(const IModel &model)
        {
            if( ! is_enabled() )
                return;

            Real max_distance = 0;
            int max_distance_vertex_index = 0;
            for(int i = 0; i < shape_vertex_indices.size(); ++i)
            {
                int vertex_index = shape_vertex_indices[i];
                Real distance = Math::distance( model.get_vertex_equilibrium_pos(vertex_index),
                                                      model.get_vertex_initial_pos(vertex_index) );

                if(distance > max_distance)
                {
                    max_distance = distance;
                    max_distance_vertex_index = vertex_index;
                }
            }

            if( max_distance > threshold_distance )
            {
                invoke(max_distance_vertex_index, max_distance);
                return;
            }
        }

        RegionReaction::RegionReaction(const IndexArray & shape_vertex_indices, const IRegion & region, bool reaction_on_entering) : shape_vertex_indices(shape_vertex_indices), region(region), reaction_on_entering(reaction_on_entering)
        {
            if ( 0 == shape_vertex_indices.size() )
            {
                Logger::warning("in RegionReaction: creating reaction with empty shape, disabling", __FILE__, __LINE__);
                disable();
            }
        }

        void RegionReaction::invoke_if_needed(const IModel &model)
        {
            if( ! is_enabled() )
                return;

            for(int i = 0; i < shape_vertex_indices.size(); ++i)
            {
                int vertex_index = shape_vertex_indices[i];
                Math::Vector position = model.get_vertex_equilibrium_pos(vertex_index);
                
                // when reaction_on_entering is true:  invoke if region contains point, i.e. if contains == true == reaction_on_entering,
                // when reaction_on_entering is false: invoke if region doesn't contain point, i.e. if contains == false == reaction_on_entering.
                if( region.contains(position) == reaction_on_entering )
                {
                    invoke(vertex_index);
                    return;
                }
            }
        }

        HitReaction::HitReaction(const IndexArray & shape_vertex_indices, Math::Real velocity_threshold) : shape_vertex_indices(shape_vertex_indices), velocity_threshold(velocity_threshold)
        {
            if ( 0 == shape_vertex_indices.size() )
            {
                Logger::warning("in HitReaction: creating reaction with empty shape, disabling", __FILE__, __LINE__);
                disable();
            }
        }

        void HitReaction::invoke_if_needed(const IndexArray & hit_vertices,
                                           const Math::Vector &hit_velocity)
        {
            if( ! is_enabled() || hit_velocity.norm() < velocity_threshold )
                return;

            for(int i = 0; i < shape_vertex_indices.size(); ++i)
            {
                int vertex_index = shape_vertex_indices[i];

                if( hit_vertices.contains(vertex_index) )
                {
                    invoke(vertex_index, hit_velocity);
                    return;
                }
            }
        }

        void StretchReaction::invoke_if_needed(const IModel &model)
        {
            if( ! is_enabled() )
                return;

            Math::Vector v1 = model.get_vertex_equilibrium_pos(index1);
            Math::Vector v2 = model.get_vertex_equilibrium_pos(index2);
            Math::Real dist = Math::distance(v1, v2);
            if (dist > dist_threshold)
            {
                invoke(dist);
            }
        }
    }
}

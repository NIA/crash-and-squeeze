#include "Core/reactions.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;

    namespace Core
    {
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

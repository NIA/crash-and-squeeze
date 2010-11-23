#include "Core/reactions.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Core
    {
        void ShapeDeformationReaction::invoke_if_needed(const IModel &model)
        {
            for(int i = 0; i < shape_vertex_indices.size(); ++i)
            {
                int vertex_index = shape_vertex_indices[i];
                Math::Real distance = Math::distance( model.get_vertex_equilibrium_pos(vertex_index),
                                                      model.get_vertex_initial_pos(vertex_index) );

                if( distance > threshold_distance )
                {
                    invoke(vertex_index, distance);
                    return;
                }
            }
        }

        void RegionReaction::invoke_if_needed(const IModel &model)
        {
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
            if(hit_velocity.norm() < velocity_threshold)
                return;

            for(int i = 0; i < hit_vertices.size(); ++i)
            {
                int vertex_index = shape_vertex_indices[i];

                if( hit_vertices.contains(vertex_index) )
                {
                    invoke(vertex_index, hit_velocity);
                    return;
                }
            }
        }
    }
}

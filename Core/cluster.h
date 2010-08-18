#pragma once
#include "core.h"
#include "vector.h"
#include "matrix.h"
#include "physical_vertex.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        struct PhysicalVertexMappingInfo
        {
            // index in model's vertex array
            int vertex_index;

            // TODO: thread-safe cluster addition: Math::Vector velocity_additions[MAX_CLUSTERS_FOR_VERTEX]
            
            // initial offset of position of vertex from
            // cluster's center of mass
            Math::Vector initial_offset_position;
        };
        
        const Math::Real DEFAULT_GOAL_SPEED_CONSTANT = 1;

        class Cluster
        {
        private:
            /* -- constant (at run-time) fields -- */
            
            // array of vertices belonging to the cluster
            PhysicalVertexMappingInfo *vertices;
            
            // number of vertices in vertex_indices
            int vertices_num;
            // allocated space for vertices
            int allocated_vertices_num; 
            static const int INITIAL_ALLOCATED_VERTICES_NUM = 100;
            
            // initial center of mass of vertices
            Math::Vector initial_center_of_mass;

            Math::Real total_mass;

            // a constant, determining how fast points are pulled to
            // their position, i.e. how rigid the body is:
            // 0 means no constraint at all, 1 means absolutely rigid
            Math::Real goal_speed_constant;
            
            // TODO: const Math::Real linear_deformation_constant;

            /* -- variable (at run-time) fields -- */
            
            // center of mass of vertices
            Math::Vector center_of_mass;
            Math::Matrix rotation;
            Math::Matrix total_deformation;
            
            // TODO: Math::Matrix plasticity_state;
            
        public:
            Cluster(Math::Real goal_speed_constant = DEFAULT_GOAL_SPEED_CONSTANT);
            virtual ~Cluster();

            void add_vertex(int vertex_index, const PhysicalVertex &vertex);

            int get_vertices_num() const { return vertices_num; }
            int get_vertex_index(int index) const { /* TODO: errors */ return vertices[index].vertex_index; }
            Math::Vector get_initial_vertex_offset_position(int index) const
            {
                // TODO: errors
                return vertices[index].initial_offset_position;
            }
            Math::Vector get_initial_center_of_mass() const { return initial_center_of_mass; }
            Math::Real get_total_mass() const { return total_mass; }
            Math::Real get_goal_speed_constant() const { return goal_speed_constant; }
            Math::Vector get_center_of_mass() const { return center_of_mass; }
            Math::Matrix get_rotation() const { return rotation; }
            Math::Matrix get_total_deformation() const { return total_deformation; }
            // TODO: Math::Matrix get_plasticity_state() const { return plasticity_state; }
        private:
            // No copying!
            Cluster(const Cluster &);
            Cluster & operator=(const Cluster &);
        };
    }
}

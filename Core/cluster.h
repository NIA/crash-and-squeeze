#pragma once
#include "Core/core.h"
#include "Math/vector.h"
#include "Math/matrix.h"
#include "Core/physical_vertex.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        struct PhysicalVertexMappingInfo;
        
        // a constant, determining how fast points are pulled to
        // their position, i.e. how rigid the body is:
        // 0 means no constraint at all, 1 means absolutely rigid
        const Math::Real DEFAULT_GOAL_SPEED_CONSTANT = 1;
        
        // a constant, determining how rigid body is:
        // if it equals `b`, then optimal deformation for goal positions
        // is calculated as (1 - b)*A + b*R, where R is optimal rotation
        // and A is optimal linear transformation.
        // Thus 0 means freely (but only linearly) deformable body,
        // 1 means absolutely rigid
        const Math::Real DEFAULT_LINEAR_ELASTICITY_CONSTANT = 1;
        
        // a constant, determining how much energy is lost:
        // 0 - approx. no loss, 1 - maximum damping, no repulse
        // (WARNING: setting too close to 1 may cause collapse of model,
        //  but setting too close to 0 may cause instability)
        const Math::Real DEFAULT_DAMPING_CONSTANT = 0.6;

        const Math::Real DEFAULT_YIELD_CONSTANT = 0.15; //!!!
        const Math::Real DEFAULT_CREEP_CONSTANT = 50;
        const Math::Real DEFAULT_MAX_DEFORMATION_CONSTANT = 2;

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

            // a constant, determining how rigid body is:
            // if it equals `b`, then optimal deformation for goal positions
            // is calculated as (1 - b)*A + b*R, where R is optimal rotation
            // and A is optimal linear transformation.
            // Thus 0 means freely (but only linearly) deformable body,
            // 1 means absolutely rigid
            Math::Real linear_elasticity_constant;

            // a constant, determining how much energy is lost:
            // 0 - approx. no loss, 1 - maximum damping, no repulse
            Math::Real damping_constant;

            // plasticity parameter: a treshold of strain, after
            // which deformation becomes non-reversible
            Math::Real yield_constant;
            
            // plasticity paramter: a coefficient determining how fast
            // plasticity_state will be changed on large deformation
            Math::Real creep_constant;

            /* -- variable (at run-time) fields -- */
            /* ( just stored here, calculated elsewhere )- */
            
            // center of mass of vertices
            Math::Vector center_of_mass;
            Math::Matrix rotation;
            Math::Matrix total_deformation;
            Math::Matrix plasticity_state;
            
            bool check_vertex_index(int index, const char *error_message) const;

        public:
            Cluster();
            virtual ~Cluster();

            void add_vertex(int vertex_index, PhysicalVertex &vertex);

            int get_vertices_num() const { return vertices_num; }

            int get_vertex_index(int index) const;

            // returns offset of vector position in equilibrium state
            // taking into account plasticity_state
            const Math::Vector get_initial_vertex_offset_position(int index) const;

            const Math::Vector & get_initial_center_of_mass() const { return initial_center_of_mass; }
            
            Math::Real get_total_mass() const { return total_mass; }
            
            Math::Real get_goal_speed_constant() const { return goal_speed_constant; }
            Math::Real get_linear_elasticity_constant() const { return linear_elasticity_constant; }
            Math::Real get_damping_constant() const { return damping_constant; }
            Math::Real get_yield_constant() const { return yield_constant; }
            Math::Real get_creep_constant() const { return creep_constant; }
            
            const Math::Vector & get_center_of_mass() const { return center_of_mass; }
            void set_center_of_mass(Math::Vector point) { center_of_mass = point; }
            
            const Math::Matrix & get_rotation() const { return rotation; }
            void set_rotation(const Math::Matrix & matrix) { rotation = matrix; }
            
            const Math::Matrix & get_total_deformation() const { return total_deformation; }
            void set_total_deformation(const Math::Matrix &matrix)
            {
                total_deformation = matrix;
            }
            const Math::Matrix & get_plasticity_state() const { return plasticity_state; }
            void set_plasticity_state(const Math::Matrix &matrix)
            {
                plasticity_state = matrix;
            }
        private:
            // No copying!
            Cluster(const Cluster &);
            Cluster & operator=(const Cluster &);
        };
    }
}

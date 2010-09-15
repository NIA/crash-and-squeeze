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
        
        // a constant, determining how much deformation velocities are damped:
        // 0 - no damping of vibrations, 1 - maximum damping, rigid body
        const Math::Real DEFAULT_DAMPING_CONSTANT = 0.7;

        const Math::Real DEFAULT_YIELD_CONSTANT = 0.1; //!!!
        const Math::Real DEFAULT_CREEP_CONSTANT = 40;
        const Math::Real DEFAULT_MAX_DEFORMATION_CONSTANT = 3;

        class Cluster
        {
        private:
            // -- constant (at run-time) fields --
            
            // array of vertices belonging to the cluster
            PhysicalVertexMappingInfo *vertices;
            
            // number of vertices in vertex_indices
            int vertices_num;
            // allocated space for vertices
            int allocated_vertices_num; 

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

            // -- variable (at run-time) fields --

            // center of mass of vertices
            Math::Vector center_of_mass;
            // plastic deformation applied to initial shape
            Math::Matrix plasticity_state;
            // optimal linear transformation satisfying shape matching (A)
            Math::Matrix linear_transformation;
            // first (asymmetric) term of linear_transformation (Apq)
            Math::Matrix asymmetric_term;
            // second (symmetric) term of linear_transformation (Aqq)
            Math::Matrix symmetric_term;
            // rotational part of linear_transformation (R)
            Math::Matrix rotation;
            // symmetric part of linear_transformation (S)
            Math::Matrix scale;
            // total deformation used in goal positions calculation (interpolated from R and A)
            Math::Matrix total_deformation;
            
            // -- access helpers --
            bool check_vertex_index(int index, const char *error_message) const;
            
            // -- shape matching steps --
            
            // re-computes center of mass
            void update_center_of_mass();
            
            // computes asymmetric_term and symmetric_term
            void compute_shape_matching_terms();
            
            // computes required transformations
            void compute_transformations();
            
            // computes linear_transformation
            void compute_linear_transformation();
            
            // computes goal positions and applies corrections to velocities of vertices
            void apply_goal_positions(Math::Real dt);
            
            // updates plasticity_state due to deformation applied
            void update_plasticity_state(Math::Real dt);
        
        public:
            Cluster();
            virtual ~Cluster();

            void add_vertex(PhysicalVertex &vertex);

            // -- methods --
            void match_shape(Math::Real dt);

            // -- getters/setters --
            
            int get_vertices_num() const { return vertices_num; }

            PhysicalVertex & get_vertex(int index);
            const PhysicalVertex & get_vertex(int index) const;

            // returns equilibrium position of vertex
            // (measured off the center of mass of the cluster)
            // taking into account plasticity_state
            const Math::Vector get_equilibrium_position(int index) const;

            const Math::Vector & get_initial_center_of_mass() const { return initial_center_of_mass; }
            
            Math::Real get_total_mass() const { return total_mass; }
            
            Math::Real get_goal_speed_constant() const { return goal_speed_constant; }
            Math::Real get_linear_elasticity_constant() const { return linear_elasticity_constant; }
            Math::Real get_damping_constant() const { return damping_constant; }
            Math::Real get_yield_constant() const { return yield_constant; }
            Math::Real get_creep_constant() const { return creep_constant; }
            
            const Math::Vector & get_center_of_mass() const { return center_of_mass; }
            const Math::Matrix & get_rotation() const { return rotation; }
            const Math::Matrix & get_total_deformation() const { return total_deformation; }
            const Math::Matrix & get_plasticity_state() const { return plasticity_state; }
            
            static const int INITIAL_ALLOCATED_VERTICES_NUM = 100;
        private:
            // No copying!
            Cluster(const Cluster &);
            Cluster & operator=(const Cluster &);
        };
    }
}

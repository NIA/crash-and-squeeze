#pragma once
#include "Core/core.h"
#include "Core/physical_vertex.h"
#include "Math/vector.h"
#include "Math/matrix.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An internal struct defining a membership of vertex in cluster
        struct PhysicalVertexMappingInfo
        {
            // index in model's vertex array
            PhysicalVertex *vertex;

            // TODO: thread-safe cluster addition: Math::Vector velocity_additions[MAX_CLUSTERS_FOR_VERTEX]
            
            // initial position of vertex measured off
            // the cluster's center of mass
            Math::Vector initial_offset_position;
            // position in deformed shape (plasticity_state*initial_offset_position)
            Math::Vector equilibrium_offset_position;
        };

        class Cluster
        {
        private:
            // -- constant (at run-time) fields --
            
            // array of vertices belonging to the cluster
            Collections::Array<PhysicalVertexMappingInfo> vertex_infos;

            // initial center of mass of vertices
            Math::Vector initial_center_of_mass;

            Math::Real total_mass;

            bool initial_characteristics_computed;
            bool valid;

            // pos and size define the space occupied by the cluster
            Math::Vector pos;
            Math::Vector size;
            
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

            // plasticity parameter: a threshold of strain, after
            // which deformation becomes non-reversible
            Math::Real yield_constant;
            
            // plasticity paramter: a coefficient determining how fast
            // plasticity_state will be changed on large deformation
            Math::Real creep_constant;

            // plasticity paramter: a threshold of maximum allowed strain
            Math::Real max_deformation_constant;

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
            bool check_initial_characteristics() const;
            
            // -- shape matching steps --
            
            // re-computes center of mass
            void update_center_of_mass();
            
            // computes required transformations
            void compute_transformations();
            
            // computes linear_transformation
            void compute_linear_transformation();
            
            // computes asymmetric_term (each step)
            void compute_asymmetric_term();
            
            // computes symmetric_term (only after plasticity state changed)
            void compute_symmetric_term();
            
            // computes goal positions and applies corrections to velocities of vertices
            void apply_goal_positions(Math::Real dt);
            
            // updates plasticity_state due to deformation applied
            void update_plasticity_state(Math::Real dt);
            
            // is called after updating plasticity_state
            void update_equilibrium_positions();

            // is called after updating plasticity_state
            void update_equilibrium_positions(Math::Real dt);
        
        public:
            Cluster();
            virtual ~Cluster();
            
            // -- methods --

            void add_vertex(PhysicalVertex &vertex);

            // this must be called after last vertex is added
            void compute_initial_characteristics();

            bool is_valid() const { return valid; }

            void match_shape(Math::Real dt);

            // -- getters/setters --

            int get_vertices_num() const { return vertex_infos.size(); }

            PhysicalVertex & get_vertex(int index);
            const PhysicalVertex & get_vertex(int index) const;

            const Math::Vector & get_initial_center_of_mass() const;

            // returns equilibrium position of vertex
            // (measured off the initial center of mass of the cluster)
            // taking into account plasticity_state
            const Math::Vector & get_equilibrium_position(int index) const;

            Math::Real get_total_mass() const { return total_mass; }

            Math::Real get_goal_speed_constant() const { return goal_speed_constant; }
            Math::Real get_linear_elasticity_constant() const { return linear_elasticity_constant; }
            Math::Real get_yield_constant() const { return yield_constant; }
            Math::Real get_creep_constant() const { return creep_constant; }

            const Math::Vector & get_center_of_mass() const { return center_of_mass; }
            const Math::Matrix & get_rotation() const { return rotation; }
            const Math::Matrix & get_total_deformation() const { return total_deformation; }
            const Math::Matrix & get_plasticity_state() const { return plasticity_state; }
            Math::Real get_relative_plastic_deformation() const;

            static const int INITIAL_ALLOCATED_VERTICES_NUM = 100;

            static const Math::Real DEFAULT_GOAL_SPEED_CONSTANT;
            static const Math::Real DEFAULT_LINEAR_ELASTICITY_CONSTANT;
            static const Math::Real DEFAULT_YIELD_CONSTANT;
            static const Math::Real DEFAULT_CREEP_CONSTANT;
            static const Math::Real DEFAULT_MAX_DEFORMATION_CONSTANT;
        private:
            // No copying!
            Cluster(const Cluster &);
            Cluster & operator=(const Cluster &);
        };
    }
}

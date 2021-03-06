#pragma once
#include "Core/core.h"
#include "Core/physical_vertex.h"
#include "Core/graphical_vertex.h"
#include "Core/simulation_params.h"
#include "Math/vector.h"
#include "Math/matrix.h"
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
#include "Math/quadratic.h"
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An internal struct defining a membership of physical vertex in cluster
        struct PhysicalVertexMappingInfo
        {
            PhysicalVertex *vertex;

            // initial position of vertex measured off
            // the cluster's center of mass
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            Math::TriVector initial_offset_pos;
#else
            Math::Vector initial_offset_pos;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            // position in deformed shape (plasticity_state*initial_offset_position),
            // used for computing symmetric and asymmetric terms of A (Apq and Aqq)
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            Math::TriVector equilibrium_offset_pos;
#else
            Math::Vector    equilibrium_offset_pos;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            // position in model coordinates (center_of_mass + rotation*equilibrium_offset_pos),
            // used to update PhysicalVertex's equilibrium_pos (which, in turn, is used mainly for reactions)
            Math::Vector equilibrium_pos;
            // index for stroring velocity_addition in PhysicalVertex independently of other classes
            int addition_index;

            void setup_initial_values(const Math::Vector & center_of_mass);
        };
        
        // An internal struct defining a membership of graphical vertex in cluster
        struct GraphicalVertexMappingInfo
        {
            GraphicalVertex *vertex;

            // initial state of graphical vertex, with all points re-computed as
            // offsets from the center of mass
            GraphicalVertex initial_offset_state;
            // state after plastic deformation: plasticity_state*initial_offset_state
            GraphicalVertex deformed_offset_state;
            // previous state, used to find difference between it and current state
            GraphicalVertex previous_state;

            void setup_initial_values(const Math::Vector & center_of_mass);
        };

        class Cluster
        {
        private:
            // -- constant (at run-time) fields --
            
            Collections::Array<PhysicalVertexMappingInfo> physical_vertex_infos;
            
            Collections::Array<GraphicalVertexMappingInfo> graphical_vertex_infos;

            Math::Real total_mass;
            
            // center of mass of vertices in initial state (before deformations)
            Math::Vector initial_center_of_mass;

            bool initial_characteristics_computed;
            bool valid;

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
            
            // plasticity parameter: a coefficient determining how fast
            // plasticity_state will be changed on large deformation
            Math::Real creep_constant;

#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            // another plasticity paramter: a coefficient determining how fast
            // quadratic part of plasticity_state will be changed on large deformation
            Math::Real qx_creep_constant;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED

            // plasticity parameter: a threshold of maximum allowed strain
            Math::Real max_deformation_constant;

            // -- variable (at run-time) fields --

            // center of mass of vertices
            Math::Vector center_of_mass;
            // plastic deformation applied to initial shape
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            Math::TriMatrix plasticity_state;
#else
            Math::Matrix plasticity_state;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            // plasticity_state.inverted().transposed()
            Math::Matrix plasticity_state_inv_trans;
            // measure of plasticity_state
            Math::Real plastic_deformation_measure;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            // optimal quadratic transformation satisfying shape matching (A~ = [A Q M])
            Math::TriMatrix optimal_transformation;
            // first (asymmetric) term of optimal_transformation (Apq~)
            Math::TriMatrix asymmetric_term;
            // second (symmetric) term of optimal_transformation (Aqq~)
            Math::NineMatrix symmetric_term;
#else
            // optimal linear transformation satisfying shape matching (A)
            Math::Matrix    optimal_transformation;
            // first (asymmetric) term of optimal_transformation (Apq)
            Math::Matrix    asymmetric_term;
            // second (symmetric) term of optimal_transformation (Aqq)
            Math::Matrix     symmetric_term;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            // rotational part of optimal_transformation (R)
            Math::Matrix rotation;
            // symmetric part of optimal_transformation (S)
            Math::Matrix scale;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            // total deformation used in goal positions calculation (interpolated from R and A~)
            Math::TriMatrix total_deformation;
            // position transformation for graphical vertices
            Math::TriMatrix graphical_pos_transform;
#else
            // total deformation used in goal positions calculation (interpolated from R and A)
            Math::Matrix total_deformation;
            // position transformation for graphical vertices
            Math::Matrix graphical_pos_transform;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            // normal transformation for graphical vertices (graphical_pos_transform inverted and transposed)
            Math::Matrix graphical_nrm_transform;
            
            // -- access helpers --
            bool check_initial_characteristics() const;
            
            // -- shape matching steps --
            
            // re-computes center of mass
            void update_center_of_mass();
            
            // computes required transformations
            void compute_transformations();
            
            // computes optimal_transformation
            void compute_optimal_transformation();
            
            // computes asymmetric_term (each step)
            void compute_asymmetric_term();
            
            // computes symmetric_term (only after plasticity state changed)
            void compute_symmetric_term();
            
            // computes goal positions and applies corrections to velocities of vertices
            void apply_goal_positions(Math::Real dt);
            
            // updates plasticity_state due to deformation applied
            void update_plasticity_state(Math::Real dt);
            
            // updates equilibrium position offsets (if plasticity_state changed) and equilibrium positions
            void update_equilibrium_positions(bool plasticity_state_changed);

            // updates graphical_pos_transform & graphical_nrm_transform
            void update_graphical_transformations();
        
        public:
            Cluster();
            virtual ~Cluster();
            
            // -- methods --

            void add_physical_vertex(PhysicalVertex &vertex);
            void add_graphical_vertex(GraphicalVertex &vertex);

            // this must be called after the last vertex (physical or graphical) is added
            void compute_initial_characteristics();

            bool is_valid() const { return valid; }

            void match_shape(Math::Real dt);

            // -- getters/setters --

            // Sets current simulation params given by corresponding fields of `params`.
            void set_simulation_params(const SimulationParams & params);
            
            // Gets current simulation params into `params`. Only cluster's params are written, other fields are untouched.
            void get_simulation_params(SimulationParams /*out*/ & params) const;

            int get_physical_vertices_num() const { return physical_vertex_infos.size(); }
            int get_graphical_vertices_num() const { return graphical_vertex_infos.size(); }

            PhysicalVertex & get_physical_vertex(int index);
            const PhysicalVertex & get_physical_vertex(int index) const;

            // returns equilibrium position of vertex
            // (measured off the initial center of mass of the cluster)
            // taking into account plasticity_state
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            const Math::TriVector & get_equilibrium_offset_pos(int index) const;
#else
            const Math::Vector & get_equilibrium_offset_pos(int index) const;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            // returns addition index for index'th vertex
            const int get_addition_index(int index) const;

            Math::Real get_total_mass() const { return total_mass; }

            Math::Real get_goal_speed_constant() const { return goal_speed_constant; }
            Math::Real get_linear_elasticity_constant() const { return linear_elasticity_constant; }
            Math::Real get_yield_constant() const { return yield_constant; }
            Math::Real get_creep_constant() const { return creep_constant; }

            const Math::Vector & get_center_of_mass() const { return center_of_mass; }
            const Math::Vector & get_initial_center_of_mass() const { return initial_center_of_mass; }
            /*
            const Math::Matrix & get_rotation() const { return rotation; }
            const Math::Matrix & get_total_deformation() const { return total_deformation; }
            const Math::Matrix & get_plasticity_state() const { return plasticity_state; }
            const Math::Matrix & get_plasticity_state_inv_tr() const { return plasticity_state_inv_trans; }
            */
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            const Math::TriMatrix & get_graphical_pos_transform() const { return graphical_pos_transform; }
#else
            const Math::Matrix & get_graphical_pos_transform() const { return graphical_pos_transform; }
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
            const Math::Matrix & get_graphical_nrm_transform() const { return graphical_nrm_transform; }
            Math::Real get_relative_plastic_deformation() const;
        
            void log_properties(int id);

            static const int INITIAL_ALLOCATED_VERTICES_NUM = 100;

            static const Math::Real DEFAULT_GOAL_SPEED_CONSTANT;
            static const Math::Real DEFAULT_LINEAR_ELASTICITY_CONSTANT;
            static const Math::Real DEFAULT_YIELD_CONSTANT;
            static const Math::Real DEFAULT_CREEP_CONSTANT;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            static const Math::Real DEFAULT_QX_CREEP_CONSTANT;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            static const Math::Real DEFAULT_MAX_DEFORMATION_CONSTANT;
        private:
            // No copying!
            Cluster(const Cluster &);
            Cluster & operator=(const Cluster &);
        };
    }
}

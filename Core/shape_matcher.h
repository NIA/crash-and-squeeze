#pragma once

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
        // An internal struct defining a membership of vertex in ShapeMatcher
        struct PhysicalVertexMappingInfo
        {
            // index in model's vertex array
            PhysicalVertex *vertex;

            // TODO: thread-safe cluster addition: Math::Vector velocity_additions[MAX_CLUSTERS_FOR_VERTEX]
            
            // initial position of vertex measured off
            // the cluster's center of mass
            Math::Vector initial_offset_pos;
            // position in deformed shape (plasticity_state*initial_offset_position)
            Math::Vector equilibrium_offset_pos;
            // position in model coordinates
            Math::Vector equilibrium_pos;
        };

        class ShapeMatcher
        {
        private:
            // -- constant (at run-time) fields --
            
            // array of vertices belonging to the ShapeMatcher
            Collections::Array<PhysicalVertexMappingInfo> vertex_infos;

            Math::Real total_mass;

            bool initial_characteristics_computed;
            bool valid;

            // -- variable (at run-time) fields --

            // center of mass of vertices
            Math::Vector center_of_mass;
            
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
            
            // -- helpers --
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

        public:
            ShapeMatcher();
            
            // -- methods --

            // reset to initial, empty state
            void reset();

            void add_vertex(PhysicalVertex &vertex);

            // this must be called after last vertex is added
            void compute_initial_characteristics();

            bool is_valid() const { return valid; }

            // Updates equilibrium offset positions after the shape to be matched has changed due to transformation.
            // NOTE: vertices are left untouched, only internal data are changed.
            void update_equilibrium_offset_positions(const Math::Matrix &transformation);
            
            // updates equilibrium positions of contained vertices
            // NOTE: vertices' fields `equilibrium_position' are modified (!)
            void update_vertices_equilibrium_positions();
            
            void match_shape();

            // -- getters/setters --
 
            int get_vertices_num() const { return vertex_infos.size(); }

            PhysicalVertex & get_vertex(int index);
            const PhysicalVertex & get_vertex(int index) const;

            // returns equilibrium position of vertex
            // (measured off the initial center of mass of the cluster)
            // taking into account plasticity_state
            const Math::Vector & get_equilibrium_offset_pos(int index) const;

            const Math::Vector & get_center_of_mass() const { return center_of_mass; }

            const Math::Matrix & get_linear_transformation() const { return linear_transformation; }
            const Math::Matrix & get_rotation() const { return rotation; }
            const Math::Matrix & get_scale() const { return scale; }

            static const int INITIAL_ALLOCATED_VERTICES_NUM = 100;

        private:
            // No copying!
            ShapeMatcher(const ShapeMatcher &);
            ShapeMatcher & operator=(const ShapeMatcher &);
        };
    }
}

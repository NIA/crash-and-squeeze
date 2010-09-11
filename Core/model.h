#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Core/force.h"
#include "Math/floating_point.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace CrashAndSqueeze
{
    class Cluster;

    namespace Core
    {
        struct PhysicalVertex;
        class Cluster;

        class Model
        {
        private:
            PhysicalVertex *vertices;
            int vertices_num;

            Math::Real total_mass;

            Cluster *clusters;
            int clusters_num;

            // -- fields used in initialization --
            int clusters_by_axes[Math::VECTOR_SIZE];
            Math::Real cluster_padding_coeff;
            
            // minimum values of coordinates of vertices
            Math::Vector min_pos;
            // maximum values of coordinates of vertices
            Math::Vector max_pos;
            // dimensions of a cluster
            Math::Vector cluster_sizes;

            // -- initialization steps --
            void init_vertices(const void *source_vertices,
                               VertexInfo const &vertex_info,
                               const MassFloat *masses,
                               const MassFloat constant_mass);
            
            void init_clusters();
            
            void add_vertex_to_clusters(PhysicalVertex &vertex);

            // -- fields used in step computation --
            Math::Vector center_of_mass;
            Math::Matrix inertia_tensor;
            Math::Vector center_of_mass_velocity;
            Math::Vector angular_velocity;
            
            // -- step computation steps --
            // computes center of mass and inertia tensor
            void find_body_properties();
            // computes velocity of center of mass and angular velocity
            void find_body_motion();
            // corrects velocity additions from shape matching
            void correct_velocity_additions();
            // damps oscillation velocity
            void damp_velocity(PhysicalVertex &v);
            
            // helper for find_body_motion() and correct_velocity_additions()
            Math::Vector compute_angular_velocity(const Math::Vector &angular_momentum);

            // TODO: DisplayVertex display_vertices; int display_vertices_num;
        public:
            // TODO: low-/hi-polygonal meshes
            // Takes a pointer source_vertices to vetrices_num vertices of arbitrary
            // strucutre, described by vertex_info. Takes a pointer masses to vertices_num
            // values of mass. If masses are equal for all vertices, it can be null
            // and the mass should be given as constant_mass argument.
            Model(const void *source_vertices,
                  int vetrices_num,
                  VertexInfo const &vertex_info,
                  const int clusters_by_axes[Math::VECTOR_SIZE],
                  Math::Real cluster_padding_coeff,
                  const MassFloat *masses,
                  const MassFloat constant_mass = 0);
            
            virtual ~Model();

            void compute_next_step(const Force * const forces[], int forces_num);

            void update_vertices(/*out*/ void *vertices, int vertices_num, VertexInfo const &vertex_info);

            int get_vertices_num() const { return vertices_num; }
            int get_clusters_num() const { return vertices_num; }
            
            // TODO: function used for testing, is needed anyway?
            PhysicalVertex const *get_vertices() const { return vertices; }
            // TODO: function used for testing, is needed anyway?
            Cluster const *get_clusters() const { return clusters; }
        private:
            // No copying!
            Model(const Model &);
            Model & operator=(const Model &);
        };
    }
}

#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Core/force.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        struct PhysicalVertex;
        class Cluster;

        class Model
        {
        private:
            PhysicalVertex *vertices;
            int vertices_num;

            Cluster *clusters;
            int clusters_num;

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
                  const MassFloat *masses,
                  const MassFloat constant_mass = 0);
            
            virtual ~Model();

            void compute_next_step(const Force *forces, int forces_num);

            void update_vertices(/*out*/ void *vertices, int vertices_num, VertexInfo const &vertex_info);

            int get_vertices_num() const { return vertices_num; }
            PhysicalVertex const *get_vertices() const { return vertices; }
            int get_clusters_num() const { return vertices_num; }
            Cluster const *get_clusters() const { return clusters; }
        private:
            // No copying!
            Model(const Model &);
            Model & operator=(const Model &);
        };
    }
}

#include "model.h"
#include "physical_vertex.h"
#include "cluster.h"
#include <cstdlib>
#include <assert.h>

namespace CrashAndSqueeze
{
    using namespace Math;
    
    namespace Core
    {
        inline const void *add_to_pointer(const void *pointer, int offset)
        {
            return reinterpret_cast<const void*>( reinterpret_cast<const char*>(pointer) + offset );
        }

        Model::Model( const void *source_vertices,
                      int vertices_num,
                      VertexInfo const &vertex_info,
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices_num(vertices_num), clusters_num(0), vertices(NULL), clusters(NULL)
        {
            // TODO: errors
            assert(this->vertices_num >= 0);
            this->vertices = new PhysicalVertex[this->vertices_num];

            // TODO: decomposition into clusters, only one so far
            clusters_num = 1;
            clusters = new Cluster[clusters_num];

            const void *source_vertex = source_vertices;
            for(int i = 0; i < this->vertices_num; ++i)
            {
                PhysicalVertex &vertex = this->vertices[i];
                
                // TODO: many points and vectors, only position so far
                const VertexFloat *position = reinterpret_cast<const VertexFloat*>( add_to_pointer(source_vertex, vertex_info.points_offsets[0]));
                vertex.pos = Vector( static_cast<Real>(position[0]),
                                     static_cast<Real>(position[1]),
                                     static_cast<Real>(position[2]) );
                
                if( NULL != masses )
                    vertex.mass = static_cast<Real>(masses[i]);
                else
                    vertex.mass = constant_mass;

                clusters[0].add_vertex(i, vertex);

                source_vertex = add_to_pointer(source_vertex, vertex_info.vertex_size);
            }
        }

        Model::~Model()
        {
            if(NULL != vertices) delete[] vertices;
            if(NULL != clusters) delete[] clusters;
        }

    }
}

#include "Core/model.h"
#include "Core/physical_vertex.h"
#include "Core/cluster.h"
#include <cstdlib>

namespace CrashAndSqueeze
{
    using namespace Math;
    using namespace Logging;
    
    namespace Core
    {
        namespace
        {
            inline const void *add_to_pointer(const void *pointer, int offset)
            {
                return reinterpret_cast<const void*>( reinterpret_cast<const char*>(pointer) + offset );
            }
            inline void *add_to_pointer(void *pointer, int offset)
            {
                return reinterpret_cast<void*>( reinterpret_cast<char*>(pointer) + offset );
            }
        }

        Model::Model( const void *source_vertices,
                      int vertices_num,
                      VertexInfo const &vertex_info,
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices_num(vertices_num), clusters_num(0), vertices(NULL), clusters(NULL)
        {
            if(this->vertices_num < 0)
            {
                logger.error("creating model with `vertices_count` < 0", __FILE__, __LINE__);
                this->vertices_num = 0;
            }
            else
            {
                this->vertices = new PhysicalVertex[this->vertices_num];

                // TODO: decomposition into clusters, only one so far
                clusters_num = 1;
                clusters = new Cluster[clusters_num];

                const void *source_vertex = source_vertices;
                for(int i = 0; i < this->vertices_num; ++i)
                {
                    PhysicalVertex &vertex = this->vertices[i];
                    
                    const VertexFloat *position = reinterpret_cast<const VertexFloat*>( add_to_pointer(source_vertex, vertex_info.get_point_offset(0)));
                    vertex.pos = Vector( static_cast<Real>(position[0]),
                                         static_cast<Real>(position[1]),
                                         static_cast<Real>(position[2]) );
                    
                    if( NULL != masses )
                    {
                        vertex.mass = static_cast<Real>(masses[i]);
                        if( equal(0, vertex.mass) )
                            logger.warning("creating model with vertex having zero mass. Forces will not be applied to such vertex", __FILE__, __LINE__);
                    }
                    else
                    {
                        vertex.mass = constant_mass;
                    }
                    
                    

                    clusters[0].add_vertex(i, vertex);

                    source_vertex = add_to_pointer(source_vertex, vertex_info.get_vertex_size());
                }
                if( equal(0, constant_mass) && NULL == masses )
                    logger.warning("creating model with constant zero mass of vertices. Forces will not be applied to such model", __FILE__, __LINE__);

            }
        }

        void Model::compute_next_step(const Force *forces, int forces_num)
        {
            if(NULL == forces)
            {
                logger.error("null pointer `forces` in Model::compute_next_step", __FILE__, __LINE__);
                return;
            }
            
            // TODO: QueryPerformanceCounter
            Real dt = 0.001;

            Vector goal_position;
            for(int i = 0; i < clusters_num; ++i)
            {
                Cluster &cluster = clusters[i];
                
                Vector center_of_mass = Vector::ZERO;
                for(int j = 0; j < cluster.get_vertices_num(); ++j)
                {
                    PhysicalVertex &vertex = vertices[cluster.get_vertex_index(j)];
                    center_of_mass += vertex.mass*vertex.pos/cluster.get_total_mass();
                }
                cluster.set_center_of_mass(center_of_mass);

                for(int j = 0; j < cluster.get_vertices_num(); ++j)
                {
                    PhysicalVertex &vertex = vertices[cluster.get_vertex_index(j)];
                    
                    goal_position = cluster.get_initial_vertex_offset_position(j) + center_of_mass;
                    // TODO: thread-safe cluster addition: velocity_additions[]...
                    vertex.velocity += cluster.get_goal_speed_constant()*(goal_position - vertex.pos)/dt;

                    // !!! one-cluster hack
                    //vertex.pos -= center_of_mass;
                }
            }

            Vector acceleration;
            for(int i = 0; i < vertices_num; ++i)
            {
                acceleration = Vector(0,0,0);

                if(0 != vertices[i].mass)
                {
                    for(int j=0; j < forces_num; ++j)
                    {
                        if(forces[j].is_applied_to(vertices[i].pos))
                            acceleration += forces[j].get_value_at(vertices[i].pos)/vertices[i].mass;
                    }
                }

                vertices[i].velocity += acceleration*dt;
                vertices[i].pos += vertices[i].velocity*dt;
            }
        }

        void Model::update_vertices(/*out*/ void *vertices, int vertices_num, VertexInfo const &vertex_info)
        {
            if(vertices_num > this->vertices_num)
            {
                logger.warning("requested to update too many vertices in Model::update_vertices, probably wrong vertices given?");
                vertices_num = this->vertices_num;
            }

            void *out_vertex = vertices;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &vertex = this->vertices[i];
                
                // TODO: many points and vectors, only position so far
                VertexFloat *position = reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertex, vertex_info.get_point_offset(0)));
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    position[j] = static_cast<VertexFloat>(vertex.pos[j]);

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        Model::~Model()
        {
            if(NULL != vertices) delete[] vertices;
            if(NULL != clusters) delete[] clusters;
        }

    }
}

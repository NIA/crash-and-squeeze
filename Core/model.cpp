#include "Core/model.h"
#include "Core/physical_vertex.h"
#include "Core/cluster.h"
#include <cstdlib>

namespace CrashAndSqueeze
{
    using Math::Vector;
    using Math::VECTOR_SIZE;
    using Math::Matrix;
    using Math::Real;
    using Math::equal;
    using Math::sign;
    using Logging::logger;
    
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

            const Real MAX_COORDINATE = 1.0e+100; // !!! hard-coded
            const Vector MAX_COORDINATE_VECTOR(MAX_COORDINATE, MAX_COORDINATE, MAX_COORDINATE);

            inline int compute_cluster_index(const int indices[VECTOR_SIZE], const int clusters_num[VECTOR_SIZE])
            {
                return indices[0] + indices[1]*clusters_num[0] + indices[2]*clusters_num[0]*clusters_num[1];
            }
        }

        Model::Model( const void *source_vertices,
                      int vertices_num,
                      VertexInfo const &vertex_info,
                      const int clusters_by_axes[Math::VECTOR_SIZE],
                      Math::Real cluster_padding_coeff,
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices_num(vertices_num), clusters_num(0), vertices(NULL), clusters(NULL), total_mass(0),
              min_pos(MAX_COORDINATE_VECTOR), max_pos(-MAX_COORDINATE_VECTOR), cluster_padding_coeff(cluster_padding_coeff),
              center_of_mass_velocity(Vector::ZERO), angular_velocity(Vector::ZERO), center_of_mass(Vector::ZERO),
              inertia_tensor(Matrix::ZERO)

        {
            init_vertices(source_vertices, vertex_info, masses, constant_mass);
            
            for(int i = 0; i < VECTOR_SIZE; ++i)
                this->clusters_by_axes[i] = clusters_by_axes[i];

            find_body_properties();
            
            init_clusters();
        }

        void Model::init_vertices(const void *source_vertices,
                                  VertexInfo const &vertex_info,
                                  const MassFloat *masses,
                                  const MassFloat constant_mass)
        {
            if(this->vertices_num < 0)
            {
                logger.error("creating model with `vertices_count` < 0", __FILE__, __LINE__);
                this->vertices_num = 0;
            }
            else
            {
                this->vertices = new PhysicalVertex[this->vertices_num];


                const void *source_vertex = source_vertices;
                if( equal(0, constant_mass) && NULL == masses )
                    logger.warning("creating model with constant zero mass of vertices. Forces will not be applied to such model", __FILE__, __LINE__);
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
                    total_mass += vertex.mass;

                    for(int j = 0; j < VECTOR_SIZE; ++j)
                    {
                        if(vertex.pos[j] < min_pos[j])
                            min_pos[j] = vertex.pos[j];

                        if(vertex.pos[j] > max_pos[j])
                            max_pos[j] = vertex.pos[j];
                    }
                    
                    source_vertex = add_to_pointer(source_vertex, vertex_info.get_vertex_size());
                }
            }
        }
        
        void Model::init_clusters()
        {
            if(0 == vertices_num)
                return;

            clusters_num = 1;
            for(int i = 0; i < VECTOR_SIZE; ++i)
                clusters_num *= clusters_by_axes[i];
            clusters = new Cluster[clusters_num];

            const Vector dimensions = max_pos - min_pos;
            
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                if(0 == clusters_by_axes[i])
                    logger.error("creating model with zero clusters_by_axes component", __FILE__, __LINE__);
                cluster_sizes[i] = dimensions[i]/clusters_by_axes[i];
            }
            
            // -- For each vertex --
            for(int i = 0; i < this->vertices_num; ++i)
            {
                add_vertex_to_clusters(vertices[i]);
            } 
        }

        void Model::add_vertex_to_clusters(PhysicalVertex &vertex)
        {
            const Vector padding = cluster_sizes*cluster_padding_coeff;

            // -- find position, measured off the min_pos --
            const Vector position = vertex.pos - min_pos;

            // -- choose a cluster --

            // "coordinates" of a cluster: axis indices
            int cluster_indices[VECTOR_SIZE];
            for(int j = 0; j < VECTOR_SIZE; ++j)
            {
                if(0 == cluster_sizes[j])
                    logger.error("creating model with zero dimension", __FILE__, __LINE__);
                cluster_indices[j] = static_cast<int>(position[j]/cluster_sizes[j]);
                
                if(cluster_indices[j] < 0)
                    cluster_indices[j] = 0;
                if(cluster_indices[j] > clusters_by_axes[j] - 1)
                    cluster_indices[j] = clusters_by_axes[j] - 1;
            }

            // -- and assign to it --
            int cluster_index = compute_cluster_index(cluster_indices, clusters_by_axes);
            clusters[cluster_index].add_vertex(vertex);

            // -- and, probably, to his neighbours --
            for(int j = 0; j < VECTOR_SIZE; ++j)
            {
                // -- previous cluster --
                
                if( cluster_indices[j] - 1 >= 0 &&
                    abs(position[j] - cluster_indices[j]*cluster_sizes[j]) < padding[j] )
                {
                    --cluster_indices[j];
                    int cluster_index = compute_cluster_index(cluster_indices, clusters_by_axes);
                    clusters[cluster_index].add_vertex(vertex);
                    ++cluster_indices[j];
                }

                // -- next cluster --

                if( cluster_indices[j] + 1 <= clusters_by_axes[j] - 1 &&
                    abs((cluster_indices[j] + 1)*cluster_sizes[j] - position[j]) < padding[j] )
                {
                    ++cluster_indices[j];
                    int cluster_index = compute_cluster_index(cluster_indices, clusters_by_axes);
                    clusters[cluster_index].add_vertex(vertex);
                    --cluster_indices[j];
                }
            }
        }

        namespace
        {
            void integrate_velocity(PhysicalVertex &v, const Force * const forces[], int forces_num, Math::Real dt)
            {
                Vector acceleration = Vector::ZERO;

                if(0 != v.mass && NULL != forces)
                {
                    for(int j=0; j < forces_num; ++j)
                    {
                        if(NULL == forces[j])
                        {
                            logger.error("in Model::integrate_velocity: null pointer item of `forces` array ", __FILE__, __LINE__);
                            return;
                        }
                        acceleration += forces[j]->get_value_at(v.pos, v.velocity)/v.mass;
                    }
                }

                v.velocity += v.velocity_addition + acceleration*dt;
                v.velocity_addition = Vector::ZERO;
            }

            void integrate_position(PhysicalVertex &v, Math::Real dt)
            {
                v.pos += v.velocity*dt;
            }
        }

        void Model::find_body_properties()
        {
            if(0 == total_mass)
                return;
            
            center_of_mass = Vector::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];
                
                center_of_mass += v.mass*v.pos/total_mass;
            }

            inertia_tensor = Matrix::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];
                Vector offset = v.pos - center_of_mass;
                
                inertia_tensor += v.mass * ( (offset*offset)*Matrix::IDENTITY - Matrix(offset, offset) );
            }
        }

        Vector Model::compute_angular_velocity(const Vector &angular_momentum)
        {
            if( !inertia_tensor.is_invertible() )
            {
                logger.error("in Model::compute_next_step: inertia_tensor is singular, cannot find angular velocity", __FILE__, __LINE__);
                return Vector::ZERO;
            }
            return inertia_tensor.inverted()*angular_momentum;
        }

        void Model::find_body_motion()
        {
            if(0 == total_mass)
                return;

            center_of_mass_velocity = Vector::ZERO;
            Vector angular_momentum = Vector::ZERO;
            
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];
                
                center_of_mass_velocity += v.mass*v.velocity/total_mass;
                angular_momentum += v.mass * cross_product(v.pos - center_of_mass, v.velocity);
            }
            angular_velocity = compute_angular_velocity(angular_momentum);
        }

        void Model::correct_velocity_additions()
        {
            Vector linear_momentum_addition = Vector::ZERO;
            Vector angular_momentum_addition = Vector::ZERO;

            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];

                if(0 == v.including_clusters_num)
                {
                    logger.error("in Model::correct_velocity_additions: internal error: orphan vertex not belonging to any cluster", __FILE__, __LINE__);
                    return;
                }

                // we need average velocity addition, not sum
                v.velocity_addition /= v.including_clusters_num;

                linear_momentum_addition += v.mass*v.velocity_addition;
                angular_momentum_addition += v.mass * cross_product(v.pos - center_of_mass, v.velocity_addition);
            }

            
            if(0 != total_mass)
            {
                Vector center_of_mass_velocity_addition = linear_momentum_addition / total_mass;
                Vector angular_velocity_addition = compute_angular_velocity(angular_momentum_addition);

                for(int i = 0; i < vertices_num; ++i)
                {
                    PhysicalVertex &v = vertices[i];

                    Vector velocity_correction = - center_of_mass_velocity_addition
                                                 - cross_product(angular_velocity_addition, v.pos - center_of_mass);

                    v.velocity_addition += velocity_correction;
                }
            }
        }

        void Model::damp_velocity(PhysicalVertex &v)
        {
            Vector rigid_velocity = center_of_mass_velocity + cross_product(angular_velocity, v.pos - center_of_mass);
            Vector oscillation_velocity = v.velocity - rigid_velocity;
            v.velocity -= DEFAULT_DAMPING_CONSTANT*oscillation_velocity; // !!!
        }

        void Model::compute_next_step(const Force * const forces[], int forces_num)
        {
            if(NULL == forces && 0 != forces_num)
            {
                logger.error("in Model::compute_next_step: null pointer `forces`", __FILE__, __LINE__);
                return;
            }
            
            // TODO: QueryPerformanceCounter
            Real dt = 0.01;

            // -- For each cluster of model: do shape matching --
            for(int i = 0; i < clusters_num; ++i)
            {
                clusters[i].match_shape(dt);
            }
            
            find_body_properties();
            
            correct_velocity_additions();
            
            // -- For each vertex of model: integrate velocities --
            for(int i = 0; i < vertices_num; ++i)
            {
                integrate_velocity( vertices[i], forces, forces_num, dt );
            }
            
            find_body_motion();

            // -- For each vertex of model: damp velocities and integrate positions --
            for(int i = 0; i < vertices_num; ++i)
            {
                damp_velocity( vertices[i] );
                integrate_position( vertices[i], dt );
            }
        }

        void Model::update_vertices(/*out*/ void *vertices, int vertices_num, VertexInfo const &vertex_info)
        {
            if(vertices_num > this->vertices_num)
            {
                logger.warning("in Model::update_vertices: requested to update too many vertices, probably wrong vertices given?");
                vertices_num = this->vertices_num;
            }

            void *out_vertex = vertices;
            for(int i = 0; i < vertices_num; ++i)
            {
                const PhysicalVertex &vertex = this->vertices[i];
                
                /* // !!! one-cluster hack
                const Cluster &cluster = clusters[0];
                Vector pos = cluster.get_total_deformation() * cluster.get_initial_vertex_offset_position(i)
                           + cluster.get_center_of_mass(); */
                
                // TODO: many points and vectors, only position so far
                VertexFloat *position = reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertex, vertex_info.get_point_offset(0)));
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    position[j] = static_cast<VertexFloat>(vertex.pos[j]);

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        Model::~Model()
        {
            delete[] vertices;
            delete[] clusters;
        }

    }
}

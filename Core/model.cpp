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
    using Math::less_or_equal;
    using Math::sign;
    using Logging::logger;
    
    namespace Core
    {
        // a constant, determining how much deformation velocities are damped:
        // 0 - no damping of vibrations, 1 - maximum damping, rigid body
        const Real Model::DEFAULT_DAMPING_CONSTANT = 0.7;
        
        namespace
        {
            // -- useful constants --

            const Real MAX_COORDINATE = 1.0e+100; // !!! hard-coded
            const Vector MAX_COORDINATE_VECTOR(MAX_COORDINATE, MAX_COORDINATE, MAX_COORDINATE);
            
            // -- helpers --

            inline const void *add_to_pointer(const void *pointer, int offset)
            {
                return reinterpret_cast<const void*>( reinterpret_cast<const char*>(pointer) + offset );
            }
            inline void *add_to_pointer(void *pointer, int offset)
            {
                return reinterpret_cast<void*>( reinterpret_cast<char*>(pointer) + offset );
            }

            // TODO: test this
            int axis_indices_to_index(const int indices[VECTOR_SIZE], const int clusters_by_axes[VECTOR_SIZE])
            {
                return indices[0] + indices[1]*clusters_by_axes[0] + indices[2]*clusters_by_axes[0]*clusters_by_axes[1];
            }
            void index_to_axis_indices(int index, const int clusters_by_axes[VECTOR_SIZE], /* out */ int indices[VECTOR_SIZE])
            {
                int horisontal_layer_size = clusters_by_axes[0]*clusters_by_axes[1];
                indices[2] = index / horisontal_layer_size;
                indices[1] = (index % horisontal_layer_size) / clusters_by_axes[1];
                indices[0] = (index % horisontal_layer_size) % clusters_by_axes[1];
            }
            
            bool integrate_velocity(PhysicalVertex &v, const Force * const forces[], int forces_num, Math::Real dt)
            {
                Vector acceleration = Vector::ZERO;

                if(0 != v.mass && NULL != forces)
                {
                    for(int j=0; j < forces_num; ++j)
                    {
                        if(NULL == forces[j])
                        {
                            logger.error("in Model::compute_next_step: null pointer item of `forces` array ", __FILE__, __LINE__);
                            return false;
                        }
                        acceleration += forces[j]->get_value_at(v.pos, v.velocity)/v.mass;
                    }
                }

                v.velocity += v.velocity_addition + acceleration*dt;
                v.velocity_addition = Vector::ZERO;
                return true;
            }

            void integrate_position(PhysicalVertex &v, Math::Real dt)
            {
                v.pos += v.velocity*dt;
            }
        }

        Model::Model( const void *source_vertices,
                      int vertices_num,
                      VertexInfo const &vertex_info,
                      const int clusters_by_axes[Math::VECTOR_SIZE],
                      Math::Real cluster_padding_coeff,
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices(NULL),
              vertices_num(vertices_num),
              
              clusters_num(0),
              clusters(NULL),
              
              cluster_padding_coeff(cluster_padding_coeff),
              damping_constant(DEFAULT_DAMPING_CONSTANT),

              min_pos(MAX_COORDINATE_VECTOR),
              max_pos(-MAX_COORDINATE_VECTOR), 
              
              total_mass(0),
              center_of_mass(Vector::ZERO),
              inertia_tensor(Matrix::ZERO),
              center_of_mass_velocity(Vector::ZERO),
              angular_velocity(Vector::ZERO)
        {
            if( init_vertices(source_vertices, vertex_info, masses, constant_mass) )
            {
                if( find_body_properties() )
                {
                    for(int i = 0; i < VECTOR_SIZE; ++i)
                        this->clusters_by_axes[i] = clusters_by_axes[i];
                    
                    init_clusters();
                }
            }
        }

        bool Model::init_vertices(const void *source_vertices,
                                  VertexInfo const &vertex_info,
                                  const MassFloat *masses,
                                  const MassFloat constant_mass)
        {
            if(this->vertices_num <= 0)
            {
                logger.error("creating model with `vertices_count` <= 0", __FILE__, __LINE__);
                return false;
            }
            else
            {
                this->vertices = new PhysicalVertex[this->vertices_num];


                const void *source_vertex = source_vertices;
                
                if( less_or_equal(constant_mass, 0) && NULL == masses )
                {
                    logger.error("creating model with constant vertex mass <= 0. Vertex mass must be strictly positive.", __FILE__, __LINE__);
                    return false;
                }

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
                        if( less_or_equal(vertex.mass, 0) )
                        {
                            logger.error("creating model with vertex having mass <= 0. Vertex mass must be strictly positive.", __FILE__, __LINE__);
                            return false;
                        }
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
                return true;
            }
        }
        
        bool Model::init_clusters()
        {
            clusters_num = 1;
            for(int i = 0; i < VECTOR_SIZE; ++i)
                clusters_num *= clusters_by_axes[i];
            clusters = new Cluster[clusters_num];

            const Vector dimensions = max_pos - min_pos;
            
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                if(0 == clusters_by_axes[i])
                {
                    logger.error("creating model with zero clusters_by_axes component", __FILE__, __LINE__);
                    return false;
                }
                cluster_sizes[i] = dimensions[i]/clusters_by_axes[i];
            }
            
            // -- For each vertex: assign --
            for(int i = 0; i < this->vertices_num; ++i)
            {
                if ( false == add_vertex_to_clusters(vertices[i]) )
                    return false;
            }

            int axis_indices[VECTOR_SIZE];
            // -- For each cluster: set its space --
            for(int i = 0; i < clusters_num; ++i)
            {
                index_to_axis_indices(i, clusters_by_axes, axis_indices);
                if( axis_indices_to_index(axis_indices, clusters_by_axes) != i )
                {
                    logger.error("in Model::init_clusters: internal assertion failed: i -> {i1, i2, i3} -> i', i' != i");
                    return false;
                }

                Vector pos;
                for(int j = 0; j < VECTOR_SIZE; ++j)
                    pos[j] = axis_indices[j]*cluster_sizes[j];

                clusters[i].set_space(pos, cluster_sizes);
            }

            // -- For each cluster: precompute --
            for(int i = 0; i < clusters_num; ++i)
            {
                clusters[i].compute_initial_characteristics();
            }

            return true;
        }

        bool Model::get_nearest_cluster_indices(const Math::Vector position, /*out*/ int cluster_indices[VECTOR_SIZE])
        {
            for(int j = 0; j < VECTOR_SIZE; ++j)
            {
                if(equal(0, cluster_sizes[j]))
                {
                    logger.error("in Model::get_nearest_cluster_indices: cluster with a zero dimension, probably creating model with a zero or too little dimension", __FILE__, __LINE__);
                    return false;
                }
                cluster_indices[j] = static_cast<int>(position[j]/cluster_sizes[j]);
                
                if(cluster_indices[j] < 0)
                    cluster_indices[j] = 0;
                if(cluster_indices[j] > clusters_by_axes[j] - 1)
                    cluster_indices[j] = clusters_by_axes[j] - 1;
            }

            return true;
        }

        bool Model::add_vertex_to_clusters(PhysicalVertex &vertex)
        {
            const Vector padding = cluster_sizes*cluster_padding_coeff;

            // -- find position, measured off the min_pos --
            const Vector position = vertex.pos - min_pos;

            // -- choose a cluster --

            int cluster_indices[VECTOR_SIZE];
            if( false == get_nearest_cluster_indices(position, cluster_indices) )
                return false;
            int cluster_index = axis_indices_to_index(cluster_indices, clusters_by_axes);

            // -- and assign to it --
            vertex.nearest_cluster_index = cluster_index;
            clusters[cluster_index].add_vertex(vertex);

            // -- and, probably, to his neighbours --
            for(int j = 0; j < VECTOR_SIZE; ++j)
            {
                // -- previous cluster --
                
                if( cluster_indices[j] - 1 >= 0 &&
                    abs(position[j] - cluster_indices[j]*cluster_sizes[j]) < padding[j] )
                {
                    --cluster_indices[j];
                    int cluster_index = axis_indices_to_index(cluster_indices, clusters_by_axes);
                    clusters[cluster_index].add_vertex(vertex);
                    ++cluster_indices[j];
                }

                // -- next cluster --

                if( cluster_indices[j] + 1 <= clusters_by_axes[j] - 1 &&
                    abs((cluster_indices[j] + 1)*cluster_sizes[j] - position[j]) < padding[j] )
                {
                    ++cluster_indices[j];
                    int cluster_index = axis_indices_to_index(cluster_indices, clusters_by_axes);
                    clusters[cluster_index].add_vertex(vertex);
                    --cluster_indices[j];
                }
            }

            return true;
        }
        
        void  Model::set_space_deformation_callback(SpaceDeformationCallback callback, Math::Real threshold)
        {
            for(int i = 0; i < clusters_num; ++i)
            {
                clusters[i].set_deformation_callback(callback, threshold);
            }
        }

        bool Model::check_total_mass()
        {
            if(less_or_equal(total_mass, 0))
            {
                logger.error("internal error: Model run-time check: total_mass is <= 0", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        bool Model::find_body_properties()
        {
            if( false == check_total_mass() )
                return false;
            
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

            return true;
        }

        bool Model::compute_angular_velocity(const Vector &angular_momentum, /*out*/ Vector & result)
        {
            if( ! inertia_tensor.is_invertible() )
            {
                logger.error("in Model::compute_angular_velocity: inertia_tensor is singular, cannot invert to find angular velocity", __FILE__, __LINE__);
                return false;
            }
            result = inertia_tensor.inverted()*angular_momentum;
            return true;
        }

        bool Model::find_body_motion()
        {
            if( false == check_total_mass() )
                return false;

            center_of_mass_velocity = Vector::ZERO;
            Vector angular_momentum = Vector::ZERO;
            
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];
                
                center_of_mass_velocity += v.mass*v.velocity/total_mass;
                angular_momentum += v.mass * cross_product(v.pos - center_of_mass, v.velocity);
            }
            if( false == compute_angular_velocity(angular_momentum, angular_velocity) )
                return false;

            return true;
        }

        bool Model::correct_velocity_additions()
        {
            if( false == check_total_mass() )
                return false;

            Vector linear_momentum_addition = Vector::ZERO;
            Vector angular_momentum_addition = Vector::ZERO;

            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];

                linear_momentum_addition += v.mass*v.velocity_addition;
                angular_momentum_addition += v.mass * cross_product(v.pos - center_of_mass, v.velocity_addition);
            }

            Vector center_of_mass_velocity_addition = linear_momentum_addition / total_mass;
            
            Vector angular_velocity_addition;
            if( false == compute_angular_velocity(angular_momentum_addition, angular_velocity_addition) )
                return false;

            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];

                Vector velocity_correction = - center_of_mass_velocity_addition
                                             - cross_product(angular_velocity_addition, v.pos - center_of_mass);

                v.velocity_addition += velocity_correction;
            }
            return true;
        }

        void Model::damp_velocity(PhysicalVertex &v)
        {
            Vector rigid_velocity = center_of_mass_velocity + cross_product(angular_velocity, v.pos - center_of_mass);
            Vector oscillation_velocity = v.velocity - rigid_velocity;
            v.velocity -= damping_constant*oscillation_velocity;
        }

        bool Model::compute_next_step(const Force * const forces[], int forces_num)
        {
            if(NULL == forces && 0 != forces_num)
            {
                logger.error("in Model::compute_next_step: null pointer `forces`", __FILE__, __LINE__);
                return false;
            }
            
            // TODO: QueryPerformanceCounter
            Real dt = 0.01;

            // -- For each cluster of model: do shape matching --
            for(int i = 0; i < clusters_num; ++i)
            {
                clusters[i].match_shape(dt);
            }
            
            if( false == find_body_properties())
                return false;
            
            if( false == correct_velocity_additions())
                return false;
            
            // -- For each vertex of model: integrate velocities --
            for(int i = 0; i < vertices_num; ++i)
            {
                if( false == integrate_velocity( vertices[i], forces, forces_num, dt ))
                    return false;
            }
            
            if( false == find_body_motion() )
                return false;

            // -- For each vertex of model: damp velocities and integrate positions --
            for(int i = 0; i < vertices_num; ++i)
            {
                damp_velocity( vertices[i] );
                integrate_position( vertices[i], dt );
            }

            return true;
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

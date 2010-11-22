#include "Core/model.h"
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
    using Logging::Logger;
    
    namespace Core
    {
        namespace
        {
            // -- useful constants --

            const Real MAX_COORDINATE = 1.0e+100; // !!! hard-coded
            const Vector MAX_COORDINATE_VECTOR(MAX_COORDINATE, MAX_COORDINATE, MAX_COORDINATE);
            const int INITIAL_ALLOCATED_CALLBACK_INFOS = 10;
            
            // -- helpers --

            inline const void *add_to_pointer(const void *pointer, int offset)
            {
                return reinterpret_cast<const void*>( reinterpret_cast<const char*>(pointer) + offset );
            }
            inline void *add_to_pointer(void *pointer, int offset)
            {
                return reinterpret_cast<void*>( reinterpret_cast<char*>(pointer) + offset );
            }
        }

        // a constant, determining how much deformation velocities are damped:
        // 0 - no damping of vibrations, 1 - maximum damping, rigid body
        const Real Model::DEFAULT_DAMPING_CONSTANT = 0.7*Body::MAX_RIGIDITY_COEFF;

        Model::Model( const void *source_vertices,
                      int vertices_num,
                      VertexInfo const &vertex_info,
                      const int clusters_by_axes[Math::VECTOR_SIZE],
                      Math::Real cluster_padding_coeff,
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices(vertices_num),
              initial_vertices(vertices_num),

              cluster_padding_coeff(cluster_padding_coeff),

              damping_constant(DEFAULT_DAMPING_CONSTANT),

              min_pos(MAX_COORDINATE_VECTOR),
              max_pos(-MAX_COORDINATE_VECTOR),

              body(NULL),
              initial_state(NULL),
              frame(NULL),

              hit_vertices_indices(vertices_num)
        {
            // -- Finish initialization of arrays --
            // -- (create enought items and freeze or just forbid reallocations) --
            
            vertices.create_items(vertices_num);
            vertices.freeze();

            initial_vertices.create_items(vertices_num);
            initial_vertices.freeze();

            hit_vertices_indices.forbid_reallocation(vertices.size());

            // Init vertices
            if( false != init_vertices(source_vertices, vertex_info, masses, constant_mass) )
            {
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    this->clusters_by_axes[i] = clusters_by_axes[i];
                
                // Init clusters
                init_clusters();
            }
        }

        bool Model::init_vertices(const void *source_vertices,
                                  VertexInfo const &vertex_info,
                                  const MassFloat *masses,
                                  const MassFloat constant_mass)
        {
            const void *source_vertex = source_vertices;
            
            if( less_or_equal(constant_mass, 0) && NULL == masses )
            {
                Logger::error("creating model with constant vertex mass <= 0. Vertex mass must be strictly positive.", __FILE__, __LINE__);
                return false;
            }

            for(int i = 0; i < vertices.size(); ++i)
            {
                const VertexFloat *position = reinterpret_cast<const VertexFloat*>( add_to_pointer(source_vertex, vertex_info.get_point_offset(0)));
                Vector pos = Vector( static_cast<Real>(position[0]),
                                     static_cast<Real>(position[1]),
                                     static_cast<Real>(position[2]) );
                
                Real mass;
                if( NULL != masses )
                {
                    mass = static_cast<Real>(masses[i]);
                    if( less_or_equal(mass, 0) )
                    {
                        Logger::error("creating model with vertex having mass <= 0. Vertex mass must be strictly positive.", __FILE__, __LINE__);
                        return false;
                    }
                }
                else
                {
                    mass = static_cast<Real>(constant_mass);
                }
                
                vertices[i] = initial_vertices[i] = PhysicalVertex(pos, mass);

                for(int j = 0; j < VECTOR_SIZE; ++j)
                {
                    if(pos[j] < min_pos[j])
                        min_pos[j] = pos[j];

                    if(pos[j] > max_pos[j])
                        max_pos[j] = pos[j];
                }
                
                source_vertex = add_to_pointer(source_vertex, vertex_info.get_vertex_size());
            }

            body = new Body(vertices);
            initial_state = new Body(initial_vertices);
            return true;
        }
        
        bool Model::init_clusters()
        {
            int clusters_num = 1;
            for(int i = 0; i < VECTOR_SIZE; ++i)
                clusters_num *= clusters_by_axes[i];

            clusters.create_items(clusters_num);
            clusters.freeze();

            const Vector dimensions = max_pos - min_pos;
            
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                if(0 == clusters_by_axes[i])
                {
                    Logger::error("creating model with zero clusters_by_axes component", __FILE__, __LINE__);
                    return false;
                }
                cluster_sizes[i] = dimensions[i]/clusters_by_axes[i];
            }
            
            // -- For each vertex: assign --
            for(int i = 0; i < vertices.size(); ++i)
            {
                if ( false == add_vertex_to_clusters(vertices[i]) )
                    return false;
            }

            // -- For each cluster: precompute --
            for(int i = 0; i < clusters.size(); ++i)
            {
                clusters[i].compute_initial_characteristics();
            }

            return true;
        }
        
        int Model::axis_indices_to_index(const int indices[VECTOR_SIZE], const int clusters_by_axes[VECTOR_SIZE])
        {
            return indices[0] + indices[1]*clusters_by_axes[0] + indices[2]*clusters_by_axes[0]*clusters_by_axes[1];
        }

        void Model::index_to_axis_indices(int index, const int clusters_by_axes[VECTOR_SIZE], /* out */ int indices[VECTOR_SIZE])
        {
            int horisontal_layer_size = clusters_by_axes[0]*clusters_by_axes[1];
            indices[2] = index / horisontal_layer_size;
            indices[1] = (index % horisontal_layer_size) / clusters_by_axes[0];
            indices[0] = (index % horisontal_layer_size) % clusters_by_axes[0];
        }

        bool Model::get_nearest_cluster_indices(const Math::Vector position, /*out*/ int cluster_indices[VECTOR_SIZE])
        {
            for(int j = 0; j < VECTOR_SIZE; ++j)
            {
                if(equal(0, cluster_sizes[j]))
                {
                    Logger::error("in Model::get_nearest_cluster_indices: cluster with a zero dimension, probably creating model with a zero or too little dimension", __FILE__, __LINE__);
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
            const Vector position = vertex.get_pos() - min_pos;

            // -- choose a cluster --

            int cluster_indices[VECTOR_SIZE];
            if( false == get_nearest_cluster_indices(position, cluster_indices) )
                return false;
            int cluster_index = axis_indices_to_index(cluster_indices, clusters_by_axes);

            // -- and assign to it --
            vertex.set_nearest_cluster_index(cluster_index);
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

        void Model::set_frame(const IndexArray &frame_indices)
        {
            delete frame;
            frame = new Body(vertices, frame_indices);
            frame->compute_properties();
        }

        void Model::hit(const IRegion &region, const Vector & velocity)
        {
            hit_vertices_indices.clear();
            Real region_mass = 0;

            // -- Find vertices in this region --
            
            for(int i = 0; i < vertices.size(); ++i)
            {
                PhysicalVertex &v = vertices[i];

                if( region.contains(v.get_pos()) )
                {
                    hit_vertices_indices.push_back(i);
                    region_mass += v.get_mass();
                }
            }

            // -- Report error if none found --

            if( equal(0, region_mass) )
            {
                Logger::warning("in Model::hit: there is no vertex inside given region", __FILE__, __LINE__);
                return;
            }

            // -- Add the same velocity to all vertices in region --

            Vector region_velocity = (velocity * body->get_total_mass())/region_mass;
            
            for(int i = 0; i < hit_vertices_indices.size(); ++i)
            {
                vertices[ hit_vertices_indices[i] ].add_to_velocity(region_velocity);
            }

            // -- Invoke reactions if needed --

            for(int i = 0; i < hit_reactions.size(); ++i)
            {
                hit_reactions[i]->invoke_if_needed(hit_vertices_indices, region_velocity);
            }
        }

        bool Model::compute_next_step(const ForcesArray & forces, Real dt,
                                      /*out*/ Vector & linear_velocity_change,
                                      /*out*/ Vector & angular_velocity_change)
        {
            shape_deform_reactions.freeze();

            // -- For each cluster of model: do shape matching --
            for(int i = 0; i < clusters.size(); ++i)
            {
                clusters[i].match_shape(dt);
            }
            
            // Re-compute properties (due to changed positions of vertices)
            if( false == body->compute_properties() )
                return false;
            
            // -- Force linear and angular momenta conservation --

            if( false == body->compute_velocity_additions() )
                return false;

            body->compensate_velocities( body->get_linear_velocity_addition(),
                                         body->get_angular_velocity_addition() );
            
            // -- For each vertex of model: integrate velocities: sum velocity additions and apply forces --
            for(int i = 0; i < vertices.size(); ++i)
            {
                if( false == vertices[i].integrate_velocity( forces, dt ) )
                    return false;
            }

            if( NULL != frame )
            {
                // -- Ensure that the frame moves as a rigid body --
            
                if( false == frame->compute_velocities() )
                    return false;

                frame->set_rigid_motion();
            }
            
            // Find macroscopic motion for damping and subsequent substraction
            if( false == body->compute_velocities() )
                return false;

            // Damp deformation oscillations
            body->set_rigid_motion(damping_constant);

            // -- Substract macroscopic motion of body --
            // (to make the reference frame of center of mass current reference frame again)
            
            linear_velocity_change = body->get_linear_velocity();
            angular_velocity_change = body->get_angular_velocity();

            body->compensate_velocities(body->get_linear_velocity(), body->get_angular_velocity());

            // -- For each vertex of model: integrate positions --
            for(int i = 0; i < vertices.size(); ++i)
            {
                vertices[i].integrate_position(dt);
            }

            if( NULL != frame )
            {
                // -- If there is the frame, initial_state should repeat its motion so that frame position cannot change

                // Re-compute frame velocities, changed after the call of body->compensate_velocities
                if( false == frame->compute_velocities() )
                    return false;

                initial_state->set_rigid_motion(*frame);
                
                // For each vertex of initial state: integrate positions
                for(int i = 0; i < initial_vertices.size(); ++i)
                {
                    initial_vertices[i].integrate_position(dt);
                }
            }
            
            // -- Invoke reactions if needed --
            
            for(int i = 0; i < shape_deform_reactions.size(); ++i)
            {
                shape_deform_reactions[i]->invoke_if_needed(*this);
            }

            for(int i = 0; i < region_reactions.size(); ++i)
            {
                region_reactions[i]->invoke_if_needed(*this);
            }

            return true;
        }

        void Model::update_any_vertices(Collections::Array<PhysicalVertex> &src_vertices,
                                        /*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            if(vertices_num > src_vertices.size())
            {
                Logger::warning("in Model::update_any_vertices: requested to update too many vertices, probably wrong vertices given?", __FILE__, __LINE__);
                vertices_num = src_vertices.size();
            }

            void *out_vertex = out_vertices;
            for(int i = 0; i < vertices_num; ++i)
            {
                // TODO: many points and vectors, only position so far
                VertexFloat *position = reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertex, vertex_info.get_point_offset(0)));

                for(int j = 0; j < VECTOR_SIZE; ++j)
                    position[j] = static_cast<VertexFloat>( (src_vertices[i].get_pos())[j] );

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        void Model::update_vertices(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            update_any_vertices(vertices, out_vertices, vertices_num, vertex_info);
        }

        void Model::update_initial_vertices(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            update_any_vertices(initial_vertices, out_vertices, vertices_num, vertex_info);
        }

        Model::~Model()
        {
            delete body;
            delete initial_state;
            delete frame;
        }
    }
}

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

            const int CLUSTERS_NUM[VECTOR_SIZE] = {2, 2, 12}; // !!! hard-coded
            const Real PADDING_COEFF = 0.6; // !!! hard-coded
            const Real MAX_COORDINATE = 1.0e+100; // !!! hard-coded

            inline int compute_cluster_index(const int indices[VECTOR_SIZE], const int clusters_num[VECTOR_SIZE])
            {
                return indices[0] + indices[1]*clusters_num[0] + indices[2]*clusters_num[0]*clusters_num[1];
            }
        }

        Model::Model( const void *source_vertices,
                      int vertices_num,
                      VertexInfo const &vertex_info,
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices_num(vertices_num), clusters_num(0), vertices(NULL), clusters(NULL), total_mass(0)
        {
            if(this->vertices_num < 0)
            {
                logger.error("creating model with `vertices_count` < 0", __FILE__, __LINE__);
                this->vertices_num = 0;
            }
            else
            {
                this->vertices = new PhysicalVertex[this->vertices_num];

                // -- Read vertices --

                Vector min_pos( MAX_COORDINATE, MAX_COORDINATE, MAX_COORDINATE);
                Vector max_pos(-MAX_COORDINATE,-MAX_COORDINATE,-MAX_COORDINATE);

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
                
                // -- Decompose to clusters --

                clusters_num = 1;
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    clusters_num *= CLUSTERS_NUM[i];
                clusters = new Cluster[clusters_num];

                const Vector dimensions = max_pos - min_pos;
                
                Vector cluster_size;
                for(int i = 0; i < VECTOR_SIZE; ++i)
                {
                    if(0 == CLUSTERS_NUM[i])
                        logger.error("creating model with zero CLUSTERS_NUM component", __FILE__, __LINE__);
                    cluster_size[i] = dimensions[i]/CLUSTERS_NUM[i];
                }
                
                const Vector padding = cluster_size*PADDING_COEFF;

                // -- For each vertex --
                for(int i = 0; i < this->vertices_num; ++i)
                {
                    PhysicalVertex &vertex = this->vertices[i];
                    
                    // -- find position, measured off the min_pos --
                    const Vector position = vertex.pos - min_pos;
                    
                    // -- choose a cluster --

                    // "coordinates" of a cluster: axis indices
                    int cluster_indices[VECTOR_SIZE];
                    for(int j = 0; j < VECTOR_SIZE; ++j)
                    {
                        if(0 == cluster_size[j])
                            logger.error("creating model with zero cluster_size component", __FILE__, __LINE__);
                        cluster_indices[j] = static_cast<int>(position[j]/cluster_size[j]);
                        
                        if(cluster_indices[j] < 0)
                            cluster_indices[j] = 0;
                        if(cluster_indices[j] > CLUSTERS_NUM[j] - 1)
                            cluster_indices[j] = CLUSTERS_NUM[j] - 1;
                    }
                    
                    // -- and assign to it --
                    int cluster_index = compute_cluster_index(cluster_indices, CLUSTERS_NUM);
                    clusters[cluster_index].add_vertex(i, vertex);

                    // -- and, probably, to his neighbours --
                    for(int j = 0; j < VECTOR_SIZE; ++j)
                    {
                        // -- previous cluster --
                        
                        if( cluster_indices[j] - 1 >= 0 &&
                            abs(position[j] - cluster_indices[j]*cluster_size[j]) < padding[j] )
                        {
                            --cluster_indices[j];
                            int cluster_index = compute_cluster_index(cluster_indices, CLUSTERS_NUM);
                            clusters[cluster_index].add_vertex(i, vertex);
                            ++cluster_indices[j];
                        }
                        
                        // -- next cluster --
                        
                        if( cluster_indices[j] + 1 <= CLUSTERS_NUM[j] - 1 &&
                            abs((cluster_indices[j] + 1)*cluster_size[j] - position[j]) < padding[j] )
                        {
                            ++cluster_indices[j];
                            int cluster_index = compute_cluster_index(cluster_indices, CLUSTERS_NUM);
                            clusters[cluster_index].add_vertex(i, vertex);
                            --cluster_indices[j];
                        }
                    }
                }
            }
        }

        void Model::compute_next_step(const Force * const forces[], int forces_num)
        {
            if(NULL == forces && 0 != forces_num)
            {
                logger.error("in Model::compute_next_step null pointer `forces`", __FILE__, __LINE__);
                return;
            }
            
            // TODO: QueryPerformanceCounter
            Real dt = 0.01;

            // -- For each cluster of model (and then for each vertex in it) --
            for(int i = 0; i < clusters_num; ++i)
            {
                Cluster &cluster = clusters[i];

                if(0 == cluster.get_vertices_num())
                    continue;
                
                // -- Find current center of mass --

                Vector center_of_mass = Vector::ZERO;
                if( 0 != cluster.get_total_mass() )
                {
                    for(int j = 0; j < cluster.get_vertices_num(); ++j)
                    {
                        PhysicalVertex &vertex = vertices[cluster.get_vertex_index(j)];
                        center_of_mass += vertex.mass*vertex.pos/cluster.get_total_mass();
                    }
                }
                cluster.set_center_of_mass(center_of_mass);
                
                // -- Shape matching: find optimal linear transformation --

                Matrix Apq = Matrix::ZERO;
                Matrix Aqq = Matrix::ZERO;
                for(int j = 0; j < cluster.get_vertices_num(); ++j)
                {
                    PhysicalVertex &vertex = vertices[cluster.get_vertex_index(j)];
                    Vector const &init_pos = cluster.get_initial_vertex_offset_position(j);

                    Apq += vertex.mass*Matrix( vertex.pos - center_of_mass, init_pos );
                    // TODO: re-compute this only on update of plasticity_state
                    Aqq += vertex.mass*Matrix( init_pos, init_pos );
                }
                if( Aqq.is_invertible() )
                {
                    Aqq = Aqq.inverted();
                }
                else
                {
                    logger.warning("in Model::compute_next_step: singular matrix Aqq, unable to compute Aqq.inverted(), assumed it to be Matrix::IDENTITY", __FILE__, __LINE__);
                    Aqq = Matrix::IDENTITY; // TODO: is this good workaround???
                }

                Matrix linear_transformation = Apq*Aqq; // optimal linear transformation
                
                // -- Shape matching: adjust volume --
                
                Real det = linear_transformation.determinant();
                if( ! equal(0, det) )
                {
                    if( det < 0 )
                        logger.warning("in Model::compute_next_step: linear_transformation.determinant() is less than 0, inverted state detected!", __FILE__, __LINE__);
                    linear_transformation /= pow( abs(det), 1.0/3)*sign(det);
                }
                else
                {
                    logger.warning("in Model::compute_next_step: linear_transformation is singular, so volume-preserving constraint cannot be enforced", __FILE__, __LINE__);
                    // but now, while polar decomposition is only for invertible matrix - it's very, very bad...
                }

                // -- Shape matching: retrieve optimal rotation from optimal linear transformation --

                Matrix rotation; // optimal rotation
                Matrix scale;
                Apq.do_polar_decomposition(rotation, scale, 6);

                cluster.set_rotation(rotation);
                
                // -- Shape matching: allow linear deformation by interpolating linear_transformation and rotation --
                
                Real beta = cluster.get_linear_elasticity_constant();
                Matrix total_deformation = beta*rotation + (1 - beta)*linear_transformation;
                
                cluster.set_total_deformation(total_deformation);
                
                // -- Shape matching: finally find goal position and add a correction to velocity --
                
                Vector linear_momentum_addition = Vector::ZERO;
                for(int j = 0; j < cluster.get_vertices_num(); ++j)
                {
                    PhysicalVertex &vertex = vertices[cluster.get_vertex_index(j)];
                    const Vector &init_pos = cluster.get_initial_vertex_offset_position(j);
                    
                    Vector goal_position = total_deformation*init_pos + center_of_mass;
                    // TODO: thread-safe cluster addition: velocity_additions[]...
                    Vector velocity_addition = cluster.get_goal_speed_constant()*(goal_position - vertex.pos)/dt;
                    vertex.velocity_addition += velocity_addition;
                    // TODO: thread-safe cluster addition: velocity_addition_coeffs[]...
                    vertex.velocity_addition_coeff = 1 - cluster.get_damping_constant();
                    
                    linear_momentum_addition += vertex.mass*velocity_addition;
                }
                if( 0 != cluster.get_total_mass() )
                {
                    Vector velocity_correction = - linear_momentum_addition / cluster.get_total_mass();
                    for(int j = 0; j < cluster.get_vertices_num(); ++j)
                    {
                        PhysicalVertex &vertex = vertices[cluster.get_vertex_index(j)];
                        vertex.velocity_addition += velocity_correction;
                    }
                }

                // -- Update plasticity state --
                
                Matrix plasticity_state = cluster.get_plasticity_state();
                        
                Matrix deformation = linear_transformation - Matrix::IDENTITY;
                Real deformation_measure = deformation.norm();
                if(deformation_measure > cluster.get_yield_constant())
                {
                    plasticity_state = (Matrix::IDENTITY + dt*cluster.get_creep_constant()*deformation)*plasticity_state;
                    Math::Real det = plasticity_state.determinant();
                    if( ! equal(0, det) )
                    {
                        plasticity_state /= pow( abs(det), 1.0/3);
                        
                        Matrix plastic_deformation = plasticity_state - Matrix::IDENTITY;
                        Real plastic_deform_measure = plastic_deformation.norm();
                        
                        if( plastic_deform_measure < DEFAULT_MAX_DEFORMATION_CONSTANT ) // !!!
                        {
                            cluster.set_plasticity_state(plasticity_state);
                        }
                    }
                }
            }

            // -- For each vertex of model: integrate velocities --
            for(int i = 0; i < vertices_num; ++i)
            {
                Vector acceleration = Vector::ZERO;
                PhysicalVertex &v = vertices[i];

                if(0 != v.mass && NULL != forces)
                {
                    for(int j=0; j < forces_num; ++j)
                    {
                        if(NULL == forces[j])
                        {
                            logger.error("in Model::compute_next_step: null pointer item of `forces` array ", __FILE__, __LINE__);
                            return;
                        }
                        acceleration += forces[j]->get_value_at(v.pos, v.velocity)/v.mass;
                    }
                }

                if(0 == v.including_clusters_num)
                {
                    logger.error("in Model::compute_next_step: internal error: orphan vertex not belonging to any cluster", __FILE__, __LINE__);
                    return;
                }
                
                // we need average velocity addition, not sum
                v.velocity_addition /= v.including_clusters_num;

                v.velocity += v.velocity_addition + acceleration*dt;
                v.velocity_addition = Vector::ZERO;
            }
            
            // -- Compute macroscopic characteristics --
            Vector center_of_mass = Vector::ZERO;
            Vector center_of_mass_velocity = Vector::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];
                
                center_of_mass += v.mass*v.pos/total_mass;
                center_of_mass_velocity += v.mass*v.velocity/total_mass;
            }
            Vector angular_momentum = Vector::ZERO;
            Matrix inertia_tensor = Matrix::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];
                Vector offset = v.pos - center_of_mass;
                
                angular_momentum += v.mass * cross_product(offset, v.velocity);
                
                inertia_tensor += v.mass * (offset*offset*Matrix::IDENTITY - Matrix(offset, offset));
            }

            if( !inertia_tensor.is_invertible() )
            {
                logger.warning("in Model::compute_next_step: inertia_tensor is singular, assumed it to be Matrix::IDENTITY");
                inertia_tensor = Matrix::IDENTITY;
            }
            Matrix inertia_tensor_inverted = inertia_tensor.inverted();
            Vector angular_velocity = inertia_tensor.inverted()*angular_momentum;

            // -- For each vertex of model: damp velocities and integrate positions --
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = vertices[i];

                Vector rigid_velocity = center_of_mass_velocity + cross_product(angular_velocity, v.pos - center_of_mass);
                Vector deformation_velocity = v.velocity - rigid_velocity;
                v.velocity -= DEFAULT_DAMPING_CONSTANT*deformation_velocity; // !!!

                v.pos += v.velocity*dt;
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

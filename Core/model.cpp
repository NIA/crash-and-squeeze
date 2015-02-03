#include "Core/model.h"

namespace CrashAndSqueeze
{
    using Math::Vector;
    using Math::VECTOR_SIZE;
    using Math::Matrix;
    using Math::Real;
    using Math::equal;
    using Math::less_or_equal;
    using Math::sign;
    using Math::minimum;
    using Logging::Logger;
    using Parallel::IPrimFactory;
    using Parallel::TaskQueue;
    using Parallel::AbstractTask;
    
    namespace Core
    {
        namespace
        {
            // -- useful constants --

            const Real MAX_COORDINATE = 1.0e+100; // !!! hard-coded
            const Vector MAX_COORDINATE_VECTOR(MAX_COORDINATE, MAX_COORDINATE, MAX_COORDINATE);
            const int INITIAL_ALLOCATED_CALLBACK_INFOS = 10;

            // -- helpers --
            template<class T>
            inline void make_fixed_size(Collections::Array<T> &arr, int size)
            {
                arr.create_items(size);
                arr.freeze();
            }

            // TODO: move to regions.cpp?
            // weight func for BoxRegion clusters
            class BoxRegionWeightFunc : public IScalarField
            {
            private:
                BoxRegion * region;
                Vector paddings;
            public:
                BoxRegionWeightFunc(BoxRegion * region, Vector paddings)
                    : region(region), paddings(paddings)
                {}

                Real get_value_at(const Math::Vector &point) const
                {
                    if ( ! region->contains(point) )
                        return 0;
                    int side_index;
                    Real dist = region->get_distance_to_border(point, &side_index);
                    if (dist <= paddings[side_index])
                        return dist/paddings[side_index];
                    else
                        return 1;
                }
            };
        }

        Model::ClusterTask::ClusterTask()
            : cluster(NULL), dt(0) {}

        void Model::ClusterTask::setup(Cluster & cluster, Math::Real & dt, Parallel::IEventSet * event_set, int event_index)
        {
            this->cluster = &cluster;
            this->dt = &dt;
            this->event_set = event_set;
            this->event_index = event_index;
        }
        
        void Model::ClusterTask::execute()
        {
            cluster->match_shape(*dt);
            event_set->set(event_index);
        }

        void Model::FinalTask::execute()
        {
            model->integrate_particle_system();
        }

        // a constant, determining how much deformation velocities are damped:
        // 0 - no damping of vibrations, 1 - maximum damping, rigid body
        const Real Model::DEFAULT_DAMPING_CONSTANT = 0.5*Body::MAX_RIGIDITY_COEFF;

        Model::Model( void *source_physical_vertices,
                      int physical_vetrices_num,
                      VertexInfo const &physical_vertex_info,

                      void *source_graphical_vertices,
                      int graphical_vetrices_num,
                      VertexInfo const &graphical_vertex_info,

                      const int clusters_by_axes[Math::VECTOR_SIZE],
                      Math::Real cluster_padding_coeff,

                      IPrimFactory * prim_factory,
                      
                      const MassFloat *masses,
                      const MassFloat constant_mass)
            : vertices(physical_vetrices_num),
              initial_positions(physical_vetrices_num),
              hit_vertices_indices(physical_vetrices_num),
              
              graphical_vertices(graphical_vetrices_num),

              cluster_padding_coeff(cluster_padding_coeff),

              damping_constant(DEFAULT_DAMPING_CONSTANT),

              min_pos(MAX_COORDINATE_VECTOR),
              max_pos(-MAX_COORDINATE_VECTOR),

              body(NULL),
              frame(NULL),

              cluster_regions(NULL),
              null_cluster_index(0),

              velocities_changed_callback(NULL),

              prim_factory(prim_factory),
              cluster_tasks_completed(NULL),
              cluster_tasks(NULL),
#pragma warning( push )
#pragma warning( disable : 4355 )
              final_task(this),
#pragma warning( pop )
              task_queue(NULL),
              step_completed(NULL),
              tasks_ready(NULL),
              success(true)
        {
            // -- Finish initialization of arrays --
            // -- (create enought items and freeze or just forbid reallocations) --
            
            make_fixed_size(vertices,          physical_vetrices_num);
            make_fixed_size(initial_positions, physical_vetrices_num);
            
            make_fixed_size(graphical_vertices, graphical_vetrices_num);

            hit_vertices_indices.forbid_reallocation(vertices.size());

            // Init physical vertices
            if( false != init_physical_vertices(source_physical_vertices, physical_vertex_info, masses, constant_mass) )
            {
                // Init graphical vertices
                init_graphical_vertices(source_graphical_vertices, graphical_vertex_info);

                // Init clusters
                for(int i = 0; i < VECTOR_SIZE; ++i)
                    this->clusters_by_axes[i] = clusters_by_axes[i];

                if( false != create_auto_cluster_regions() )
                {
                    if( false != init_clusters() )
                    {
                        // Update cluster indices for graphical vertices
                        update_cluster_indices(source_graphical_vertices, graphical_vetrices_num, graphical_vertices, graphical_vertex_info);
                        update_cluster_indices(source_physical_vertices, physical_vetrices_num, vertices, physical_vertex_info);

                        init_tasks();
                    }
                }
            }
        }

        bool Model::init_physical_vertices(const void *source_vertices,
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
                const VertexFloat *src_position = reinterpret_cast<const VertexFloat*>( add_to_pointer(source_vertex, vertex_info.get_point_offset(0)));
                
                Vector pos;
                VertexInfo::vertex_floats_to_vector(src_position, pos);
                
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
                
                vertices[i] = PhysicalVertex(pos, mass);
                initial_positions[i] = pos;
                
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
            return true;
        }
        
        void Model::init_graphical_vertices(const void *source_vertices,
                                            VertexInfo const &vertex_info)
        {
            const void * source_graphical_vertex = source_vertices;
            for(int i = 0; i < graphical_vertices.size(); ++i)
            {
                graphical_vertices[i] = GraphicalVertex(vertex_info, source_graphical_vertex);
                
                source_graphical_vertex = add_to_pointer(source_graphical_vertex, vertex_info.get_vertex_size());
            }
        }

        bool Model::create_auto_cluster_regions()
        {
            int clusters_num = 1;
            for(int i = 0; i < VECTOR_SIZE; ++i)
                clusters_num *= clusters_by_axes[i];

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

            cluster_regions = new RegionsArray(clusters_num);
            cluster_weight_funcs = new WeightFuncsArray(clusters_num);

            const Vector padding = cluster_sizes*cluster_padding_coeff;

            for(int ix = 0; ix < clusters_by_axes[0]; ++ix)
            {
                for(int iy = 0; iy < clusters_by_axes[1]; ++iy)
                {
                    for(int iz = 0; iz < clusters_by_axes[2]; ++iz)
                    {
                        Vector min_corner = min_pos
                                          + Vector(ix*cluster_sizes[0],
                                                   iy*cluster_sizes[1],
                                                   iz*cluster_sizes[2])
                                          - padding;
                        Vector max_corner = min_corner + cluster_sizes + 2*padding;
                        BoxRegion * region = new BoxRegion(min_corner, max_corner);
                        cluster_regions->push_back( region );
                        cluster_weight_funcs->push_back( new BoxRegionWeightFunc(region, padding) );
                    }
                }
            }
            return true;
        }

        bool Model::init_clusters()
        {
            int clusters_num = cluster_regions->size();

            // after last cluster
            null_cluster_index = static_cast<ClusterIndex>(clusters_num);

            clusters.create_items(clusters_num);
            clusters.freeze();

            Collections::Array<Cluster *> found_clusters;

            // -- For each vertex: assign --
            for(int i = 0; i < vertices.size(); ++i)
            {
                if ( false == find_clusters_for_vertex(vertices[i], found_clusters) )
                    return false;

                for(int j = 0; j < found_clusters.size(); ++j)
                {
                    found_clusters[j]->add_physical_vertex(vertices[i]);
                }
            }

            // -- For each graphical vertex: assign --
            for(int i = 0; i < graphical_vertices.size(); ++i)
            {
                if( false == find_clusters_for_vertex(graphical_vertices[i], found_clusters) )
                    return false;

                for(int j = 0; j < found_clusters.size(); ++j)
                {
                    found_clusters[j]->add_graphical_vertex(graphical_vertices[i]);
                }

                graphical_vertices[i].normalize_weights();
            }

            // -- For each cluster: precompute --
            for(int i = 0; i < clusters.size(); ++i)
            {
                clusters[i].compute_initial_characteristics();
                clusters[i].log_properties(i);
            }

            return true;
        }
        bool Model::find_clusters_for_vertex(IVertex &vertex, /*out*/ Collections::Array<Cluster *> & found_clusters)
        {
            if(0 == cluster_regions->size())
                return false;

            found_clusters.clear();

            // While looking for cluster containing this vertex,find the one
            // with minimal distance (it will be used, if none containing found)
            Real min_distance = Math::MAX_REAL;
            int min_distance_index = -1;
            for(int i = 0; i < cluster_regions->size(); ++i)
            {
                const IRegion * region = cluster_regions->get(i);
                if( region->contains(vertex.get_pos()) )
                {
                    Real weight = cluster_weight_funcs->get(i)->get_value_at(vertex.get_pos());
                    vertex.include_to_one_more_cluster(i, weight);
                    found_clusters.push_back( &clusters[i] );
                }
                else
                {
                    double dst = distance( vertex.get_pos(), region->get_center() );
                    if(dst < min_distance)
                    {
                        min_distance = dst;
                        min_distance_index = i;
                    }
                }
            }

            // If not found good cluster...
            if(0 == vertex.get_including_clusters_num())
            {
                vertex.include_to_one_more_cluster(min_distance_index, 1);
                found_clusters.push_back( &clusters[min_distance_index] );
            }

            return true;
        }

        void Model::init_tasks()
        {
            int clusters_num = clusters.size();
            cluster_tasks = new ClusterTask[clusters_num];
            task_queue = new TaskQueue(clusters_num + 1, prim_factory);
            cluster_tasks_completed = prim_factory->create_event_set(clusters_num, true);
            step_completed = prim_factory->create_event(true);
            tasks_ready = prim_factory->create_event(false);
            for(int i = 0; i < clusters_num; ++i)
            {
                cluster_tasks[i].setup(clusters[i], dt, cluster_tasks_completed, i);
                task_queue->push(&cluster_tasks[i]);
            }
            task_queue->push(&final_task);
        }

        Vector Model::get_vertex_initial_pos(int index) const
        {
            return initial_positions[index];
        }

        // returns equilibrium position, moved so that to match immovable initial positions
        Vector Model::get_vertex_equilibrium_pos(int index) const
        {
            return relative_to_frame.get_orientation() * (vertices[index].get_equilibrium_pos() + relative_to_frame.get_position());
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

            // -- Find vertices in this region --

            for(int i = 0; i < vertices.size(); ++i)
            {
                PhysicalVertex &v = vertices[i];

                if( region.contains(v.get_pos()) )
                    hit_vertices_indices.push_back(i);
            }

            // -- Report warning if none found --

            if( 0 == hit_vertices_indices.size() )
            {
                Logger::warning("in Model::hit: there is no vertex inside given region", __FILE__, __LINE__);
                return;
            }

            // -- Add the same velocity to all vertices in region --

            for(int i = 0; i < hit_vertices_indices.size(); ++i)
            {
                vertices[ hit_vertices_indices[i] ].add_to_velocity(velocity);
            }

            // -- Invoke reactions if needed --

            for(int i = 0; i < hit_reactions.size(); ++i)
            {
                hit_reactions[i]->invoke_if_needed(hit_vertices_indices, velocity);
            }
        }

        void Model::prepare_tasks(const ForcesArray & forces, Math::Real dt, VelocitiesChangedCallback * vcb)
        {
            // wait for previous step to complete
            step_completed->wait();
            
            // reset events
            cluster_tasks_completed->unset();
            step_completed->unset();
            
            // store parameters of this step
            this->dt = dt;
            this->forces = &forces;
            this->velocities_changed_callback = vcb;
            
            // reset queue
            if(task_queue->is_empty())
            {
                task_queue->reset();
            }
            
            // start new step
            tasks_ready->set();
        }

        bool Model::complete_next_task()
        {
            AbstractTask *task;
            if( NULL != (task = task_queue->pop()) )
            {
                task->complete();
                return true;
            }
            else
            {
                return false;
            }
        }

        bool Model::wait_for_clusters()
        {
            cluster_tasks_completed->wait();
            return success;
        }

        bool Model::wait_for_step()
        {
            step_completed->wait();
            return success;
        }

        void Model::abort()
        {
            success = false;
            task_queue->clear();
            cluster_tasks_completed->set();
            step_completed->set();
        }

        void Model::integrate_particle_system()
        {
            // this is last task, so unset event to make threads wait till new step
            tasks_ready->unset();
            cluster_tasks_completed->wait();

            // TODO: place it somewhere more logical
            shape_deform_reactions.freeze();

            // Re-compute properties (due to changed positions of vertices)
            if( false == body->compute_properties() )
                return;

            // -- Force linear and angular momenta conservation --

            if( false == body->compute_velocity_additions() )
                return;

            body->compensate_velocities( body->get_linear_velocity_addition(),
                                         body->get_angular_velocity_addition() );

            // -- For each vertex of model: integrate velocities: sum velocity additions and apply forces --
            for(int i = 0; i < vertices.size(); ++i)
            {
                if( false == vertices[i].integrate_velocity( *forces, dt ) )
                    return;
            }

            if( NULL != frame )
            {
                // -- Ensure that the frame moves as a rigid body --

                if( false == frame->compute_properties() )
                    return;
                if( false == frame->compute_velocities() )
                    return;

                frame->set_rigid_motion();
            }

            // Find macroscopic motion for damping and subsequent substraction
            if( false == body->compute_velocities() )
                return;

            // Damp deformation oscillations
            body->set_rigid_motion(damping_constant);

            // -- Substract macroscopic motion of body --
            // (to make the reference frame of center of mass current reference frame again)

            if(NULL != velocities_changed_callback)
            {
                velocities_changed_callback->invoke( body->get_linear_velocity(),
                                                     body->get_angular_velocity() );
            }

            body->compensate_velocities(body->get_linear_velocity(), body->get_angular_velocity());

            // -- For each vertex of model: integrate positions --
            for(int i = 0; i < vertices.size(); ++i)
            {
                vertices[i].integrate_position(dt);
            }

            if( NULL != frame )
            {
                // -- If there is the frame, relative_to_frame should repeat its inverted motion --

                // Re-compute frame velocities, changed after the call of body->compensate_velocities
                if( false == frame->compute_properties() )
                    return;
                if( false == frame->compute_velocities() )
                    return;

                relative_to_frame.set_linear_velocity( - frame->get_linear_velocity() );
                relative_to_frame.set_angular_velocity( - frame->get_angular_velocity() );
                relative_to_frame.integrate(dt);
            }
            
            step_completed->set();
        }

        // TODO: is this function thread-safe? Reading matrix from RigidBody is not atomic.
        // Should use locks for access to RigidBody methods?
        void Model::react_to_events()
        {
            // -- Invoke reactions if needed --

            for(int i = 0; i < shape_deform_reactions.size(); ++i)
            {
                shape_deform_reactions[i]->invoke_if_needed(*this);
            }

            for(int i = 0; i < region_reactions.size(); ++i)
            {
                region_reactions[i]->invoke_if_needed(*this);
            }

            for(int i = 0; i < stretch_reactions.size(); ++i)
            {
                stretch_reactions[i]->invoke_if_needed(*this);
            }
        }

        const Matrix & Model::get_cluster_transformation(int cluster_index) const
        {
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
            // TODO: returns only linear part!
            return clusters[cluster_index].get_graphical_pos_transform().to_matrix();
#else
            return clusters[cluster_index].get_graphical_pos_transform();
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
        }

        const Matrix & Model::get_cluster_normal_transformation(int cluster_index) const
        {
            return clusters[cluster_index].get_graphical_nrm_transform();
        }

        const Vector & Model::get_cluster_center(int cluster_index) const
        {
            return clusters[cluster_index].get_center_of_mass();
        }

        const Vector & Model::get_cluster_initial_center(int cluster_index) const
        {
            return clusters[cluster_index].get_initial_center_of_mass();
        }

        void Model::update_any_positions(PositionFunc pos_func, /*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            if(vertices_num != vertices.size())
            {
                Logger::warning("in Model::update_any_positions: requested to update wrong number of physical vertices, probably wrong vertices given?", __FILE__, __LINE__);
                vertices_num = minimum(vertices_num, vertices.size());
            }

            void *out_vertex = out_vertices;
            for(int i = 0; i < vertices_num; ++i)
            {
                VertexFloat *out_position =
                    reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertex, vertex_info.get_point_offset(0)) );

                const Vector src_position = (this->*pos_func)(i);

                VertexInfo::vector_to_vertex_floats(src_position, out_position);

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        template <class VertexType /*: public IVertex*/>
        void Model::update_cluster_indices(/*out*/ void *out_vertices, int vertices_num, const Collections::Array<VertexType> & src_vertices, const VertexInfo &vertex_info)
        {
            if(vertices_num != src_vertices.size())
            {
                Logger::warning("in Model::update_vertices: requested to update wrong number of vertices, probably wrong vertices given?", __FILE__, __LINE__);
                vertices_num = minimum(vertices_num, src_vertices.size());
            }
            void *out_vertex = out_vertices;
            for(int i = 0; i < vertices_num; ++i)
            {
                int including_clusters_num = src_vertices[i].get_including_clusters_num();

                ClusterIndex *out_cluster_indices =
                    reinterpret_cast<ClusterIndex*>( add_to_pointer(out_vertex, vertex_info.get_cluster_indices_offset()) );
                for(int j = 0; j < VertexInfo::CLUSTER_INDICES_NUM; ++j)
                {
                    if(j < including_clusters_num)
                        out_cluster_indices[j] = src_vertices[i].get_including_cluster_index(j);
                    else
                        out_cluster_indices[j] = null_cluster_index;
                }
                int *out_clusters_num =
                    reinterpret_cast<int*>(add_to_pointer(out_vertex, vertex_info.get_clusters_num_offset()));
                *out_clusters_num = including_clusters_num;

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        void Model::update_vertices(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            if(vertices_num != graphical_vertices.size())
            {
                Logger::warning("in Model::update_vertices: requested to update wrong number of graphical vertices, probably wrong vertices given?", __FILE__, __LINE__);
                vertices_num = minimum(vertices_num, graphical_vertices.size());
            }
            if(vertex_info.get_points_num() > graphical_vertices[0].get_points_num())
            {
                Logger::error("in Model::update_vertices: vertex_info incompatible with that was used for initialization: too many points per vertex requested", __FILE__, __LINE__);
                return;
            }
            if(vertex_info.get_vectors_num() > graphical_vertices[0].get_vectors_num())
            {
                Logger::error("in Model::update_vertices: vertex_info incompatible with that was used for initialization: too many vectors per vertex requested", __FILE__, __LINE__);
                return;
            }

            void *out_vertex = out_vertices;
            for(int i = 0; i < vertices_num; ++i)
            {
                const GraphicalVertex & vertex = graphical_vertices[i];
                int clusters_num = vertex.get_including_clusters_num();
                if(0 == clusters_num)
                {
                    Logger::error("GraphicalVertex doesn't belong to any cluster", __FILE__, __LINE__);
                    return;
                }

                for(int j = 0; j < vertex_info.get_points_num(); ++j)
                {
                    Vector new_point = Vector::ZERO;
                    for( int k = 0; k < clusters_num; ++k)
                    {
                        const Cluster & cluster = clusters[vertex.get_including_cluster_index(k)];
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                        new_point += (cluster.get_graphical_pos_transform() * Math::TriVector(vertex.get_point(j) - cluster.get_initial_center_of_mass())
                                   + cluster.get_center_of_mass()) * vertex.get_cluster_weight(k);
#else
                        new_point += (cluster.get_graphical_pos_transform() * (vertex.get_point(j) - cluster.get_initial_center_of_mass())
                                   + cluster.get_center_of_mass()) * vertex.get_cluster_weight(k);
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
                    }

                    VertexFloat *destination =
                        reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertex, vertex_info.get_point_offset(j)) );

                    VertexInfo::vector_to_vertex_floats(new_point, destination);
                }

                for(int j = 0; j < vertex_info.get_vectors_num(); ++j)
                {
                    Vector new_vector = Vector::ZERO;
                    for( int k = 0; k < clusters_num; ++k)
                    {
                        const Cluster & cluster = clusters[vertex.get_including_cluster_index(k)];
#if CAS_QUADRATIC_EXTENSIONS_ENABLED
                         // TODO: use quadratic transformation for vectors too (but how?)
                        if(vertex.is_vector_orthogonal(j))
                            new_vector += cluster.get_graphical_nrm_transform() * vertex.get_vector(j) * vertex.get_cluster_weight(k);
                        else
                            new_vector += cluster.get_graphical_pos_transform().to_matrix() * vertex.get_vector(j) * vertex.get_cluster_weight(k);
#else
                        if(vertex.is_vector_orthogonal(j))
                            new_vector += cluster.get_graphical_nrm_transform() * vertex.get_vector(j) * vertex.get_cluster_weight(k);
                        else
                            new_vector += cluster.get_graphical_pos_transform() * vertex.get_vector(j) * vertex.get_cluster_weight(k);
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED
                    }

                    VertexFloat *destination =
                        reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertex, vertex_info.get_vector_offset(j)) );

                    VertexInfo::vector_to_vertex_floats(new_vector, destination);
                }

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        void Model::update_current_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            update_any_positions(&Model::get_vertex_current_pos, out_vertices, vertices_num, vertex_info);
        }

        void Model::update_initial_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            update_any_positions(&Model::get_vertex_initial_pos, out_vertices, vertices_num, vertex_info);
        }

        void Model::update_equilibrium_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info)
        {
            update_any_positions(&Model::get_vertex_equilibrium_pos, out_vertices, vertices_num, vertex_info);
        }

        Model::~Model()
        {
            delete body;
            delete frame;

            delete[] cluster_tasks;
            prim_factory->destroy_event_set(cluster_tasks_completed);
            prim_factory->destroy_event(step_completed);
            prim_factory->destroy_event(tasks_ready);
            delete task_queue;
        }
    }
}

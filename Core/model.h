#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Core/imodel.h"
#include "Core/cluster.h"
#include "Core/force.h"
#include "Core/reactions.h"
#include "Core/body.h"
#include "Core/regions.h"
#include "Core/rigid_body.h"
#include "Core/graphical_vertex.h"
#include "Core/simulation_params.h"
#include "Math/floating_point.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Collections/array.h"
#include "Parallel/abstract_task.h"
#include "Parallel/task_queue.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class VelocitiesChangedCallback
        {
        public:
            virtual void invoke(const Math::Vector &linear_velocity_change, const Math::Vector &angular_velocity_change) = 0;
        };

        typedef Collections::Array<IRegion*> RegionsArray;
        typedef Collections::Array<IScalarField*> WeightFuncsArray;

        class Model : public IModel
        {
        private:
            // -- constant (at run-time) properties
            
            Collections::Array<PhysicalVertex> vertices;
            Collections::Array<GraphicalVertex> graphical_vertices;
            Collections::Array<Cluster> clusters;

            // TODO: is it rational to store ALL position if in used only to compare with a few reactons?
            // Probably better to store only needed ones in Reaction class?
            Collections::Array<Math::Vector> initial_positions;

            // TODO: do we need separate arrays for all these types? Some of them are invoked identically
            Collections::Array<ShapeDeformationReaction *> shape_deform_reactions;
            Collections::Array<RegionReaction *> region_reactions;
            Collections::Array<HitReaction *> hit_reactions;
            Collections::Array<StretchReaction*> stretch_reactions;

            // -- fields used in initialization --
            int clusters_by_axes[Math::VECTOR_SIZE];
            Math::Real cluster_padding_coeff;
            // TODO: check for memory leaks from such not deleted arrays like cluster_regions
            RegionsArray * cluster_regions;
            WeightFuncsArray * cluster_weight_funcs;
            // index of zero cluster matrix (normally it is equal to clusters_num because this zero matrix is placed after the last cluster matrix)
            ClusterIndex null_cluster_index;
            
            // minimum values of coordinates of vertices
            Math::Vector min_pos;
            // maximum values of coordinates of vertices
            Math::Vector max_pos;
            // dimensions of a cluster
            Math::Vector cluster_sizes;
            // all-model center of mass
            Math::Vector center_of_mass;
            
            // a constant, determining how much deformation velocities are damped:
            // 0 - no damping of vibrations, 1 - maximum damping, rigid body
            Math::Real damping_constant;

            // -- initialization steps: all return false on failure --
            
            bool init_physical_vertices(const void *source_vertices,
                                        const VertexInfo &vertex_info,
                                        const MassFloat *masses,
                                        const MassFloat constant_mass);
            
            void init_graphical_vertices(const void *source_vertices,
                                         const VertexInfo &vertex_info);
            
            bool create_auto_cluster_regions();

            bool init_clusters();

            template <class VertexType /*: public IVertex*/>
            void update_cluster_indices(/*out*/ void *out_vertices, int vertices_num, const Collections::Array<VertexType> & src_vertices, const VertexInfo &vertex_info);
            
            bool find_clusters_for_vertex(IVertex &vertex, /*out*/ Collections::Array<Cluster *> & found_clusters);

            void init_tasks();

            // -- fields used in step computation --

            class ClusterTask : public Parallel::AbstractTask
            {
            private:
                Cluster *cluster;
                Math::Real *dt;
                Parallel::IEventSet * event_set;
                int event_index;
            protected:
                // implement AbstractTask
                virtual void execute();
            public:
                ClusterTask();
                void setup(Cluster & cluster, Math::Real & dt, Parallel::IEventSet * event_set, int event_index);
            } *cluster_tasks;

            class FinalTask : public Parallel::AbstractTask
            {
            private:
                Model *model;
            protected:
                // implement AbstractTask
                virtual void execute();
            public:
                FinalTask(Model *model) : model(model) {}
            } final_task;
            
            Parallel::IPrimFactory * prim_factory;
            Parallel::IEventSet * cluster_tasks_completed;
            Parallel::IEvent * step_completed;
            Parallel::IEvent * tasks_ready;

            Parallel::TaskQueue * task_queue;

            volatile bool success;

            // current step parameters
            Math::Real dt;
            const ForcesArray * forces;
            VelocitiesChangedCallback * velocities_changed_callback;
            // entire model as a body
            Body *body;
            // rigid frame
            Body *frame;
            // describes motion of model relative to the frame (inverted motion of frame relative to model)
            RigidBody relative_to_frame;

            // used by Model::hit: here placed are indices of the found vertices (which are inside the given region)
            IndexArray hit_vertices_indices;

            // -- step computation steps --
            void integrate_particle_system();

            bool correct_velocity_additions();

            typedef Math::Vector (Model::*PositionFunc)(int index) const;
            void update_any_positions(PositionFunc pos_func, /*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);

        public:
            // Takes a pointer source_vertices to vetrices_num vertices of arbitrary
            // strucutre, described by vertex_info. Takes a pointer masses to vertices_num
            // values of mass. If masses are equal for all vertices, it can be null
            // and the mass should be given as constant_mass argument.
            // The model must have rigid frame, defined by frame_indices array of indices
            // of frame vertices.
            // TODO: the fact that cluster_padding_coeff defines only half of overlapping area is not obvious.
            Model(void *source_physical_vertices,
                  int physical_vetrices_num,
                  VertexInfo const &physical_vertex_info,

                  void *source_graphical_vertices,
                  int graphical_vetrices_num,
                  VertexInfo const &graphical_vertex_info,

                  const int clusters_by_axes[Math::VECTOR_SIZE],
                  Math::Real cluster_padding_coeff,

                  Parallel::IPrimFactory * prim_factory,
                  
                  const MassFloat *masses,
                  const MassFloat constant_mass = 0);

            // -- Initial configuration --
            
            void set_frame(const IndexArray &frame_indices);

            // -- Reactions --
            
            void add_hit_reaction(HitReaction & reaction) { hit_reactions.push_back(&reaction); }
            void add_region_reaction(RegionReaction & reaction) { region_reactions.push_back(&reaction); }
            void add_shape_deformation_reaction(ShapeDeformationReaction & reaction) { shape_deform_reactions.push_back(&reaction); }
            void add_stretch_reaction(StretchReaction &reaction) { stretch_reactions.push_back(&reaction); }

            // -- Run-time simulation interface --
            
            // Sets current simulation params given by `params`. Cluster params are set equally for each cluster.
            void set_simulation_params(const SimulationParams &params);

            // Gets current simulation params into `params`. Clusters params are simply taken from the first cluster as if they were all equal,
            // so setting them back with `set_simulation_params` may change simulation if they were actually different.
            void get_simulation_params(SimulationParams /*out*/ &params) const;

            // Applies addition of `velocity' to model's velocity. The argument `region'
            // specifies the region being directly hit: at the first step momentum addition
            // is distributed between vertices inside this region.
            //
            // TODO: should it be done _after_ preparing tasks, i.e. in parallel with clusters computing?
            // Should then use a lock to avoid race condition between this and last task? (unlikely, because it
            // will probably finish before, but possible)
            void hit(const IRegion &region, const Math::Vector & velocity);
            
            // Prepares tasks for next step. If there are some tasks from previous step left,
            // this function will wait for them to complete. Returns false on failure.
            //
            // Computation of next step will be in local coordinates of body (coordinate system is
            // bound to body frame if there is any, otherwise - to the center of mass). After that
            // the change of global motion is returned via `linear_velocity_change' and `angular_velocity_change'
            void prepare_tasks(const ForcesArray & forces, Math::Real dt, VelocitiesChangedCallback * vcb);

            // In order to compute next step, a queue of tasks must be prepaired first
            // with Model::prepare_tasks, and then Model::complete_next_task should be called multiple
            // times until it returns false (meaning no tasks for this step left)
            //
            // This function is thread-safe and can be called from different threads simultaneously.
            bool complete_next_task();

            // wait until the tasks for next step are ready
            void wait_for_tasks() { tasks_ready->wait(); }

            // Wait until all cluster computation tasks are complete.
            // Returns true if computation was successful, false otherwise
            bool wait_for_clusters();

            // wait until all tasks for current step are completed
            // Returns true if computation was successful, false otherwise
            bool wait_for_step();

            // Abort computation in onther threads by emtying task queue and releasing waiting threads
            void abort();

            // detect happened evetns and invoke reactions, if needed
            void react_to_events();

            // get cluster parameters for computation on GPU
            const Math::Matrix & get_cluster_transformation(int cluster_index) const;
            const Math::Matrix & get_cluster_normal_transformation(int cluster_index) const;
            const Math::Vector & get_cluster_initial_center(int cluster_index) const;
            const Math::Vector & get_cluster_center(int cluster_index) const;

            // TODO: remove or implement proper non-gpu computation
            void update_vertices(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            
            // These methods are needed only for test demo
            void update_current_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            void update_equilibrium_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            void update_initial_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);

            // -- Properties --
            
            virtual int get_vertices_num() const { return vertices.size(); }
            int get_clusters_num() const { return clusters.size(); }
            
            const PhysicalVertex & get_vertex(int index) const { return vertices[index]; }
            const Cluster & get_cluster(int index) const { return clusters[index]; }

            const Math::Vector & get_center_of_mass() { return center_of_mass; }
            
            // -- Implement IModel --

            // returns equilibrium position, moved so that to match immovable initial positions
            virtual Math::Vector get_vertex_equilibrium_pos(int index) const;
            virtual Math::Vector get_vertex_initial_pos(int index) const;
            
            Math::Vector get_vertex_current_pos(int index) const { return vertices[index].get_pos(); }

            virtual ~Model();

            // -- static methods --

            static const Math::Real DEFAULT_DAMPING_CONSTANT;
        private:
            // No copying!
            Model(const Model &);
            Model & operator=(const Model &);
        };
    }
}

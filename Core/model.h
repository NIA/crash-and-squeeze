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
        class Model : public IModel
        {
        private:
            // -- constant (at run-time) properties
            
            Collections::Array<PhysicalVertex> vertices;
            Collections::Array<GraphicalVertex> graphical_vertices;
            Collections::Array<Cluster> clusters;

            Collections::Array<Math::Vector> initial_positions;

            Collections::Array<ShapeDeformationReaction *> shape_deform_reactions;
            Collections::Array<RegionReaction *> region_reactions;
            Collections::Array<HitReaction *> hit_reactions;

            // -- fields used in initialization --
            int clusters_by_axes[Math::VECTOR_SIZE];
            Math::Real cluster_padding_coeff;
            // index of zero cluster matrix
            ClusterIndex null_cluster_index;
            
            // minimum values of coordinates of vertices
            Math::Vector min_pos;
            // maximum values of coordinates of vertices
            Math::Vector max_pos;
            // dimensions of a cluster
            Math::Vector cluster_sizes;
            
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
            
            bool init_clusters();
            void update_cluster_indices(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            
            bool get_nearest_cluster_indices(const Math::Vector position, /*out*/ int cluster_indices[Math::VECTOR_SIZE]);
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
            
            Parallel::IPrimFactory * prim_factory;
            Parallel::IEventSet * tasks_completed;

            Parallel::TaskQueue * task_queue;

            Math::Real dt;
            // entire model as a body
            Body *body;
            // rigid frame
            Body *frame;
            // describes motion of model relative to the frame (inverted motion of frame relative to model)
            RigidBody relative_to_frame;

            // used by Model::hit: here placed are indices of the found vertices (which are inside the given region)
            IndexArray hit_vertices_indices;

            // -- step computation steps --
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
            Model(const void *source_physical_vertices,
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

            // -- Run-time emulation interface --
            
            // Applies addition of `velocity' to model's velocity. The argument `region'
            // specifies the region being directly hit: at the first step momentum addition
            // is distributed between vertices inside this region.
            void hit(const IRegion &region, const Math::Vector & velocity);
            
            // Prepares tasks for next step
            Parallel::TaskQueue * prepare_tasks(Math::Real dt);
            // In order to compute next step, a queue of tasks must be retrieved first
            // from Model::prepare_tasks, and only then Model::compute_next_step should be called.
            // The wait for the tasks to complete is performed inside this Model::compute_next_step.
            //
            // This method computes next step in local coordinates of body (coordinate system is
            // bound to body frame if there is any, otherwise - to the center of mass), and sets
            // `linear_velocity_change' and `angular_velocity_change': the change of global motion
            bool compute_next_step(const ForcesArray & forces, Math::Real dt,
                                   /*out*/ Math::Vector & linear_velocity_change,
                                   /*out*/ Math::Vector & angular_velocity_change);

            Math::Matrix get_cluster_transformation(int cluster_index) const;
            Math::Matrix get_cluster_normal_transformation(int cluster_index) const;
            const Math::Vector & get_cluster_initial_center(int cluster_index) const;
            const Math::Vector & get_cluster_center(int cluster_index) const;

            void update_vertices(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            
            void update_current_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            void update_equilibrium_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);
            void update_initial_positions(/*out*/ void *out_vertices, int vertices_num, const VertexInfo &vertex_info);

            // -- Properties --
            
            virtual int get_vertices_num() const { return vertices.size(); }
            int get_clusters_num() const { return clusters.size(); }
            
            const PhysicalVertex & get_vertex(int index) const { return vertices[index]; }
            const Cluster & get_cluster(int index) const { return clusters[index]; }
            
            // -- Implement IModel --

            // returns equilibrium position, moved so that to match immovable initial positions
            virtual Math::Vector get_vertex_equilibrium_pos(int index) const;
            virtual Math::Vector get_vertex_initial_pos(int index) const;
            
            Math::Vector get_vertex_current_pos(int index) const { return vertices[index].get_pos(); }

            virtual ~Model();

            // -- static methods --

            static const Math::Real DEFAULT_DAMPING_CONSTANT;
            
            // some internal helpers, public just to be able to test them
            static int axis_indices_to_index(const int indices[Math::VECTOR_SIZE],
                                             const int clusters_by_axes[Math::VECTOR_SIZE]);
            static void index_to_axis_indices(int index,
                                              const int clusters_by_axes[Math::VECTOR_SIZE],
                                              /* out */ int indices[Math::VECTOR_SIZE]);
        private:
            // No copying!
            Model(const Model &);
            Model & operator=(const Model &);
        };
    }
}

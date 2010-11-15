#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Core/imodel.h"
#include "Core/force.h"
#include "Core/reactions.h"
#include "Core/body.h"
#include "Math/floating_point.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class Model : public IModel
        {
        private:
            // -- constant (at run-time) properties
            
            Collections::Array<PhysicalVertex> vertices;
            Collections::Array<Cluster> clusters;

            ShapeDeformationReactions shape_deform_reactions;

            // -- fields used in initialization --
            int clusters_by_axes[Math::VECTOR_SIZE];
            Math::Real cluster_padding_coeff;
            
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
            
            bool init_vertices(const void *source_vertices,
                               const VertexInfo &vertex_info,
                               const MassFloat *masses,
                               const MassFloat constant_mass);
            
            bool init_clusters();
            
            bool get_nearest_cluster_indices(const Math::Vector position, /*out*/ int cluster_indices[Math::VECTOR_SIZE]);
            bool add_vertex_to_clusters(PhysicalVertex &vertex);

            // -- fields used in step computation --

            // entire model as a body
            Body *body;
            // rigid frame
            Body *frame;

            // -- step computation steps --
            bool correct_velocity_additions();

            // TODO: DisplayVertex display_vertices; int display_vertices_num;
        public:
            // Takes a pointer source_vertices to vetrices_num vertices of arbitrary
            // strucutre, described by vertex_info. Takes a pointer masses to vertices_num
            // values of mass. If masses are equal for all vertices, it can be null
            // and the mass should be given as constant_mass argument.
            // The model must have rigid frame, defined by frame_indices array of indices
            // of frame vertices.
            // TODO: low-/hi-polygonal meshes
            Model(const void *source_vertices,
                  int vetrices_num,
                  VertexInfo const &vertex_info,
                  const int clusters_by_axes[Math::VECTOR_SIZE],
                  Math::Real cluster_padding_coeff,
                  const MassFloat *masses,
                  const MassFloat constant_mass = 0);

            // -- Initial configuration --
            
            void set_frame(const IndexArray &frame_indices);

            void add_shape_deformation_reaction(ShapeDeformationReaction & reaction);

            // -- Run-time emulation interface --
            
            // computes next step in local coordinates of body (coordinate system is bound to body frame),
            // sets `linear_velocity_change' and `angular_velocity_change': the change of global motion
            bool compute_next_step(const ForcesArray & forces,
                                   /*out*/ Math::Vector & linear_velocity_change,
                                   /*out*/ Math::Vector & angular_velocity_change);

            void update_vertices(/*out*/ void *vertices, int vertices_num, VertexInfo const &vertex_info);

            // -- Properties --
            
            virtual int get_vertices_num() const { return vertices.size(); }
            virtual int get_clusters_num() const { return clusters.size(); }
            
            virtual const PhysicalVertex & get_vertex(int index) const { return vertices[index]; }
            virtual const Cluster & get_cluster(int index) const { return clusters[index]; }
            
            virtual ~Model();

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

#pragma once
#include "Core/core.h"
#include "Core/force.h"
#include "Math/vector.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class PhysicalVertex
        {
        private:
            // position of vertex in global coordinate system
            Math::Vector pos;
            Math::Real mass;
            Math::Vector velocity;
            // Number of clusters which include current vertex
            // TODO: needed only for ShapeDeformationReaction, might be removed when ShapeDeformationReaction removed
            int including_clusters_num;
            int nearest_cluster_index;
            // TODO: thread-safe cluster addition: velocity_additions[]...
            Math::Vector velocity_addition;
            
        public:
            static const int NOT_IN_A_CLUSTER = -2;

            PhysicalVertex( Math::Vector pos,
                            Math::Real mass,
                            Math::Vector velocity = Math::Vector(0,0,0) )
                : pos(pos), mass(mass), velocity(velocity), velocity_addition(Math::Vector::ZERO),
                  including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER) {}
            
            PhysicalVertex()
                : mass(0), pos(Math::Vector::ZERO), velocity(Math::Vector::ZERO), velocity_addition(Math::Vector::ZERO),
                  including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER) {}

            // -- properties --
            
            // position getter
            const Math::Vector & get_pos() const { return pos; }
            // velocity accessors
            const Math::Vector & get_velocity() const { return velocity; }
            Math::Vector angular_velocity_to_linear(const Math::Vector &body_angular_velocity,
                                                    const Math::Vector &body_center) const;
            // mass getter
            Math::Real get_mass() const { return mass; }
            
            // accessor to including_clusters_num
            void include_to_one_more_cluster() { ++including_clusters_num; }
            // TODO: this getter used for testing, is needed anyway?
            int get_including_clusters_num() const { return including_clusters_num; }

            // accessors to nearest_cluster_index
            bool set_nearest_cluster_index(int index);
            int get_nearest_cluster_index() const;

            // -- accessors to velocity_addition --

            const Math::Vector & get_velocity_addition() const { return velocity_addition; }

            // gets an addition from a single cluster, divides it by including_clusters_num,
            // and adds corrected value to velocity_addition, thus averaging addiitons
            bool add_to_average_velocity_addition(const Math::Vector &addition);
            
            void add_to_velocity(const Math::Vector &correction) { velocity += correction; }
            void set_velocity(const Math::Vector &new_velocity) { velocity = new_velocity; }

            // -- methods --

            // step integration
            bool integrate_velocity(const ForcesArray & forces, Math::Real dt);
            void integrate_position(Math::Real dt);
        };

        typedef Collections::Array<PhysicalVertex> PhysicalVertexArray;
    }
}

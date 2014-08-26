#pragma once
#include "Core/core.h"
#include "Core/force.h"
#include "Core/ivertex.h"
#include "Math/vector.h"
#include "Collections/array.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        class PhysicalVertex : public IVertex
        {
        private:
            // position of vertex in global coordinate system
            Math::Vector pos;
            Math::Real mass;
            Math::Vector velocity;
            
            static const int MAX_CLUSTERS_NUM = 8;
            Math::Vector velocity_additions[MAX_CLUSTERS_NUM];
            // computed with PhysicalVertex::compute_velocity_addition
            Math::Vector velocity_addition;
            Math::Vector equilibrium_pos;
            
            int next_addition_index;

            // Number of clusters which include current vertex
            int including_clusters_num;
        public:
            PhysicalVertex( Math::Vector pos = Math::Vector::ZERO,
                            Math::Real mass = 0,
                            Math::Vector velocity = Math::Vector(0,0,0) )
                : pos(pos), mass(mass), velocity(velocity),
                  equilibrium_pos(pos), including_clusters_num(0), next_addition_index(0) {}

            // -- properties --
            
            // position getter
            const Math::Vector & get_pos() const { return pos; }
            // velocity accessors
            const Math::Vector & get_velocity() const { return velocity; }
            Math::Vector angular_velocity_to_linear(const Math::Vector &body_angular_velocity,
                                                    const Math::Vector &body_center) const;
            // mass getter
            Math::Real get_mass() const { return mass; }
            
            // -- accessors to velocity_addition --

            // gets an addition from a single cluster, divides it by including_clusters_num,
            // and adds corrected value to velocity_addition, thus averaging addiitons.
            // addition_index is index in velocity_additions array: each cluster writes into its own
            // place to avoid race condition
            bool add_to_average_velocity_addition(const Math::Vector &addition, int addition_index);

            // averages velocity_additions into velocity_addition
            bool compute_velocity_addition();

            // Before calling this velocity_addition must computed with
            // PhysicalVertex::compute_velocity_addition
            const Math::Vector & get_velocity_addition() const { return velocity_addition; }

            int get_next_addition_index() { return next_addition_index++; }

            // adds to equilibrium_pos `delta', divided by including_clusters_num
            bool change_equilibrium_pos(const Math::Vector &delta);
            const Math::Vector & get_equilibrium_pos() const { return equilibrium_pos; }

            void add_to_velocity(const Math::Vector &correction) { velocity += correction; }
            void set_velocity(const Math::Vector &new_velocity) { velocity = new_velocity; }

            // -- methods --

            // step integration
            bool integrate_velocity(const ForcesArray & forces, Math::Real dt);
            void integrate_position(Math::Real dt);

            // implement IVertex
            virtual void include_to_one_more_cluster(int cluster_index, Math::Real weight);
            virtual int get_including_clusters_num() const { return including_clusters_num; }
            virtual bool check_in_cluster();
        };

        typedef Collections::Array<PhysicalVertex> PhysicalVertexArray;
    }
}

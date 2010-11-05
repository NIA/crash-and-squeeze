#pragma once
#include "Core/core.h"
#include "Core/force.h"
#include "Math/vector.h"

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
            int including_clusters_num;
            int nearest_cluster_index;
            // TODO: thread-safe cluster addition: velocity_additions[], velocity_addition_coeffs[]...
            Math::Vector velocity_addition;
            // a constant, determining how much deformation velocities are damped:
            // 0 - no damping of vibrations, 1 - maximum damping, rigid body
            Math::Real damping_constant;

            // common angular momentum function for both velocity and velocity_addition
            Math::Vector angular_momentum(const Math::Vector &which_velocity, const Math::Vector &center) const;
            
        public:
            static const int NOT_IN_A_CLUSTER = -2;
            static const Math::Real DEFAULT_DAMPING_CONSTANT;

            PhysicalVertex( Math::Vector pos,
                            Math::Real mass,
                            Math::Vector velocity = Math::Vector(0,0,0) )
                : pos(pos), mass(mass), velocity(velocity), velocity_addition(Math::Vector::ZERO),
                  including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER),
                  damping_constant(DEFAULT_DAMPING_CONSTANT) {}
            
            PhysicalVertex()
                : mass(0), pos(Math::Vector::ZERO), velocity(Math::Vector::ZERO), velocity_addition(Math::Vector::ZERO),
                  including_clusters_num(0), nearest_cluster_index(NOT_IN_A_CLUSTER),
                  damping_constant(DEFAULT_DAMPING_CONSTANT) {}

            // -- properties --
            
            // position getter
            const Math::Vector & get_pos() const { return pos; }
            // velocity accessors
            const Math::Vector & get_velocity() const { return velocity; }
            Math::Vector angular_velocity_to_linear(const Math::Vector &body_angular_velocity,
                                                    const Math::Vector &body_center) const;
            // damps oscillation velocity
            void damp_velocity(const Math::Vector &body_velocity,
                               const Math::Vector &body_angular_velocity,
                               const Math::Vector &body_center);
            // momenta getters
            const Math::Vector get_linear_momentum() const { return mass*velocity; }
            const Math::Vector get_angular_momentum(const Math::Vector &center) const
            {
                return angular_momentum(velocity, center);
            }
            // mass getter
            Math::Real get_mass() const { return mass; }
            
            // accessor to including_clusters_num
            void include_to_one_more_cluster() { ++including_clusters_num; }
            // TODO: this getter used for testing, is needed anyway?
            int get_including_clusters_num() const { return including_clusters_num; }

            // accessors to nearest_cluster_index
            bool set_nearest_cluster_index(int index);
            int get_nearest_cluster_index() const;

            // -- accessors to velocity_addition

            const Math::Vector get_linear_momentum_addition() const { return mass*velocity_addition; }
            const Math::Vector get_angular_momentum_addition(const Math::Vector &center) const
            {
                return angular_momentum(velocity_addition, center);
            }

            // gets an addition from a single cluster,
            // corrects it in place (!) according to including_clusters_num,
            // and adds corrected value to velocity_addition
            bool add_to_velocity_addition(Math::Vector &addition);
            
            // applies specific (per a unity of mass) velocity correction
            void correct_velocity_addition(const Math::Vector &specific_velocity_correction);

            // step integration
            bool integrate_velocity(const ForcesArray & forces, Math::Real dt);
            void integrate_position(Math::Real dt) { pos += velocity*dt; }
        };
    }
}

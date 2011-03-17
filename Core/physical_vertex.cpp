#include "Core/physical_vertex.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;
    using Math::Vector;

    namespace Core
    {
        // gets an addition from a single cluster, divides it by including_clusters_num,
        // and adds corrected value to velocity_addition, thus averaging addiitons
        bool PhysicalVertex::add_to_average_velocity_addition(const Vector & addition, int addition_index)
        {
            // additions from different clusters are placed in different array items,
            // and get averaged in PhysicalVertex::integrate_velocity
            velocity_additions[addition_index] = addition;
            return true;
        }

        bool PhysicalVertex::change_equilibrium_pos(const Vector &delta)
        {
            if( false == check_in_cluster() )
                return false;

            equilibrium_pos += delta/get_including_clusters_num();
            return true;
        }

        bool PhysicalVertex::compute_velocity_addition()
        {
            if( false == check_in_cluster() )
                return false;
            
            // compute average velocity addition
            Vector velocity_addition = Vector::ZERO;
            for(int i = 0; i < get_including_clusters_num(); ++i)
            {
                velocity_addition += velocity_additions[i];
            }
            velocity_addition /= get_including_clusters_num();
            return true;
        }

        bool PhysicalVertex::integrate_velocity(const ForcesArray & forces, Real dt)
        {
            Vector acceleration = Vector::ZERO;

            if(0 != mass)
            {
                for(int i = 0; i < forces.size(); ++i)
                {
                    if(NULL == forces[i])
                    {
                        Logger::error("in PhysicalVertex::integrate_velocity: null pointer item of `forces' array ", __FILE__, __LINE__);
                        return false;
                    }
                    acceleration += forces[i]->get_value_at(pos, velocity)/mass;
                }
            }


            velocity += velocity_addition + acceleration*dt;
            return true;
        }

        void PhysicalVertex::integrate_position(Math::Real dt)
        {
            pos += velocity*dt;
        }

        Vector PhysicalVertex::angular_velocity_to_linear(const Math::Vector &body_angular_velocity,
                                                          const Math::Vector &body_center) const
        {
            return cross_product(body_angular_velocity, pos - body_center);
        }

        void PhysicalVertex::include_to_one_more_cluster(int cluster_index)
        {
            ignore_unreferenced(cluster_index);
            ++including_clusters_num;
        }

        bool PhysicalVertex::check_in_cluster()
        {
            if(0 == including_clusters_num)
            {
                Logger::error("internal error: PhysicalVertex doesn't belong to any cluster", __FILE__, __LINE__);
                return false;
            }
            return true;
        }
    }
}

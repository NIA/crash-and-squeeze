#include "Core/physical_vertex.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;
    using Math::Vector;

    namespace Core
    {
        bool PhysicalVertex::set_nearest_cluster_index(int index)
        {
            if(NOT_IN_A_CLUSTER != nearest_cluster_index)
            {
                Logger::error("in PhysicalVertex::set_nearest_cluster_index: already set", __FILE__, __LINE__);
                return false;
            }
            if(index < 0)
            {
                Logger::error("in PhysicalVertex::set_nearest_cluster_index: invalid index", __FILE__, __LINE__);
                return false;
            }

            nearest_cluster_index = index;
            return true;
        }

        int PhysicalVertex::get_nearest_cluster_index() const
        {
            if(NOT_IN_A_CLUSTER == nearest_cluster_index)
            {
                Logger::error("in PhysicalVertex::get_nearest_cluster_index: internal error: vertex doesn't belong to any cluster", __FILE__, __LINE__);
                return 0;
            }
            return nearest_cluster_index;
        }

        // gets an addition from a single cluster, divides it by including_clusters_num,
        // and adds corrected value to velocity_addition, thus averaging addiitons
        bool PhysicalVertex::add_to_average_velocity_addition(const Vector & addition)
        {
            // we need average velocity addition, not sum, so divide by including_clusters_num
            if(0 == including_clusters_num)
            {
                Logger::error("internal error: in Cluster::apply_goal_positions: vertex with incorrect zero value of including_clusters_num", __FILE__, __LINE__);
                return false;
            }

            // TODO: thread-safe cluster addition: velocity_additions[]...
            velocity_addition += addition/including_clusters_num;
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
            velocity_addition = Vector::ZERO;
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
    }
}

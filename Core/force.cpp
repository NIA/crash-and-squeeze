#include "Core/force.h"

namespace CrashAndSqueeze
{
    using namespace Math;
    using Logging::logger;

    namespace Core
    {
        // -- abstract Force --

        Vector Force::compute_value_at(const Vector &point, const Vector &velocity) const
        {
            ignore_unreferenced(point);
            ignore_unreferenced(velocity);
            return value;
        }

        Vector Force::get_value_at(const Vector &point, const Vector &velocity) const
        {
            return is_active() && is_applied_to(point) ? compute_value_at(point, velocity) : Vector::ZERO;
        }

        // -- PointForce --
        
        void PointForce::set_radius(Real radius)
        {
            if( radius < 0 )
            {
                logger.warning("given radius for PointForce is less than 0, corrected to 0", __FILE__, __LINE__);
                this->radius = 0;
            }
            else
            {
                this->radius = radius;
            }
        }

        // -- PlaneForce --

        void PlaneForce::set_max_distance(const Real &value)
        {
            max_distance = value;
            if(max_distance < 0)
            {
                max_distance = 0;
                logger.warning("given max distance for PlaneForce is less than 0, corrected to 0", __FILE__, __LINE__);
            }
        }

        // -- HalfSpaceSpringForce --
        void HalfSpaceSpringForce::set_spring_constant(Real value)
        {
            spring_constant = value;
            if(spring_constant < 0)
            {
                spring_constant = 0;
                logger.warning("given spring constant for HalfSpaceSpringForce is less than 0, corrected to 0", __FILE__, __LINE__);
            }
        }

        void HalfSpaceSpringForce::set_damping_constant(Real value)
        {
            damping_constant = value;
            if(damping_constant < 0)
            {
                damping_constant = 0;
                logger.warning("given damping constant for HalfSpaceSpringForce is less than 0, corrected to 0", __FILE__, __LINE__);
            }
        }
    }
}

#include "Core/force.h"

namespace CrashAndSqueeze
{
    using Math::Vector;
    using Math::Real;
    using Math::less_or_equal;
    using Logging::Logger;

    namespace Core
    {
        // -- abstract Force --
        
        Force::Force()
        {
            set_value(Vector::ZERO);
            activate();
        }

        Force::Force(const Vector &value)
        {
            set_value(value);
            activate();
        }

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

        // -- EverywhereForce --
        
        EverywhereForce::EverywhereForce()
        {
        }
        
        EverywhereForce::EverywhereForce(const Vector &value)
            : Force(value)
        {
        }
        
        bool EverywhereForce::is_applied_to(Vector const &point) const
        {
            ignore_unreferenced(point);
            return true;
        }
        
        // -- ForceWithRadius --
        
        ForceWithRadius::ForceWithRadius()
        {
            set_radius(0);
        }

        ForceWithRadius::ForceWithRadius(const Vector &value, Real radius)
            : Force(value)
        {
            set_radius(radius);
        }
        
        void ForceWithRadius::set_radius(Real radius)
        {
            if( radius < 0 )
            {
                Logger::warning("given radius for a subclass of ForceWidthRadius is less than 0, corrected to 0", __FILE__, __LINE__);
                this->radius = 0;
            }
            else
            {
                this->radius = radius;
            }
        }

        // -- PointForce --

        PointForce::PointForce()
        {
            set_point_of_application(Vector::ZERO);
        }

        PointForce::PointForce(const Vector &value, const Vector &point_of_application, Real radius)
            :  ForceWithRadius(value, radius)
        {
            set_point_of_application(point_of_application);
        }
        
        bool PointForce::is_applied_to(const Vector &point) const
        {
            return less_or_equal( distance(point, point_of_application), get_radius() );
        }

        // -- PlaneForce --

        PlaneForce::PlaneForce()
        {
            set_max_distance(0);
        }

        PlaneForce::PlaneForce(const Vector &value,
                               const Vector &plane_point,
                               const Vector &plane_normal,
                               Real max_distance)
            : Force(value), plane(plane_point, plane_normal)
        {
            set_max_distance(max_distance);
        }

        void PlaneForce::set_max_distance(const Real &value)
        {
            max_distance = value;
            if(max_distance < 0)
            {
                max_distance = 0;
                Logger::warning("given max distance for PlaneForce is less than 0, corrected to 0", __FILE__, __LINE__);
            }
        }

        bool PlaneForce::is_applied_to(const Vector &point) const
        {
            return less_or_equal( plane.distance_to(point), max_distance );
        }

        // -- Spring --
        
        Spring::Spring()
        {
            set_spring_constant(0);
            set_damping_constant(0);
        }

        Spring::Spring(Real spring_constant, Real damping_constant)
        {
            set_spring_constant(spring_constant);
            set_damping_constant(damping_constant);
        }

        void Spring::set_spring_constant(Real value)
        {
            spring_constant = value;
            if(spring_constant < 0)
            {
                spring_constant = 0;
                Logger::warning("given spring constant for Spring is less than 0, corrected to 0", __FILE__, __LINE__);
            }
        }

        void Spring::set_damping_constant(Real value)
        {
            damping_constant = value;
            if(damping_constant < 0)
            {
                damping_constant = 0;
                Logger::warning("given damping constant for Spring is less than 0, corrected to 0", __FILE__, __LINE__);
            }
        }

        Vector Spring::compute_force(const Vector &shift, const Vector &velocity) const
        {
            return - spring_constant*shift - damping_constant*velocity;
        }

        // -- HalfSpaceSpringForce --
        
        HalfSpaceSpringForce::HalfSpaceSpringForce()
        {
        }
        
        HalfSpaceSpringForce::HalfSpaceSpringForce(const Real spring_constant,
                                                   const Vector &plane_point,
                                                   const Vector &plane_normal,
                                                   const Real damping_constant)
            : spring(spring_constant, damping_constant),
              plane(plane_point, plane_normal)
        {
        }
        
        bool HalfSpaceSpringForce::is_applied_to(const Vector &point) const
        {
            return plane.projection_to_normal(point) <= 0;
        }

        Vector HalfSpaceSpringForce::compute_value_at(const Vector &point, const Vector &velocity) const
        {
            Real x = plane.projection_to_normal(point);
            return spring.compute_force(x*plane.get_normal(), velocity);
        }

        // -- CylinderSpringForce --

        CylinderSpringForce::CylinderSpringForce()
        {
            set_points(Vector::ZERO, Vector(1,1,1));
        }

        CylinderSpringForce::CylinderSpringForce(const Real spring_constant,
                                                 const Vector &point1,
                                                 const Vector &point2,
                                                 const Real radius,
                                                 const Real damping_constant)
            : ForceWithRadius(Vector::ZERO, radius),
              spring(spring_constant, damping_constant)
        {
            set_points(point1, point2);
        }

        void CylinderSpringForce::set_points(const Vector &p1, const Vector &p2)
        {
            if(p1 == p2)
            {
                Logger::error("given points for CylinderSpringForce are equal", __FILE__, __LINE__);
            }
            else
            {
                point1 = p1;
                point2 = p2;
            }
        }

        bool CylinderSpringForce::is_applied_to(const Vector &point) const
        {
            Vector normal_component;
            Real projection = (point - point1).project_to(point2 - point1, &normal_component);
            return projection >= 0
                && projection <= distance(point1, point2)
                && normal_component.norm() <= get_radius();
        }

        Vector CylinderSpringForce::compute_value_at(const Vector &point, const Vector &velocity) const
        {
            Vector normal_component;
            (point - point1).project_to(point2 - point1, &normal_component);

            Vector direction = normal_component.is_zero() ? Vector(0,0,1) : normal_component.normalized();
            Vector shift = normal_component - direction*get_radius();

            return spring.compute_force(shift, velocity);
        }
    }
}

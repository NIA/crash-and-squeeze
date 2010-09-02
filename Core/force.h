#pragma once
#include "Core/core.h"
#include "Math/vector.h"
#include "Math/plane.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract force:
        // * stores value
        // * provides Force::is_applied_to to check if it is applied to some point
        //   (virtual, so override it to change behaviour)
        // * provides Force::get_value_at to get value of force at some point
        //   (override Force::compute_value_at to change behaviour)
        class Force
        {
        private:
            Math::Vector value;
            bool is_active_;

        protected:
            virtual Math::Vector compute_value_at(const Math::Vector &point, const Math::Vector &velocity) const;
            
        public:
            bool is_active() const { return is_active_; }
            void activate() { is_active_ = true; }
            void deactivate() { is_active_ = false; }
            void toggle() { is_active_ = !is_active_; }

            void set_value(const Math::Vector &value) { this->value = value; }
            const Math::Vector & get_value() const { return value; }

            Force() { activate(); set_value(Math::Vector::ZERO); }
            Force(const Math::Vector &value) { set_value(value); activate(); }
            
            virtual bool is_applied_to(const Math::Vector &point) const = 0;
            
            Math::Vector get_value_at(const Math::Vector &point, const Math::Vector &velocity) const;
        };

        // A simplest force applied everywhere
        class EverywhereForce : public Force
        {
        public:
            EverywhereForce() {}
            EverywhereForce(const Math::Vector &value) : Force(value) {}
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const
            {
                ignore_unreferenced(point);
                return true;
            }
        };

        // A force applied pointwise
        class PointForce : public Force
        {
        private:
            Math::Vector point_of_application;
            Math::Real radius;

        public:
            const Math::Vector & get_point_of_application() const { return point_of_application; }
            void set_point_of_application(const Math::Vector &point) { point_of_application = point; }

            Math::Real get_radius() const { return radius; }
            void set_radius(Math::Real radius);

            PointForce() { set_radius(0); }
            PointForce(const Math::Vector &value, const Math::Vector &point_of_application, Math::Real radius)
                :  Force(value)
            {
                set_point_of_application(point_of_application);
                set_radius(radius);
            }
            
            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const
            {
                return Math::less_or_equal( distance(point, point_of_application), radius );
            }
        };

        // A force applied to all points on a plane
        class PlaneForce : public Force
        {
        private:
            Math::Plane plane;
            Math::Real max_distance;

        public:
            const Math::Vector & get_plane_point() const { return plane.get_point(); }
            const Math::Vector & get_plane_normal() const { return plane.get_normal(); }
            void set_plane(const Math::Vector &point, const Math::Vector &normal) { plane.set_point(point); plane.set_normal(normal); }

            Math::Real get_max_distance() const { return max_distance; }
            void set_max_distance(const Math::Real &value);

            PlaneForce() { set_max_distance(0); }
            PlaneForce(const Math::Vector &value,
                       const Math::Vector &plane_point,
                       const Math::Vector &plane_normal,
                       Math::Real max_distance)
                : Force(value)
            {
                set_max_distance(max_distance);
                set_plane(plane_point, plane_normal);
            }
            
            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const
            {
                return Math::less_or_equal( plane.distance_to(point), max_distance );
            }
        };
        
        // A force, pushing outside half-space, defined by plane_point and plane_normal.
        // A value is obtained by Hooke's law from position and spring_constant
        // "Outside" means positive half of an axis colinear to plane_normal.
        class HalfSpaceSpringForce : public Force
        {
        private:
            Math::Plane plane;
            Math::Real spring_constant;
            Math::Real damping_constant;
        public:
            const Math::Vector & get_plane_point() const { return plane.get_point(); }
            const Math::Vector & get_plane_normal() const { return plane.get_normal(); }
            void set_plane(const Math::Vector &point, const Math::Vector &normal) { plane.set_point(point); plane.set_normal(normal); }

            Math::Real get_spring_constant() const { return spring_constant; }
            void set_spring_constant(Math::Real value);

            Math::Real get_damping_constant() const { return damping_constant; }
            void set_damping_constant(Math::Real value);
            
            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const
            {
                return plane.projection_to_normal(point) < 0;
            }

            virtual /*override*/ Math::Vector compute_value_at(const Math::Vector &point, const Math::Vector &velocity) const
            {
                Math::Real x = plane.projection_to_normal(point);
                return - spring_constant*x*plane.get_normal() - damping_constant*velocity;
            }

            HalfSpaceSpringForce() { set_spring_constant(0); set_damping_constant(0); }
            HalfSpaceSpringForce(const Math::Real spring_constant,
                                 const Math::Vector &plane_point,
                                 const Math::Vector &plane_normal,
                                 const Math::Real damping_constant = 0)
            {
                set_spring_constant(spring_constant);
                set_damping_constant(damping_constant);
                set_plane(plane_point, plane_normal);
            }
        };
    }
}

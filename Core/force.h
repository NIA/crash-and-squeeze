#pragma once
#include "Core/core.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract force:
        // * stores value
        // * provides interface to check if it is applies to some point
        class Force
        {
        private:
            Math::Vector value;
            bool is_active_;
            
        public:
            bool is_active() const { return is_active_; }
            void activate() { is_active_ = true; }
            void deactivate() { is_active_ = false; }
            void toggle() { is_active_ = !is_active_; }

            void set_value(const Math::Vector &value) { this->value = value; }
            const Math::Vector & get_value() const { return value; }

            Force() { activate(); set_value(Math::Vector::ZERO); }
            Force(const Math::Vector &value) { set_value(value); activate(); }
            
            virtual bool is_applied_to(Math::Vector const &point) const = 0;
            virtual Math::Vector get_value_at(Math::Vector const &point) const
            {
                point; // avoid unreferenced parameter warning
                return value;
            }
        };

        // A simplest force applied everywhere
        class EverywhereForce : public Force
        {
        public:
            EverywhereForce() {}
            EverywhereForce(const Math::Vector &value) : Force(value) {}
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const
            {
                point; // avoid unreferenced parameter warning
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
            void set_radius(Math::Real radius)
            {
                if( radius < 0 )
                {
                    Logging::logger.warning("given radius for PointForce is less than 0, corrected to 0", __FILE__, __LINE__);
                    this->radius = 0;
                }
                else
                {
                    this->radius = radius;
                }
            }

            PointForce() { set_radius(0); }
            PointForce(const Math::Vector &value, const Math::Vector &point_of_application, Math::Real radius)
                :  Force(value)
            {
                set_point_of_application(point_of_application);
                set_radius(radius);
            }
            
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const
            {
                return Math::less_or_equal( distance(point, point_of_application), radius );
            }
        };

        // A force applied to all points on a plane
        class PlaneForce : public Force
        {
        private:
            Math::Vector plane_point;
            Math::Vector plane_normal;
            Math::Real max_distance;

        public:
            const Math::Vector & get_plane_point() const { return plane_point; }
            const Math::Vector & get_plane_normal() const { return plane_normal; }
            void set_plane(const Math::Vector &point, const Math::Vector &normal)
            {
                this->plane_point = point;
                this->plane_normal = normal;
                if(Math::equal(0, plane_normal.squared_norm()))
                    Logging::logger.error("setting zero vector as plane normal for PlaneForce", __FILE__, __LINE__);
                else
                    this->plane_normal /= this->plane_normal.norm();
            }

            Math::Real get_max_distance() const { return max_distance; }
            void set_max_distance(const Math::Real &value)
            {
                if(value < 0)
                {
                    Logging::logger.warning("given max distance for PlaneForce is less than 0, corrected to 0", __FILE__, __LINE__);
                    this->max_distance = 0;
                }
                else
                {
                    this->max_distance = value;
                }

            }

            PlaneForce() { set_max_distance(0); set_plane(Math::Vector::ZERO, Math::Vector(1,0,0)); }
            PlaneForce(const Math::Vector &value,
                       const Math::Vector &plane_point,
                       const Math::Vector &plane_normal,
                       Math::Real max_distance)
                : Force(value)
            {
                set_max_distance(max_distance);
                set_plane(plane_point, plane_normal);
            }
            
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const
            {
                return Math::less_or_equal( abs( (point - plane_point)*plane_normal ), max_distance );
            }
        };
    }
}

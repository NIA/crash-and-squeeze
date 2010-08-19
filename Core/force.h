#pragma once
#include "Core/core.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract force:
        // * stores magnitude
        // * provides interface to check if it is applies to some point
        struct Force
        {
            Math::Vector value;
            bool is_active;
            
            Force() : is_active(true) {}
            Force(Math::Vector value) : value(value), is_active(true) {}
            
            virtual bool is_applied_to(Math::Vector const &point) const = 0;
        };

        // A force applied pointwise
        struct PointForce : public Force
        {
            Math::Vector point_of_application;
            Math::Real radius;

            PointForce() : radius(0) {}
            PointForce(Math::Vector value, Math::Vector point_of_application, Math::Real radius)
                :  Force(value), point_of_application(point_of_application), radius(radius) {}
            
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const
            {
                return Math::less_or_equal( distance(point, point_of_application), radius );
            }
        };

        // A force applied to all points on a plane
        struct PlaneForce : public Force
        {
            Math::Vector plane_point;
            Math::Vector plane_normal;
            Math::Real max_distance;

            PlaneForce() : max_distance(0), plane_normal(1,0,0) {}
            PlaneForce(Math::Vector value,
                       Math::Vector plane_point,
                       Math::Vector plane_normal,
                       Math::Real max_distance)
                : Force(value), plane_point(plane_point), plane_normal(plane_normal), max_distance(max_distance)
            {
                if(0 == plane_normal.sqared_norm())
                    this->plane_normal = Math::Vector(1,0,0);
                else
                    this->plane_normal /= this->plane_normal.norm();

            }
            
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const
            {
                return Math::less_or_equal( abs( (point - plane_point)*plane_normal ), max_distance );
            }
        };
    }
}

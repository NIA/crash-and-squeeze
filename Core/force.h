#pragma once
#include "vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract force:
        // * stores magnitude
        // * provides interface to check if it is applies to some point
        struct Force
        {
            Math::Real magnitude;
            bool is_active;
            
            Force() : magnitude(0) {}
            Force(Math::Real magnitude) : magnitude(magnitude) {}
            
            virtual bool is_applied_to(Math::Vector const &point) = 0;
        };

        // A force applied pointwise
        struct PointForce : public Force
        {
            Math::Vector point_of_application;
            Math::Real radius;

            PointForce() : radius(0) {}
            PointForce(Math::Real magnitude, Math::Vector point_of_application, Math::Real radius)
                :  Force(magnitude), point_of_application(point_of_application), radius(radius) {}
            
            virtual /*override*/ bool is_applied_to(Vector const &point)
            {
                return Math::less_or_equal( distance(point, point_of_application), radius )
            }
        };

        // A force applied to all points on a plane
        struct PlaneForce : public Force
        {
            Math::Vector plane_point;
            Math::Vector plane_normal;
            Math::Real max_distance;

            PlaneForce() : max_distance(0), plane_normal(1,0,0) {}
            PlaneForce(Math::Real magnitude,
                       Math::Vector plane_point,
                       Math::Vector plane_normal,
                       Math::Real max_distance)
                : Force(magnitude), plane_point(plane_point), plane_normal(plane_normal), max_distance(max_distance)
            {
                if(0 == plane_normal.sqared_norm())
                    this->plane_normal = Vector(1,0,0);
                else
                    this->plane_normal /= this->plane_normal.norm();

            }
            
            virtual /*override*/ bool is_applied_to(Vector const &point)
            {
                return Math::less_or_equal( abs( (point - plane_point)*plane_normal ), max_distance );
            }
        }
    }
}
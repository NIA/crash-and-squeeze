#pragma once
#include "Logging/logger.h"
#include "Math/vector.h"
#include <cmath>

namespace CrashAndSqueeze
{
    namespace Math
    {
        // A plane, represented as point and normal
        class Plane
        {
        private:
            Math::Vector point;
            Math::Vector normal;

        public:
            const Math::Vector & get_point() const { return point; }
            void set_point(const Math::Vector &point) { this->point = point; }
            
            const Math::Vector & get_normal() const { return normal; }
            void set_normal(const Math::Vector &normal)
            {
                this->normal = normal;
                if(Math::equal(0, this->normal.squared_norm()))
                    Logging::logger.error("Plane::set_normal: zero vector given, cannot normalize", __FILE__, __LINE__);
                else
                    this->normal.normalize();
            }

            Plane() { set_point(Math::Vector::ZERO); set_normal(Math::Vector(1,0,0)); }
            Plane(const Math::Vector &point, const Math::Vector &normal) { set_point(point); set_normal(normal); }

            Math::Real projection_to_normal(const Math::Vector &point) const
            {
                return (point - this->point)*this->normal;
            }

            Math::Real distance_to(const Math::Vector &point) const
            {
                return abs( projection_to_normal( point ) );
            }
        };
    }
}
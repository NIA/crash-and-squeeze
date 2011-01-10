#pragma once
#include "Core/core.h"
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An abstract region: it answers whether it contains the given point or not
        class IRegion
        {
        public:
            virtual bool contains(const Math::Vector &point) const = 0;
            virtual void move(const Math::Vector &vector) = 0;
        };
        
        class EmptyRegion : public IRegion
        {
        public:
            virtual bool contains(const Math::Vector &point) const;
            virtual void move(const Math::Vector &vector);
        };

        class SphericalRegion : public IRegion
        {
        private:
            Math::Vector center;
            Math::Real radius;
        public:
            SphericalRegion(Math::Vector center, Math::Real radius);

            // -- properties --
            const Math::Vector & get_center() const { return center; }
            void set_center(const Math::Vector & point) { center = point; }

            Math::Real get_radius() const { return radius; }
            void set_radius(Math::Real value);

            // -- implement Region --
            virtual bool contains(const Math::Vector &point) const;
            virtual void move(const Math::Vector &vector);
        };

        class CylindricalRegion : public IRegion
        {
        private:
            Math::Vector top_center;
            Math::Vector bottom_center;
            Math::Real radius;
        public:
            CylindricalRegion(const Math::Vector & top_center,
                              const Math::Vector & bottom_center,
                              Math::Real radius);
            
            // -- properties --
            const Math::Vector & get_top_center() const { return top_center; }
            const Math::Vector & get_bottom_center() const { return bottom_center; }
            bool set_axis(const Math::Vector & top_center, const Math::Vector & bottom_center);
            
            // axis is the vector from bottom_center to top_center
            Math::Vector get_axis() const { return top_center - bottom_center; }

            Math::Real get_radius() const { return radius; }
            void set_radius(Math::Real value);

            // -- implement Region --
            virtual bool contains(const Math::Vector &point) const;
            virtual void move(const Math::Vector &vector);
        };

        class BoxRegion : public IRegion
        {
        private:
            Math::Vector min_corner;
            Math::Vector max_corner;
        public:
            BoxRegion(const Math::Vector & min_corner, const Math::Vector & max_corner);

            // -- properties --
            const Math::Vector & get_min_corner() const { return min_corner; }
            const Math::Vector & get_max_corner() const { return max_corner; }
            void set_box(const Math::Vector & min_corner, const Math::Vector & max_corner);

            // -- implement Region --
            virtual bool contains(const Math::Vector &point) const;
            virtual void move(const Math::Vector &vector);
        };
    }
}

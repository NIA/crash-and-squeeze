#pragma once
#include "Collections/array.h"
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
            Force();
            Force(const Math::Vector &value);
            
            bool is_active() const { return is_active_; }
            void activate() { is_active_ = true; }
            void deactivate() { is_active_ = false; }
            void toggle() { is_active_ = !is_active_; }

            void set_value(const Math::Vector &value) { this->value = value; }
            const Math::Vector & get_value() const { return value; }

            virtual bool is_applied_to(const Math::Vector &point) const = 0;
            Math::Vector get_value_at(const Math::Vector &point, const Math::Vector &velocity) const;
        };

        typedef ::CrashAndSqueeze::Collections::Array<Force *> ForcesArray;

        // An abstract displacement
        class IDisplacement
        {
        public:
            virtual bool is_applied_to(const Math::Vector &point) const = 0;
            virtual Math::Vector get_value_at(const Math::Vector &point) const = 0;
        };

        typedef ::CrashAndSqueeze::Collections::Array<IDisplacement *> DisplacementsArray;

        // - - - - - Some concrete implementations - - - -

        // A simplest force applied everywhere
        class EverywhereForce : public Force
        {
        public:
            EverywhereForce();
            EverywhereForce(const Math::Vector &value);
            virtual /*override*/ bool is_applied_to(Math::Vector const &point) const;
        };

        // An abstract force having property of radius
        class ForceWithRadius : public Force
        {
        private:
            Math::Real radius;
        public:
            ForceWithRadius();
            ForceWithRadius(const Math::Vector &value, Math::Real radius);
            
            Math::Real get_radius() const { return radius; }
            void set_radius(Math::Real radius);
        };

        // A force applied pointwise
        class PointForce : public ForceWithRadius
        {
        private:
            Math::Vector point_of_application;

        public:
            PointForce();
            PointForce(const Math::Vector &value, const Math::Vector &point_of_application, Math::Real radius);
            
            const Math::Vector & get_point_of_application() const { return point_of_application; }
            void set_point_of_application(const Math::Vector &point) { point_of_application = point; }

            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const;
        };

        // A force applied to all points on a plane
        class PlaneForce : public Force
        {
        private:
            Math::Plane plane;
            Math::Real max_distance;

        public:
            PlaneForce();
            PlaneForce(const Math::Vector &value,
                       const Math::Vector &plane_point,
                       const Math::Vector &plane_normal,
                       Math::Real max_distance);
 
            const Math::Vector & get_plane_point() const { return plane.get_point(); }
            const Math::Vector & get_plane_normal() const { return plane.get_normal(); }
            void set_plane(const Math::Vector &point, const Math::Vector &normal) { plane.set_point(point); plane.set_normal(normal); }

            Math::Real get_max_distance() const { return max_distance; }
            void set_max_distance(const Math::Real &value);
            
            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const;
        };
        
        // An entity, used by all spring-like forces
        class Spring
        {
        private:
            Math::Real spring_constant;
            Math::Real damping_constant;

        public:
            Spring();
            Spring(Math::Real spring_constant, Math::Real damping_constant);
            
            Math::Real get_spring_constant() const { return spring_constant; }
            void set_spring_constant(Math::Real value);

            Math::Real get_damping_constant() const { return damping_constant; }
            void set_damping_constant(Math::Real value);

            Math::Vector compute_force(const Math::Vector &shift, const Math::Vector &velocity) const;
        };

        // A force, pushing outside half-space, defined by plane_point and plane_normal.
        // A value is obtained by Hooke's law from position and spring_constant
        // "Outside" means positive half of an axis collinear to plane_normal.
        class HalfSpaceSpringForce : public Force
        {
        private:
            Math::Plane plane;
            Spring spring;
        public:
            HalfSpaceSpringForce();
            HalfSpaceSpringForce(const Math::Real spring_constant,
                                 const Math::Vector &plane_point,
                                 const Math::Vector &plane_normal,
                                 const Math::Real damping_constant = 0);

            const Math::Vector & get_plane_point() const { return plane.get_point(); }
            const Math::Vector & get_plane_normal() const { return plane.get_normal(); }
            void set_plane(const Math::Vector &point, const Math::Vector &normal) { plane.set_point(point); plane.set_normal(normal); }

            Math::Real get_spring_constant() const { return spring.get_spring_constant(); }
            void set_spring_constant(Math::Real value) { spring.set_spring_constant(value); }

            Math::Real get_damping_constant() const { return spring.get_damping_constant(); }
            void set_damping_constant(Math::Real value) { spring.set_damping_constant(value); };
            
            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const;
            virtual /*override*/ Math::Vector compute_value_at(const Math::Vector &point, const Math::Vector &velocity) const;
        };

        // A force, imitating collision with cylinder: works like HalfSpringForce,
        // but the shape is cylinder instead of half-space
        class CylinderSpringForce : public ForceWithRadius
        {
        private:
            Math::Vector point1;
            Math::Vector point2;
            Spring spring;

        public:
            CylinderSpringForce();
            CylinderSpringForce(const Math::Real spring_constant,
                                const Math::Vector &point1,
                                const Math::Vector &point2,
                                const Math::Real radius,
                                const Math::Real damping_constant = 0);

            const Math::Vector & get_point1() const { return point1; }
            const Math::Vector & get_point2() const { return point2; }
            void set_points(const Math::Vector &p1, const Math::Vector &p2);

            Math::Real get_spring_constant() const { return spring.get_spring_constant(); }
            void set_spring_constant(Math::Real value) { spring.set_spring_constant(value); }

            Math::Real get_damping_constant() const { return spring.get_damping_constant(); }
            void set_damping_constant(Math::Real value) { spring.set_damping_constant(value); };
            
            virtual /*override*/ bool is_applied_to(const Math::Vector &point) const;
            virtual /*override*/ Math::Vector compute_value_at(const Math::Vector &point, const Math::Vector &velocity) const;
        };
    }
}

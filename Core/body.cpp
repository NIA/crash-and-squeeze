#include "Core/body.h"

namespace CrashAndSqueeze
{
    using Math::Real;
    using Math::less_or_equal;
    using Math::Vector;
    using Math::Matrix;
    using Logging::Logger;

    namespace Core
    {
        // creates rigid body from all given vertices
        Body::Body(PhysicalVertexArray &all_vertices)
            : vertices(all_vertices.size())
        {
            set_initial_values();

            for(int i = 0; i < all_vertices.size(); ++i)
                add_vertex(all_vertices[i]);

            vertices.freeze();
        }
        
        // creates rigid body only from vertices defined by body_indices
        Body::Body(PhysicalVertexArray &all_vertices, const IndexArray &body_indices)
            : vertices(body_indices.size())
        {
            set_initial_values();

            for(int i = 0; i < body_indices.size(); ++i)
                add_vertex(all_vertices[ body_indices[i] ]);

            vertices.freeze();
        }

        void Body::set_initial_values()
        {
              total_mass = 0;
              center_of_mass = Vector::ZERO;
              inertia_tensor = Matrix::ZERO;
              linear_velocity = Vector::ZERO;
              angular_velocity = Vector::ZERO;
              linear_velocity_addition = Vector::ZERO;
              angular_velocity_addition = Vector::ZERO;
        }

        void Body::add_vertex(PhysicalVertex &v)
        {
            vertices.push_back( &v );
            total_mass += v.get_mass();
        }

        bool Body::check_total_mass() const
        {
            if(less_or_equal(total_mass, 0))
            {
                Logger::error("internal error: Body run-time check: total_mass is <= 0", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        bool Body::compute_properties()
        {
            if( false == check_total_mass() )
                return false;
            
            center_of_mass = Vector::ZERO;
            for(int i = 0; i < vertices.size(); ++i)
            {
                const PhysicalVertex &v = *vertices[i];
                
                center_of_mass += v.get_pos()*(v.get_mass()/total_mass);
            }

            inertia_tensor = Matrix::ZERO;
            for(int i = 0; i < vertices.size(); ++i)
            {
                const PhysicalVertex &v = *vertices[i];
                Vector offset = v.get_pos() - center_of_mass;
                
                inertia_tensor += v.get_mass() * ( (offset*offset)*Matrix::IDENTITY - Matrix(offset, offset) );
            }

            return true;
        }

        bool Body::abstract_compute_velocities(VelocityFunc velocity_func,
                                               /*out*/ Vector & res_velocity,
                                               /*out*/ Vector & res_angular_velocity)
        {
            if( false == check_total_mass() )
                return false;

            res_velocity = Vector::ZERO;
            Vector angular_momentum = Vector::ZERO;
            
            for(int i = 0; i < vertices.size(); ++i)
            {
                const PhysicalVertex &vertex = *vertices[i];
                const Vector &velocity = (vertex.*velocity_func)();
                
                res_velocity += velocity*(vertex.get_mass()/total_mass);
                angular_momentum += vertex.get_mass() * cross_product(vertex.get_pos() - center_of_mass, velocity);
            }

            if( ! inertia_tensor.is_invertible() )
            {
                Logger::error("in Body::abstract_compute_velocities: inertia_tensor is singular, cannot invert to find angular velocity", __FILE__, __LINE__);
                return false;
            }
            res_angular_velocity = inertia_tensor.inverted()*angular_momentum;
            return true;
        }

        // computes velocity of center of mass and angular velocity
        bool Body::compute_velocities()
        {
            return abstract_compute_velocities(&PhysicalVertex::get_velocity,
                                               linear_velocity,
                                               angular_velocity);
        }

        // computes additions of velocity of center of mass and angular velocity
        bool Body::compute_velocity_additions()
        {
            for(int i = 0; i < vertices.size(); ++i)
            {
                if(false == vertices[i]->compute_velocity_addition())
                    return false;
            }

            return abstract_compute_velocities(&PhysicalVertex::get_velocity_addition,
                                               linear_velocity_addition,
                                               angular_velocity_addition);
        }
        
        // compensates given linear and angular velocity 
        void Body::compensate_velocities(const Math::Vector &linear, const Math::Vector &angular)
        {
            for(int i = 0; i < vertices.size(); ++i)
            {
                PhysicalVertex &v = *vertices[i];

                Vector correction = - linear - v.angular_velocity_to_linear(angular, center_of_mass);
                v.add_to_velocity(correction);
            }
        }

        const Real Body::MAX_RIGIDITY_COEFF = 1;
        
        void Body::set_rigid_motion(const IBody & body, Real coeff)
        {
            for(int i = 0; i < vertices.size(); ++i)
            {
                PhysicalVertex &v = *vertices[i];

                Vector rigid_velocity = body.get_linear_velocity()
                                      + v.angular_velocity_to_linear(body.get_angular_velocity(),
                                                                     body.get_position());

                v.set_velocity( coeff*rigid_velocity + (1 - coeff)*v.get_velocity() );
            }
        }
    }
}

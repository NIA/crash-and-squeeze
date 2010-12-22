#include "shape_matcher.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;
    using Math::equal;
    using Math::less_or_equal;
    using Math::greater_or_equal;
    using Math::sign;
    using Math::cube_root;
    using Math::Vector;
    using Math::Matrix;
    
    namespace Core
    {
        ShapeMatcher::ShapeMatcher()
            : vertex_infos(INITIAL_ALLOCATED_VERTICES_NUM)
        {
            reset();
        }

        void ShapeMatcher::reset()
        {
            vertex_infos.clear();
            
            total_mass = 0;
            center_of_mass = Vector::ZERO;
            rotation = Matrix::IDENTITY;

            initial_characteristics_computed = true;
            valid = false;
        }

        void ShapeMatcher::add_vertex(PhysicalVertex &vertex)
        {
            // update mass
            total_mass += vertex.get_mass();

            // add new vertex
            vertex_infos.create_item().vertex = &vertex;

            // invalidate initial characteristics
            initial_characteristics_computed = false;
        }

        void ShapeMatcher::compute_initial_characteristics()
        {
            vertex_infos.freeze();

            update_center_of_mass();

            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                
                vertex_infos[i].initial_offset_pos = v.get_pos() - center_of_mass;
                vertex_infos[i].equilibrium_offset_pos = vertex_infos[i].initial_offset_pos;
                vertex_infos[i].equilibrium_pos = center_of_mass + vertex_infos[i].equilibrium_offset_pos;
            }
            
            initial_characteristics_computed = true;

            compute_symmetric_term();
        }

        void ShapeMatcher::match_shape()
        {
            if(0 == get_vertices_num())
                return;

            update_center_of_mass();

            compute_linear_transformation();
            linear_transformation.do_polar_decomposition(rotation, scale);
        }
            
        void ShapeMatcher::update_center_of_mass()
        {
            center_of_mass = Vector::ZERO;
            if( 0 != total_mass )
            {
                for(int i = 0; i < get_vertices_num(); ++i)
                {
                    PhysicalVertex &v = get_vertex(i);
                    center_of_mass += v.get_mass()*v.get_pos()/total_mass;
                }
            }
        }

        void ShapeMatcher::update_equilibrium_offset_positions(const Matrix &transformation)
        {
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                vertex_infos[i].equilibrium_offset_pos = transformation * vertex_infos[i].initial_offset_pos;
            }
            compute_symmetric_term();
        }

        void ShapeMatcher::update_vertices_equilibrium_positions()
        {
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                Vector & equil_offset_pos = vertex_infos[i].equilibrium_offset_pos;
                Vector & equil_pos        = vertex_infos[i].equilibrium_pos;

                Vector new_equil_pos = center_of_mass + rotation * equil_offset_pos;

                get_vertex(i).change_equilibrium_pos(new_equil_pos - equil_pos);
                equil_pos = new_equil_pos;
            }
        }

        void ShapeMatcher::compute_asymmetric_term()
        {
            asymmetric_term = Matrix::ZERO;
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                const PhysicalVertex &v = get_vertex(i);

                asymmetric_term += v.get_mass()*Matrix( v.get_pos() - center_of_mass, get_equilibrium_offset_pos(i) );
            }
        }

        void ShapeMatcher::compute_symmetric_term()
        {
            symmetric_term = Matrix::ZERO;
            for(int i = 0; i < get_vertices_num(); ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_offset_pos(i);

                symmetric_term += v.get_mass()*Matrix( equilibrium_pos, equilibrium_pos );
            }
            if( symmetric_term.is_invertible() )
            {
                valid = true;
                symmetric_term = symmetric_term.inverted();
            }
            else
            {
                valid = false;
                symmetric_term = Matrix::ZERO;
            }
        }

        void ShapeMatcher::compute_linear_transformation()
        {
            // check that symmetric_term has been precomputed...
            if( ! check_initial_characteristics() )
                return;
            // ...and compute a brand new assymetric_term
            compute_asymmetric_term();

            linear_transformation = asymmetric_term*symmetric_term;

            // -- enforce volume conservation --

            Real det = linear_transformation.determinant();
            if( ! equal(0, det) )
            {
                if( det < 0 )
                {
                    Logger::warning("in ShapeMatcher::compute_linear_transformation: linear_transformation.determinant() is less than 0, inverted state detected!", __FILE__, __LINE__);
                }
                linear_transformation /= cube_root(det);
            }
            else
            {
                Logger::warning("in ShapeMatcher::compute_linear_transformation: linear_transformation is singular, so volume-preserving constraint cannot be enforced", __FILE__, __LINE__);
                // but now, while polar decomposition is only for invertible matrix - it's very, very bad...
            }
        }
            
        bool ShapeMatcher::check_initial_characteristics() const
        {
            if( ! initial_characteristics_computed )
            {
                Logger::error("missed precomputing: ShapeMatcher::compute_initial_characteristics must be called after last vertex is added", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        const PhysicalVertex & ShapeMatcher::get_vertex(int index) const
        {
            return *vertex_infos[index].vertex;
        }

        PhysicalVertex & ShapeMatcher::get_vertex(int index)
        {
            return *vertex_infos[index].vertex;
        }
        
        // returns equilibrium position of vertex
        // (measured off the center of mass of the cluster)
        // taking into account plasticity_state
        const Vector & ShapeMatcher::get_equilibrium_offset_pos(int index) const
        {
            check_initial_characteristics();
            return vertex_infos[index].equilibrium_offset_pos;
        }
    }
}
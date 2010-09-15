#include "Core/cluster.h"
#include <cstring>

namespace CrashAndSqueeze
{
    using Math::Real;
    using Math::equal;
    using Math::sign;
    using Math::cube_root;
    using Math::Vector;
    using Math::Matrix;
    
    namespace Core
    {
        struct PhysicalVertexMappingInfo
        {
            // index in model's vertex array
            PhysicalVertex *vertex;

            // TODO: thread-safe cluster addition: Math::Vector velocity_additions[MAX_CLUSTERS_FOR_VERTEX]
            
            // initial position of vertex measured off
            // the cluster's center of mass
            Math::Vector initial_offset_position;
        };

        Cluster::Cluster()
            : vertices(NULL),
              vertices_num(0),
              allocated_vertices_num(INITIAL_ALLOCATED_VERTICES_NUM),

              total_mass(0),

              goal_speed_constant(DEFAULT_GOAL_SPEED_CONSTANT),
              linear_elasticity_constant(DEFAULT_LINEAR_ELASTICITY_CONSTANT),
              damping_constant(DEFAULT_DAMPING_CONSTANT),
              yield_constant(DEFAULT_YIELD_CONSTANT),
              creep_constant(DEFAULT_CREEP_CONSTANT),

              initial_center_of_mass(Vector::ZERO),
              center_of_mass(Vector::ZERO),
              rotation(Matrix::IDENTITY),
              total_deformation(Matrix::IDENTITY),
              plasticity_state(Matrix::IDENTITY)
        {
            vertices = new PhysicalVertexMappingInfo[allocated_vertices_num];
        }

        void Cluster::add_vertex(PhysicalVertex &vertex)
        {
            // recompute center of mass
            Vector old_initial_center_of_mass = initial_center_of_mass;
            if( 0 != total_mass + vertex.mass )
                initial_center_of_mass = (total_mass*initial_center_of_mass + vertex.mass*vertex.pos)/(total_mass + vertex.mass);
            center_of_mass = initial_center_of_mass;
            
            // recompute vertex offsets due to change of center of mass
            for(int i = 0; i < vertices_num; ++i)
                vertices[i].initial_offset_position += old_initial_center_of_mass - initial_center_of_mass;
            
            // update mass
            total_mass += vertex.mass;

            // reallocate array if it is full
            if( vertices_num == allocated_vertices_num )
            {
                allocated_vertices_num *= 2;
                PhysicalVertexMappingInfo *new_vertices = new PhysicalVertexMappingInfo[allocated_vertices_num];
                memcpy(new_vertices, vertices, vertices_num*sizeof(vertices[0]));
                delete[] vertices;
                vertices = new_vertices;
            }
            
            // add new vertex
            ++vertices_num;
            vertices[vertices_num - 1].vertex = &vertex;
            vertices[vertices_num - 1].initial_offset_position = vertex.pos - initial_center_of_mass;

            // increment vertex's cluster counter
            ++vertex.including_clusters_num;
        }

        void Cluster::match_shape(Real dt)
        {
            if(0 == vertices_num)
                return;

            update_center_of_mass();
            compute_shape_matching_terms();
            compute_transformations();
            apply_goal_positions(dt);
            update_plasticity_state(dt);
        }
            

        void Cluster::update_center_of_mass()
        {
            center_of_mass = Vector::ZERO;
            if( 0 != total_mass )
            {
                for(int i = 0; i < vertices_num; ++i)
                {
                    PhysicalVertex &v = get_vertex(i);
                    center_of_mass += v.mass*v.pos/total_mass;
                }
            }
        }

        void Cluster::compute_shape_matching_terms()
        {
            asymmetric_term = Matrix::ZERO;
            symmetric_term = Matrix::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_position(i);

                asymmetric_term += v.mass*Matrix( v.pos - center_of_mass, equilibrium_pos );
                // TODO: re-compute this only on update of plasticity_state
                symmetric_term += v.mass*Matrix( equilibrium_pos, equilibrium_pos );
            }
            if( symmetric_term.is_invertible() )
            {
                symmetric_term = symmetric_term.inverted();
            }
            else
            {
                logger.warning("in Cluster::compute_shape_matching_terms: singular matrix symmetric_term, unable to compute symmetric_term.inverted(), assumed it to be Matrix::IDENTITY", __FILE__, __LINE__);
                symmetric_term = Matrix::IDENTITY; // TODO: is this good workaround???
            }
        }

        void Cluster::compute_transformations()
        {
            compute_linear_transformation();

            asymmetric_term.do_polar_decomposition(rotation, scale);
            
            total_deformation = linear_elasticity_constant*rotation + (1 - linear_elasticity_constant)*linear_transformation;
        }

        void Cluster::compute_linear_transformation()
        {
            linear_transformation = asymmetric_term*symmetric_term; // optimal linear transformation

            // -- enforce volume conservation --

            Real det = linear_transformation.determinant();
            if( ! equal(0, det) )
            {
                if( det < 0 )
                    logger.warning("in Cluster::compute_linear_transformation: linear_transformation.determinant() is less than 0, inverted state detected!", __FILE__, __LINE__);
                linear_transformation /= cube_root(det);
            }
            else
            {
                logger.warning("in Cluster::compute_linear_transformation: linear_transformation is singular, so volume-preserving constraint cannot be enforced", __FILE__, __LINE__);
                // but now, while polar decomposition is only for invertible matrix - it's very, very bad...
            }
        }

        void Cluster::apply_goal_positions(Real dt)
        {
            // -- find and apply velocity_addition --

            Vector linear_momentum_addition = Vector::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &vertex = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_position(i);
                
                Vector goal_position = total_deformation*equilibrium_pos + center_of_mass;
                // TODO: thread-safe cluster addition: velocity_additions[]...
                Vector velocity_addition = goal_speed_constant*(goal_position - vertex.pos)/dt;
                vertex.velocity_addition += velocity_addition;
                // TODO: thread-safe cluster addition: velocity_addition_coeffs[]...
                
                linear_momentum_addition += vertex.mass*velocity_addition;
            }

            // -- enforce total momentum conservation --

            if( 0 != total_mass )
            {
                Vector velocity_correction = - linear_momentum_addition / total_mass;
                for(int i = 0; i < vertices_num; ++i)
                {
                    PhysicalVertex &vertex = get_vertex(i);
                    vertex.velocity_addition += velocity_correction;
                }
            }
        }
        
        void Cluster::update_plasticity_state(Real dt)
        {
            Matrix deformation = linear_transformation - Matrix::IDENTITY;
            Real deformation_measure = deformation.norm();
            if(deformation_measure > yield_constant)
            {
                Matrix new_plasticity_state = (Matrix::IDENTITY + dt*creep_constant*deformation)*plasticity_state;
                Real det = new_plasticity_state.determinant();
                if( ! equal(0, det) )
                {
                    // enforce volume conservation
                    new_plasticity_state /= cube_root(det);
                    
                    Matrix plastic_deformation = new_plasticity_state - Matrix::IDENTITY;
                    Real plastic_deform_measure = plastic_deformation.norm();
                    
                    if( plastic_deform_measure < DEFAULT_MAX_DEFORMATION_CONSTANT ) // !!!
                    {
                        plasticity_state = new_plasticity_state;
                    }
                }
            }
        }
            
        bool Cluster::check_vertex_index(int index, const char *error_message) const
        {
            if(index < 0 || index >= vertices_num)
            {
                logger.error(error_message, __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        const PhysicalVertex & Cluster::get_vertex(int index) const
        {
            if( check_vertex_index(index, "Cluster::get_vertex: index out of range") )
                return *vertices[index].vertex;
            else
                return *vertices[0].vertex;
        }
        PhysicalVertex & Cluster::get_vertex(int index)
        {
            if( check_vertex_index(index, "Cluster::get_vertex: index out of range") )
                return *vertices[index].vertex;
            else
                return *vertices[0].vertex;
        }

        // returns equilibrium position of vertex
        // (measured off the center of mass of the cluster)
        // taking into account plasticity_state
        const Vector Cluster::get_equilibrium_position(int index) const
        {
            if( check_vertex_index(index, "Cluster::get_equilibrium_position: index out of range") )
                return plasticity_state*vertices[index].initial_offset_position;
            else
                return Vector::ZERO;
        }

        Cluster::~Cluster()
        {
            if(NULL != vertices) delete[] vertices;
        }
    }
}
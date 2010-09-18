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
        // a constant, determining how fast points are pulled to
        // their position, i.e. how rigid the body is:
        // 0 means no constraint at all, 1 means absolutely rigid
        const Real Cluster::DEFAULT_GOAL_SPEED_CONSTANT = 1;
        
        // a constant, determining how rigid body is:
        // if it equals `b`, then optimal deformation for goal positions
        // is calculated as (1 - b)*A + b*R, where R is optimal rotation
        // and A is optimal linear transformation.
        // Thus 0 means freely (but only linearly) deformable body,
        // 1 means absolutely rigid
        const Real Cluster::DEFAULT_LINEAR_ELASTICITY_CONSTANT = 1;
        
        // plasticity parameter: a treshold of strain, after
        // which deformation becomes non-reversible
        const Real Cluster::DEFAULT_YIELD_CONSTANT = 0.3; //!!!

        // plasticity paramter: a coefficient determining how fast
        // plasticity_state will be changed on large deformation
        const Real Cluster::DEFAULT_CREEP_CONSTANT = 40;

        // plasticity paramter: a treshold of maximum allowed strain
        const Real Cluster::DEFAULT_MAX_DEFORMATION_CONSTANT = 3;

        // An internal struct defining a membership of vertex in cluster
        struct PhysicalVertexMappingInfo
        {
            // index in model's vertex array
            PhysicalVertex *vertex;

            // TODO: thread-safe cluster addition: Math::Vector velocity_additions[MAX_CLUSTERS_FOR_VERTEX]
            
            // initial position of vertex measured off
            // the cluster's center of mass
            Math::Vector initial_offset_position;
            // position in deformed shape (plasticity_state*initial_offset_position)
            Math::Vector equilibrium_offset_position;
        };
        
        // -- Cluster methods --

        Cluster::Cluster()
            : vertices(NULL),
              vertices_num(0),
              allocated_vertices_num(INITIAL_ALLOCATED_VERTICES_NUM),
              initial_characteristics_computed(true),

              total_mass(0),
              valid(false),

              goal_speed_constant(DEFAULT_GOAL_SPEED_CONSTANT),
              linear_elasticity_constant(DEFAULT_LINEAR_ELASTICITY_CONSTANT),
              yield_constant(DEFAULT_YIELD_CONSTANT),
              creep_constant(DEFAULT_CREEP_CONSTANT),
              max_deformation_constant(DEFAULT_MAX_DEFORMATION_CONSTANT),

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
            vertices[vertices_num].vertex = &vertex;
            ++vertices_num;

            // increment vertex's cluster counter
            ++vertex.including_clusters_num;

            // invalidate initial characteristics
            initial_characteristics_computed = false;
        }

        void Cluster::compute_initial_characteristics()
        {
            update_center_of_mass();
            initial_center_of_mass = center_of_mass;

            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                
                vertices[i].initial_offset_position = v.pos - initial_center_of_mass;
                vertices[i].equilibrium_offset_position = vertices[i].initial_offset_position;
            }
            
            initial_characteristics_computed = true;

            compute_symmetric_term();
        }

        void Cluster::match_shape(Real dt)
        {
            if(0 == vertices_num)
                return;

            update_center_of_mass();
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
        void Cluster::update_equilibrium_positions()
        {
            for(int i = 0; i < vertices_num; ++i)
            {
                vertices[i].equilibrium_offset_position = plasticity_state * vertices[i].initial_offset_position;
            }
        }

        void Cluster::compute_asymmetric_term()
        {
            asymmetric_term = Matrix::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_position(i);

                asymmetric_term += v.mass*Matrix( v.pos - center_of_mass, get_equilibrium_position(i) );
            }
        }

        void Cluster::compute_symmetric_term()
        {
            symmetric_term = Matrix::ZERO;
            for(int i = 0; i < vertices_num; ++i)
            {
                PhysicalVertex &v = get_vertex(i);
                Vector equilibrium_pos = get_equilibrium_position(i);

                symmetric_term += v.mass*Matrix( equilibrium_pos, equilibrium_pos );
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

        void Cluster::compute_transformations()
        {
            compute_linear_transformation();

            asymmetric_term.do_polar_decomposition(rotation, scale);
            
            total_deformation = linear_elasticity_constant*rotation + (1 - linear_elasticity_constant)*linear_transformation;
        }

        void Cluster::compute_linear_transformation()
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
                
                Vector velocity_addition = goal_speed_constant*(goal_position - vertex.pos)/dt;

                // we need average velocity addition, not sum, so divide by vertex.including_clusters_num
                if(0 == vertex.including_clusters_num)
                {
                    logger.warning("internal error: in Cluster::apply_goal_positions: vertex with incorrect zero value of including_clusters_num", __FILE__, __LINE__);
                    continue;
                }
                velocity_addition /= vertex.including_clusters_num;
                
                // TODO: thread-safe cluster addition: velocity_additions[]...
                vertex.velocity_addition += velocity_addition;
                
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
                    
                    if( plastic_deform_measure < max_deformation_constant )
                    {
                        plasticity_state = new_plasticity_state;
                        update_equilibrium_positions();
                        compute_symmetric_term();
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

        bool Cluster::check_initial_characteristics() const
        {
            if( ! initial_characteristics_computed )
            {
                logger.error("missed precomputing: Cluster::compute_initial_characteristics must be called after last vertex is added", __FILE__, __LINE__);
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

        const Math::Vector & Cluster::get_initial_center_of_mass() const
        {
            check_initial_characteristics();
            return initial_center_of_mass;
        }
        
        // returns equilibrium position of vertex
        // (measured off the center of mass of the cluster)
        // taking into account plasticity_state
        const Vector & Cluster::get_equilibrium_position(int index) const
        {
            check_initial_characteristics();
            
            if( check_vertex_index(index, "Cluster::get_equilibrium_position: index out of range") )
                return vertices[index].equilibrium_offset_position;
            else
                return Vector::ZERO;
        }

        Cluster::~Cluster()
        {
            if(NULL != vertices) delete[] vertices;
        }
    }
}
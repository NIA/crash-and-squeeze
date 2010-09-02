#include "Core/cluster.h"
#include <cstring>

namespace CrashAndSqueeze
{
    using namespace Math;
    
    namespace Core
    {
        struct PhysicalVertexMappingInfo
        {
            // index in model's vertex array
            int vertex_index;

            // TODO: thread-safe cluster addition: Math::Vector velocity_additions[MAX_CLUSTERS_FOR_VERTEX]
            
            // initial offset of position of vertex from
            // cluster's center of mass
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

        void Cluster::add_vertex(int vertex_index, PhysicalVertex &vertex)
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
            vertices[vertices_num - 1].vertex_index = vertex_index;
            vertices[vertices_num - 1].initial_offset_position = vertex.pos - initial_center_of_mass;

            // increment vertex's cluster counter
            ++vertex.including_clusters_num;
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

        int Cluster::get_vertex_index(int index) const
        {
            if( check_vertex_index(index, "Cluster::get_vertex_index: index out of range") )
                return vertices[index].vertex_index;
            else
                return 0;
        }

        // returns offset of vector position in equilibrium state
        // taking into account plasticity_state
        const Vector Cluster::get_initial_vertex_offset_position(int index) const
        {
            if( check_vertex_index(index, "Cluster::get_initial_vertex_offset_position: index out of range") )
                return plasticity_state*vertices[index].initial_offset_position;
            else
                return Math::Vector::ZERO;
        }

        Cluster::~Cluster()
        {
            if(NULL != vertices) delete[] vertices;
        }
    }
}
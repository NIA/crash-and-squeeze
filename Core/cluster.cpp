#include "Core/cluster.h"
#include <cstring>

namespace CrashAndSqueeze
{
    using namespace Math;
    
    namespace Core
    {
        Cluster::Cluster()
            : vertices(NULL),
              vertices_num(0),
              allocated_vertices_num(INITIAL_ALLOCATED_VERTICES_NUM),
              total_mass(0),
              goal_speed_constant(DEFAULT_GOAL_SPEED_CONSTANT),
              linear_elasticity_constant(DEFAULT_LINEAR_ELASTICITY_CONSTANT),
              damping_constant(DEFAULT_DAMPING_CONSTANT),
              initial_center_of_mass(Vector::ZERO),
              center_of_mass(Vector::ZERO),
              rotation(Matrix::IDENTITY),
              total_deformation(Matrix::IDENTITY)
        {
            vertices = new PhysicalVertexMappingInfo[allocated_vertices_num];
        }

        void Cluster::add_vertex(int vertex_index, const PhysicalVertex &vertex)
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
        }

        Cluster::~Cluster()
        {
            if(NULL != vertices) delete[] vertices;
        }
    }
}
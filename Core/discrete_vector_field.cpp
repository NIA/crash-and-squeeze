#include "discrete_vector_field.h"

namespace CrashAndSqueeze
{
    using Math::Real;
    using Math::Vector;
    using Math::Point;
    using Math::ICurve;
    using Math::IConnection;
    using Math::ISpace;
    using Math::ConnectionCoeffs;
    using Logging::Logger;
    
    namespace Core
    {

        CrashAndSqueeze::Core::DiscreteVectorField::DiscreteVectorField(
            const void *source_vertices,
            int vertices_num,
            const VertexInfo &vertex_info,
            const ISpace * space)
            : nodes(vertices_num), space(space)
        {
            nodes.make_fixed_size(vertices_num);

            // Read vertices from `source_vertices`
            const void *source_vertex = source_vertices;
            for (int i = 0; i < nodes.size(); ++i)
            {
                const VertexFloat * src_position = reinterpret_cast<const VertexFloat*>( add_to_pointer(source_vertex, vertex_info.get_point_offset(0)));
                VertexInfo::vertex_floats_to_vector(src_position, nodes[i].pos);

                if (vertex_info.get_vectors_num() > 0)
                {
                    // TODO: possibility to have multiple vectors, like GraphicalVertex?
                    const VertexFloat * src_vector = reinterpret_cast<const VertexFloat*>( add_to_pointer(source_vertex, vertex_info.get_vector_offset(0)));
                    VertexInfo::vertex_floats_to_vector(src_vector, nodes[i].vector);
                }
                else
                {
                    nodes[i].vector = Vector::ZERO;
                }

                source_vertex = add_to_pointer(source_vertex, vertex_info.get_vertex_size());
            }
        }

        void DiscreteVectorField::transport(const Vector & initial_vector, const ICurve * curve, int steps_num /*= DEFAULT_STEPS_NUM*/)
        {
            if (steps_num < 1)
            {
                Logger::error("in DiscreteVectorField::transport: steps_num must be 1 or more", __FILE__, __LINE__);
                return;
            }

            const IConnection * conn = space->get_connection();
            Vector v = initial_vector;
            Vector x = curve->point_at(0);
            Vector dx = Vector::ZERO;
            Real dt = (ICurve::T_END - ICurve::T_START) / steps_num;

            for (int s = 0; s <= steps_num; ++s)
            {
                Real t = ICurve::T_START + dt*s;
                Vector new_x = curve->point_at(t);
                dx = new_x - x;

                ConnectionCoeffs gijk;
                conn->value_at(x, /*out*/ gijk);
                v += gijk.d_parallel_transport(v, dx);

                int i = find_near(x, dx.norm());
                if (NOT_FOUND != i)
                    nodes[i].vector = v;

                x = new_x;
            }
        }

        int DiscreteVectorField::find_near(Vector pos, Real reqired_dist) const
        {
            // TODO: more optimal algorithm

            // Store the minimal distance in case there are multiple nodes in the distance `reqired_dist`
            Real min_dist = 0;
            bool found = false;
            int found_index = 0;
            for (int i = 0; i < nodes.size(); ++i)
            {
                Real cur_dist = distance(pos, nodes[i].pos);
                if (cur_dist < reqired_dist) // then it is a candidate
                {
                    if (!found || cur_dist < min_dist)
                    {
                        min_dist = cur_dist;
                        found = true;
                        found_index = i;
                    }
                }

            }
            if (found)
                return found_index;
            else
                return NOT_FOUND;
        }

        // TODO: some copy-paste from model.cpp might be eliminated

        void DiscreteVectorField::update_vertices(/*out*/ void *out_vertices, const VertexInfo &vertex_info, int start_vertex /*= 0*/, int vertices_num /*= ALL_VERTICES*/, bool to_cartesian /*= true*/) const
        {
            if(vertices_num == ALL_VERTICES)
            {
                start_vertex = 0;
                vertices_num = nodes.size();
            }
            if(start_vertex < 0 || start_vertex >= nodes.size())
            {
                Logger::error("in DiscreteVectorField::update_vertices: incorrect start vertex index", __FILE__, __LINE__);
                return;
            }
            if(vertices_num > nodes.size() - start_vertex)
            {
                Logger::warning("in DiscreteVectorField::update_vertices: requested to update too many vertices, probably wrong vertices given?", __FILE__, __LINE__);
                vertices_num = nodes.size() - start_vertex;
            }

            int last_vertex = start_vertex + vertices_num - 1;
            VertexFloat * out_vertex = reinterpret_cast<VertexFloat*>( add_to_pointer(out_vertices, start_vertex*vertex_info.get_vertex_size()) );
            for(int i = start_vertex; i <= last_vertex; ++i)
            {
                // Update position
                VertexFloat * out_pos = add_to_pointer(out_vertex, vertex_info.get_point_offset(0));
                Point pos = nodes[i].pos;
                if (to_cartesian)
                    pos = space->point_to_cartesian(pos);
                VertexInfo::vector_to_vertex_floats(pos, out_pos);
                
                // Update vector
                VertexFloat * out_vector = add_to_pointer(out_vertex, vertex_info.get_vector_offset(0));
                Vector vector = nodes[i].vector;
                if (to_cartesian)
                    vector = space->vector_to_cartesian(vector, nodes[i].pos);
                VertexInfo::vector_to_vertex_floats(vector, out_vector);

                out_vertex = add_to_pointer(out_vertex, vertex_info.get_vertex_size());
            }
        }

        DiscreteVectorField::~DiscreteVectorField(void)
        {
        }

    }
}
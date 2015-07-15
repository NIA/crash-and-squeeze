#include "discrete_vector_field.h"

namespace CrashAndSqueeze
{
    using Math::Real;
    using Math::Vector;
    using Math::Point;
    using Math::ICurve;
    using Math::IConnection;
    using Math::ISurface;
    using Math::ICurvature;
    using Math::ISpace;
    using Math::ConnectionCoeffs;
    using Math::CurvatureTensor;
    using Math::maximum;
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

        Vector DiscreteVectorField::transport_along(const Vector & initial_vector, const ICurve * curve, int steps_num /*= DEFAULT_STEPS_NUM*/)
        {
            if (steps_num < 1)
            {
                Logger::error("in DiscreteVectorField::transport_along: steps_num must be 1 or more", __FILE__, __LINE__);
                return Vector::ZERO;
            }

            const IConnection * conn = space->get_connection();
            Vector vec = initial_vector;
            Vector x = curve->point_at(ICurve::T_START);
            Vector dx = Vector::ZERO;
            Real dt = (ICurve::T_END - ICurve::T_START) / steps_num;

            for (int s = 0; s <= steps_num; ++s)
            {
                // Parameter values for current step
                Real t = ICurve::T_START + dt*s;
                Vector new_x = curve->point_at(t);
                dx = new_x - x;

                // Calculate parallel transport using connection
                ConnectionCoeffs gijk;
                conn->value_at(x, /*out*/ gijk);
#if CAS_UPDATE_DURING_TRANSPORT
                vec += gijk.d_parallel_transport(vec, dx);
#else
                vec += gijk.d_parallel_transport(initial_vector, dx);
#endif // UPDATE_DURING_TRANSPORT
                x = new_x;

                // If possible, apply results to the nearby node
                // Nearby here means closer than |dx| (or closer than min_dr if |dx|<min_dr)
                // TODO: this `min_dr` was introduced as a hack in order not to skip the first point. Probably the original problem could be solved in some other way?
                static const  Real min_dr = 0.0001;
                int i = find_near(x, maximum(dx.norm(), min_dr));
                if (NOT_FOUND != i)
                    nodes[i].vector = vec;
            }
            return vec;
        }

        Vector DiscreteVectorField::transport_around(const Vector & initial_vector, const ISurface *surface, int u_steps_num /*= DEFAULT_STEPS_NUM*/, int v_steps_num /*= DEFAULT_STEPS_NUM*/)
        {
            Math::UniformVectorField uniform(initial_vector);
            return transport_around(&uniform, surface, u_steps_num, v_steps_num);
        }

        Math::Vector DiscreteVectorField::transport_around(const Math::IVectorField * vector_field, const Math::ISurface *surface, int u_steps_num /*= DEFAULT_STEPS_NUM*/, int v_steps_num /*= DEFAULT_STEPS_NUM*/)
        {
            if (u_steps_num < 1 || v_steps_num < 1)
            {
                Logger::error("in DiscreteVectorField::transport_around: steps_num must be 1 or more", __FILE__, __LINE__);
                return Vector::ZERO;
            }

            const ICurvature * curv = space->get_curvature();
            
            // initial value of `res` = vector of `vector_field` at start point of `surface`
            Vector res;
            vector_field->value_at(surface->point_at(ISurface::U_START, ISurface::V_START), res);
            
            Real du = (ISurface::U_END - ISurface::U_START) / u_steps_num;
            Real dv = (ISurface::V_END - ISurface::V_START) / v_steps_num;
            for (int us = 0; us < u_steps_num; ++us)
            {
                for (int vs = 0; vs < v_steps_num; ++vs)
                {
                    Real u = ISurface::U_START + du*us;
                    Real v = ISurface::V_START + du*vs;
                    Point x = surface->point_at(u, v);
                    Vector dx1 = surface->point_at(u+du,v) - x;
                    Vector dx2 = surface->point_at(u,v+dv) - x;

                    CurvatureTensor Rijkm;
                    curv->value_at(x, Rijkm);
#if CAS_UPDATE_DURING_TRANSPORT
                    res += Rijkm.d_parallel_transport(res, dx1, dx2);
#else
                    Vector vec;
                    vector_field->value_at(x, vec);
                    res += Rijkm.d_parallel_transport(vec, dx1, dx2);
#endif // CAS_UPDATE_DURING_TRANSPORT
                }
            }
            return res;

        }

        namespace
        {
            // Makes a surface moved by vector `shift` from `surface;
            class SurfaceShiftDecorator : public ISurface
            {
            private:
                const ISurface * surface;
                Vector shift;
            public:
                SurfaceShiftDecorator(const ISurface * surface, const Vector &shift)
                    : surface(surface), shift(shift) {}

                virtual Point point_at(Real u, Real v) const override
                {
                    return surface->point_at(u, v) + shift;
                }

            };
        }

        void DiscreteVectorField::transport_around_each(const Math::IVectorField * vector_field, const Math::ISurface *surface, int u_steps_num /*= DEFAULT_STEPS_NUM*/, int v_steps_num /*= DEFAULT_STEPS_NUM*/)
        {
            Point start_point = surface->point_at(ISurface::U_START, ISurface::V_START);
            for (int i = 0; i < nodes.size(); ++i)
            {
                Point pos = nodes[i].pos;
                SurfaceShiftDecorator shifted_surface(surface, pos - start_point);
                Vector initial_vector; /*!!!*/
                vector_field->value_at(start_point, initial_vector);
                nodes[i].vector = transport_around(vector_field, &shifted_surface, u_steps_num, v_steps_num) /*!!!*/ - initial_vector;
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
                if (cur_dist <= reqired_dist) // then it is a candidate
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

        void DiscreteVectorField::update_vertices(/*out*/ void *out_vertices, const VertexInfo &vertex_info, unsigned flags /*= POS_TO_CARTESIAN | VECTOR_TO_CARTESIAN*/, int start_vertex /*= 0*/, int vertices_num /*= ALL_VERTICES*/) const
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
                if (flags & POS_TO_CARTESIAN)
                    pos = space->point_to_cartesian(pos);
                VertexInfo::vector_to_vertex_floats(pos, out_pos);
                
                // Update vector
                VertexFloat * out_vector = add_to_pointer(out_vertex, vertex_info.get_vector_offset(0));
                Vector vector = nodes[i].vector;
                if (flags & VECTOR_TO_CARTESIAN)
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
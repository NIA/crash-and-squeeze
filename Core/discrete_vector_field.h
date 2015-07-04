#pragma once
#include "Core/core.h"
#include "Core/vertex_info.h"
#include "Math/diffgeom.h"

// 0: calculate `dv` depending on _initial_ `v` in transport summation
// 1: calculate `dv` depending on _updated_ `v` in transport summation (make v+=dv)
#define CAS_UPDATE_DURING_TRANSPORT 0

namespace CrashAndSqueeze
{
    namespace Core
    {

        class DiscreteVectorField
        {
        private:
            struct VectorAtPos
            {
                Math::Point pos;
                Math::Vector vector;
            };
            Collections::Array<VectorAtPos> nodes;
            const Math::ISpace * space;

            // Return value of find_near if the item was not found
            static const int NOT_FOUND = -1;
            // Find if there is an item with position close to `pos` (at distance <= `dist`) and return its index. Otherwise return NOT_FOUND
            int find_near(Math::Point pos, Math::Real dist) const;
        public:
            DiscreteVectorField(const void *source_vertices,
                                int vertices_num,
                                const VertexInfo &vertex_info,
                                const Math::ISpace * space);

            static const int DEFAULT_STEPS_NUM = 1000;

            // Perform transporting of `initial_vector` along `curve` using connection of `this->space`. Integration is made using `steps_num` steps.
            // Resulting vector is returned and intermediate values are stored in `nodes`
            Math::Vector transport_along(const Math::Vector & initial_vector, const Math::ICurve * curve, int steps_num = DEFAULT_STEPS_NUM);

            // Perform transporting of `initial_vector` along the border of `surface` using curvature tensor of `this->space`. Integration is made using `u_steps_num` steps along u-axis and `v_steps_num` along v-axis
            Math::Vector transport_around(const Math::Vector & initial_vector, const Math::ISurface *surface, int u_steps_num = DEFAULT_STEPS_NUM, int v_steps_num = DEFAULT_STEPS_NUM);

            // TODO: some copy-paste from model.h might be eliminated

            // A special value for `vertices_num` argument meaning "update all vertices"
            static const int ALL_VERTICES = -1;
            // Apply computed positions to given `out_vertices` using `vertex_info`. Only vertices from `start_vertex` to `vertices_num` are updated if these parameters are given
            // If `to_cartesian` is true, then the coordinates and vectors are transformed to Cartesian coordinate system using ISpace
            void update_vertices(/*out*/ void *out_vertices, const VertexInfo &vertex_info, int start_vertex = 0, int vertices_num = ALL_VERTICES, bool to_cartesian = true) const;
            
            ~DiscreteVectorField(void);
        };
    }
}


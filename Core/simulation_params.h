#pragma once
#include "Math/floating_point.h"
#include "Core/core.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // All simulation parameters together in one struct.
        // Used for easier setting them all at once
        struct SimulationParams
        {
            // -- Model parameters --

            // a constant, determining how much deformation velocities are damped:
            // see Model::DEFAULT_DAMPING_CONSTANT
            Math::Real damping_fraction;

            // -- Cluster parameters --

            // - Shape matching parameters -

            // a constant, determining how fast points are pulled to their goal positions
            // see Cluster::DEFAULT_GOAL_SPEED_CONSTANT
            Math::Real goal_speed_fraction;

            // a constant, determining how rigid body is
            // see Cluster::DEFAULT_LINEAR_ELASTICITY_CONSTANT
            Math::Real linear_elasticity_fraction;

            // - Plasticity parameters -

            // a threshold of plastic strain
            // see Cluster::DEFAULT_YIELD_CONSTANT
            Math::Real yield_threshold;

            // a speed of plastic deformation update
            // see Cluster::DEFAULT_CREEP_CONSTANT
            Math::Real creep_speed;

#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
            // a speed of plastic (quadratic!) deformation update
            // see Cluster::DEFAULT_QX_CREEP_CONSTANT
            Math::Real quadratic_creep_speed;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED

            // a threshold of max plastic deformation
            // see Cluster::DEFAULT_MAX_DEFORMATION_CONSTANT
            Math::Real max_deformation;

            // Sets the default values, mentioned in comments to each parameter
            void set_defaults();
        };
    }
}
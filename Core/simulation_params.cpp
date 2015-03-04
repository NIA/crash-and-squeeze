#include "Core/simulation_params.h"
#include "Core/cluster.h"
#include "Core/model.h"

void CrashAndSqueeze::Core::SimulationParams::set_defaults()
{
    damping_fraction = Model::DEFAULT_DAMPING_CONSTANT;
    goal_speed_fraction = Cluster::DEFAULT_GOAL_SPEED_CONSTANT;
    linear_elasticity_fraction = Cluster::DEFAULT_LINEAR_ELASTICITY_CONSTANT;
    yield_threshold = Cluster::DEFAULT_YIELD_CONSTANT;
    creep_speed = Cluster::DEFAULT_CREEP_CONSTANT;
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
    quadratic_creep_speed = Cluster::DEFAULT_QX_CREEP_CONSTANT;
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
    max_deformation = Cluster::DEFAULT_MAX_DEFORMATION_CONSTANT;
}

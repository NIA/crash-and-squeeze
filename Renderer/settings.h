#pragma once
#include <tchar.h>
#include "Core/simulation_params.h"
#include "Math/vector.h"

// kind of "using ::CrashAndSqueeze::Core::SimulationParams;"
typedef ::CrashAndSqueeze::Core::SimulationParams SimulationSettings;

// Fundamental simulation settings that are not local to CrashAndSqueeze::Core::Model
// (and thus are not included into CrashAndSqueeze::Core::SimulationParams)
struct GlobalSettings
{
    static const int AXES_COUNT = ::CrashAndSqueeze::Math::VECTOR_SIZE;
    static const int DEFAULT_CLUSTERS_BY_AXES[AXES_COUNT];

    int clusters_by_axes[AXES_COUNT];
    // TODO: the following parameters are (actually) currently read-only
    bool update_vertices_on_gpu;
    bool quadratic_deformation; // see CAS_QUADRATIC_EXTENSIONS_ENABLED
    bool quadratic_plasticity;  // see CAS_QUADRATIC_PLASTICITY_ENABLED
    bool gfx_transform_total;   // see CAS_GRAPHICAL_TRANSFORM_TOTAL

    void set_defaults();
    void set_clusters_by_axes(const int (&values)[AXES_COUNT]);
    int get_total_clusters_num() const;
};

// Displaying settings
struct RenderSettings
{
    int show_mode;
    bool wireframe;
    // TODO: the following parameters are (actually) currently read-only
    bool show_normals;
    bool paint_clusters;

    void set_defaults();

    enum ShowMode
    {
        SHOW_GRAPHICAL_VERTICES,
        SHOW_CURRENT_POSITIONS,
        SHOW_EQUILIBRIUM_POSITIONS,
        SHOW_INITIAL_POSITIONS,
        _SHOW_MODES_COUNT
    };
    static const TCHAR * SHOW_MODES_CAPTIONS[_SHOW_MODES_COUNT];
};

// Interface that defines methods to handle changing of settings.
// Should be implemented by a class which handles settings in run-time.
// Is also implemented by SettingsStorage
class ISettingsHandler
{
public:
    // Sets given settings as current
    virtual void set_settings(const SimulationSettings &sim,
                              const GlobalSettings &global,
                              const RenderSettings &render) = 0;
    // Gets current settings into given variables
    virtual void get_settings(/*out*/ SimulationSettings &sim,
                              /*out*/ GlobalSettings &global,
                              /*out*/ RenderSettings &render) const = 0;
    // TODO: do we need separate getters/setters for each type of settings?
};

// A storage of settings in a file
// TODO: not yet implemented
class SettingsStorage : public ISettingsHandler
{
public:
    virtual void set_settings(const SimulationSettings &sim, const GlobalSettings &global, const RenderSettings &render) override;
    virtual void get_settings(SimulationSettings &sim, GlobalSettings &global, RenderSettings &render) const override;
};

inline void GlobalSettings::set_defaults()
{
    // TODO: clusters_by_axes = ?
    quadratic_deformation  = CAS_QUADRATIC_EXTENSIONS_ENABLED;
    quadratic_plasticity   = CAS_QUADRATIC_PLASTICITY_ENABLED;
    gfx_transform_total    = CAS_GRAPHICAL_TRANSFORM_TOTAL;
    update_vertices_on_gpu = true;
    set_clusters_by_axes(DEFAULT_CLUSTERS_BY_AXES);
}

inline void RenderSettings::set_defaults()
{
    show_mode      = 0;
    wireframe      = false;
    show_normals   = false;
    paint_clusters = false;
}
#include "settings.h"
#include "Error.h"

const int GlobalSettings::DEFAULT_CLUSTERS_BY_AXES[GlobalSettings::AXES_COUNT] = {2, 3, 4};

const TCHAR * RenderSettings::SHOW_MODES_CAPTIONS[RenderSettings::_SHOW_MODES_COUNT] = 
{
    _T("Graphical vertices"),
    _T("Current positions"),
    _T("Equilibrium positions"),
    _T("Initial positions"),
};

void GlobalSettings::set_clusters_by_axes(const int (&values)[AXES_COUNT])
{
    for (int i = 0; i < AXES_COUNT; ++i)
        clusters_by_axes[i] = values[i];
}

int GlobalSettings::get_total_clusters_num() const
{
    static_assert(3 == AXES_COUNT, "GlobalSettings::get_total_clusters_num assumes AXES_COUNT == 3");
    return clusters_by_axes[0]*clusters_by_axes[1]*clusters_by_axes[2];
}

void SettingsStorage::set_settings(const SimulationSettings &/*sim*/, const GlobalSettings &/*global*/, const RenderSettings &/*render*/)
{
    throw NotYetImplementedError(_T("SettingsStorage not yet impemented"));
}

void SettingsStorage::get_settings(SimulationSettings &/*sim*/, GlobalSettings &/*global*/, RenderSettings &/*render*/) const 
{
    throw NotYetImplementedError(_T("SettingsStorage not yet impemented"));
}

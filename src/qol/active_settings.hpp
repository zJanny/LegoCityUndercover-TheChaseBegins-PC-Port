#pragma once

#include "qol/features.hpp"
#include "qol/settings.hpp"

namespace qol {

const Settings& activeSettings();
void setActiveSettings(const Settings& settings);
bool enabled(Feature feature);

}

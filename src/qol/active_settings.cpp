#include "qol/active_settings.hpp"

namespace qol {

namespace {

Settings& store() {
    static Settings settings;
    return settings;
}

}

const Settings& activeSettings() { return store(); }

void setActiveSettings(const Settings& settings) { store() = settings; }

bool enabled(Feature feature) { return store().enabled(feature); }

}

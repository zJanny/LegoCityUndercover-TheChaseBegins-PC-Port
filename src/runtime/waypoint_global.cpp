#include "runtime/waypoints.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {

namespace {

constexpr const char* kCarryItLevelPrefix = "bits.";
constexpr const char* kCarryItInstance = "waypointforcarryit";

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

void WaypointSet::loadCarryItWaypoint(const assets::Level& bits, const assets::ClassLookup& classOf) {
    carryIt_.clear();
    for (const assets::LevelObject& object : bits.objects) {
        if (lowerCase(object.instance) != kCarryItInstance) continue;
        std::optional<Waypoint> waypoint = buildWaypoint(bits, object, classOf);
        if (!waypoint) return;
        waypoint->instance = std::string(kCarryItLevelPrefix) + kCarryItInstance;
        waypoint->entityEnabled = false;
        carryIt_ = waypoint->instance;
        waypoints_.push_back(std::move(*waypoint));
        return;
    }
}

}

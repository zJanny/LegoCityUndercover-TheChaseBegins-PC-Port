#include "runtime/use_spot.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

void UseSpotSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    spots_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    const std::uint32_t triggerObject = assets::nameHash("TriggerObject"), sendPlayer = assets::nameHash("SendPlayerObject"),
                        startActive = assets::nameHash("UseStartActive");
    for (std::size_t index = 0; index < level.objects.size(); ++index) {
        const assets::LevelObject& object = level.objects[index];
        if (object.className != kUseSpotClass) continue;
        const assets::Level* cls = classOf(object.className);
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!cls || !placed) continue;
        UseSpot spot;
        spot.instance = lower(object.instance);
        spot.objectIndex = index;
        std::copy_n(placed->position, 3, spot.position);
        spot.matrix = assets::placementMatrix(*placed);
        if (const assets::LevelParam* param = effectiveParam(object, *cls, triggerObject)) {
            const auto found = names.find(assets::objectReferenceHash(*param));
            if (found != names.end()) spot.triggerObject = found->second;
        }
        if (const assets::LevelParam* param = effectiveParam(object, *cls, sendPlayer)) spot.sendPlayerObject = param->value != 0;
        if (const assets::LevelParam* param = effectiveParam(object, *cls, startActive)) spot.active = param->value != 0;
        const auto bound = std::find_if(cls->bounds.begin(), cls->bounds.end(), [](const assets::LevelBound& b) { return lower(b.name) == "usebound"; });
        if (bound == cls->bounds.end()) continue;
        std::copy_n(bound->centre, 3, spot.zoneCentre);
        std::copy_n(bound->half, 3, spot.zoneHalf);
        spots_.push_back(std::move(spot));
    }
}

bool UseSpotSet::inZone(const UseSpot& spot, const float player[3]) {
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(player[a] - spot.position[a] - spot.zoneCentre[a]) < spot.zoneHalf[a])) return false;
    return true;
}

std::vector<UseSpotAction> UseSpotSet::update(const assets::PlacedObjectState& state, const float player[3], bool actionPressed, bool playerFree) {
    std::vector<UseSpotAction> actions;
    if (!actionPressed || !playerFree) return actions;
    for (UseSpot& spot : spots_) {
        if (spot.triggerObject.empty() || !spot.active || state.isDisabled(spot.instance) || !inZone(spot, player)) continue;
        actions.push_back({spot.instance, spot.triggerObject, spot.sendPlayerObject});
        spot.active = false;
        break;
    }
    return actions;
}

void UseSpotSet::reactivate(const std::string& lowerInstance) {
    for (UseSpot& spot : spots_)
        if (spot.instance == lowerInstance) spot.active = true;
}

}

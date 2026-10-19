#include "runtime/cat_rescue.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <map>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

AnimationPlay play(const std::string& cat, const char* name, unsigned mode) {
    AnimationPlay start;
    start.instance = cat;
    start.request.name = name;
    start.request.mode = mode;
    start.request.loop = mode == kAnimationLoop;
    start.request.speed = 1.0f;
    start.request.blend = 0.0f;
    return start;
}

}

void CatRescueSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    spots_.clear();
    disabled_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    const std::uint32_t catParam = assets::nameHash("Cat");
    for (const assets::LevelObject& object : level.objects) {
        if (object.className != kCatRescueClass) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        CatRescueSpot spot;
        spot.instance = lower(object.instance);
        if (const assets::LevelParam* param = effectiveParam(object, *cls, catParam)) {
            const auto found = names.find(assets::objectReferenceHash(*param));
            if (found != names.end()) spot.cat = found->second;
        }
        spots_.push_back(std::move(spot));
    }
}

std::vector<AnimationPlay> CatRescueSet::update(float dt) {
    std::vector<AnimationPlay> starts;
    for (CatRescueSpot& spot : spots_) {
        if (spot.state == spot.requested) {
            spot.timer += dt;
            continue;
        }
        const bool linked = !spot.cat.empty();
        switch (spot.requested) {
        case kCatRescueIdle:
            if (linked) starts.push_back(play(spot.cat, "idle_sat", kAnimationLoop));
            break;
        case kCatRescueJump:
            if (linked) starts.push_back(play(spot.cat, "rescuecat_jump", kAnimationOnce));
            break;
        case kCatRescueJumpIdle:
            if (linked) starts.push_back(play(spot.cat, "rescuecat_jump_idle", kAnimationLoop));
            break;
        case kCatRescueRescued:
            if (linked) starts.push_back(play(spot.cat, "rescuecat_rescue", kAnimationOnce));
            break;
        case kCatRescueGone:
            if (linked) disabled_.push_back(spot.cat);
            disabled_.push_back(spot.instance);
            break;
        default:
            break;
        }
        spot.state = spot.requested;
        spot.timer = 0;
    }
    return starts;
}

void CatRescueSet::request(const std::string& lowerSpot, unsigned state) {
    for (CatRescueSpot& spot : spots_)
        if (spot.instance == lowerSpot) spot.requested = state;
}

std::vector<std::string> CatRescueSet::takeDisabled() {
    std::vector<std::string> taken;
    taken.swap(disabled_);
    return taken;
}

std::vector<std::string> CatRescueSet::cats() const {
    std::vector<std::string> result;
    for (const CatRescueSpot& spot : spots_)
        if (!spot.cat.empty()) result.push_back(spot.cat);
    return result;
}

}

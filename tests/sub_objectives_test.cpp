// The sub-objective manager update (src/runtime/sub_objectives.cpp, docs/re/objective-hud.md). Values from the disassembly:
// record update 0x2be934 (seen byte +0x80, removal of a target that was seen enabled and is now disabled or destroyed, centroid
// with the key counted for more than one target, map slot fade over 0x375d38 = 1.0 written by 0x34bb18, deletion of an empty
// record), the arrow update 0x2ed8d8 (timer 0.5 from 0x2c293c, "0.5 - t" on a change of direction, use marker distance 4.0),
// the arrow alpha and anchor of 0x29de50 (lift 1.2, bob sin(t / 0.2) * 0.2 from 0x34bb18). No game data needed.
#include "runtime/sub_objectives.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

struct World {
    std::map<std::string, std::array<float, 3>> positions;
    std::set<std::string> hidden, destroyed, onMap;
    std::map<std::string, std::array<float, 3>> markers;

    SubObjectiveWorld view() {
        SubObjectiveWorld world;
        world.position = [this](const std::string& name, float out[3]) {
            const auto found = positions.find(name);
            if (found == positions.end()) return false;
            for (int i = 0; i < 3; ++i) out[i] = found->second[std::size_t(i)];
            return true;
        };
        world.hidden = [this](const std::string& name) { return hidden.count(name) != 0; };
        world.destroyed = [this](const std::string& name) { return destroyed.count(name) != 0; };
        world.onMap = [this](const float p[3]) {
            for (const std::string& name : onMap)
                if (positions[name][0] == p[0] && positions[name][2] == p[2]) return true;
            return false;
        };
        world.useMarker = [this](const std::string& name, float out[3], float& alpha) {
            const auto found = markers.find(name);
            if (found == markers.end()) return false;
            for (int i = 0; i < 3; ++i) out[i] = found->second[std::size_t(i)];
            alpha = 0.5f;
            return true;
        };
        return world;
    }
};

void records() {
    World world;
    world.positions = {{"wp", {0, 0, 0}}, {"car", {30, 0, 0}}, {"van", {0, 0, 60}}};
    ObjectiveState state;
    state.setSubObjective("wp", "car", 0, true, true);
    state.update(0.25f, world.view());
    const SubObjectiveGroup* group = state.group("wp", 0);
    require(group && near(group->centroid[0], 30.0f) && near(group->centroid[2], 0.0f), "one target: the centroid is the target (no division, 0x2be934)");
    require(group->hasSlot && near(group->slotTimer, 0.25f), "no target on the map: a slot of icon 14 is made and fades in by dt");
    for (int i = 0; i < 5; ++i) state.update(0.25f, world.view());
    require(near(group->slotTimer, 1.0f), "the slot timer stops at 0x375d38 = 1.0");
    state.setSubObjective("wp", "van", 0, true, true);
    state.update(0.25f, world.view());
    require(near(group->centroid[0], 10.0f) && near(group->centroid[2], 20.0f), "two targets: the key is counted, (car + van + wp) / 3");
    world.onMap.insert("car");
    state.update(0.25f, world.view());
    require(group->targetOnMap && group->hasSlot && near(group->slotTimer, 0.75f), "a target on the map: the slot fades out");
    for (int i = 0; i < 3; ++i) state.update(0.25f, world.view());
    require(!group->hasSlot, "the slot is freed when the timer is no longer above 0");
    require(kSubObjectiveSlotIcons[0] == 14 && kSubObjectiveSlotIcons[1] == 13 && kSubObjectiveSlotIcons[2] == 15, "byte table 0x3589fc");

    world.hidden.insert("van");
    state.update(0.25f, world.view());
    require(group->targets.size() == 1 && group->targets[0] == "car", "a target seen enabled is removed once it is disabled");
    world.destroyed.insert("car");
    state.update(0.25f, world.view());
    require(state.groups().empty(), "a destroyed target is removed and the empty record deleted");
}

void seenByte() {
    World world;
    world.positions = {{"wp", {0, 0, 0}}, {"box", {5, 0, 5}}};
    world.hidden.insert("box");
    ObjectiveState state;
    state.setSubObjective("wp", "box", 0, true, false);
    state.update(0.1f, world.view());
    require(state.group("wp", 0) && state.group("wp", 0)->targets.size() == 1, "a target added while disabled stays (byte +0x80 is 0)");
    world.hidden.clear();
    state.update(0.1f, world.view());
    require(state.group("wp", 0)->seenEnabled[0] == 1, "seen enabled once its flag bit 0 is clear");
    world.hidden.insert("box");
    state.update(0.1f, world.view());
    require(state.groups().empty(), "then disabling removes it");

    ObjectiveState fallback;
    fallback.setSubObjective("wp", "box", 0, true, true);
    fallback.setSubObjective("other", "box", 0, false);
    require(fallback.groups().empty() || fallback.group("wp", 0)->targets.empty(),
            "a disable with an unknown key removes the target from the record of the type that lists it (0x1fcc68)");
}

void arrows() {
    World world;
    world.positions = {{"key", {1, 0, 1}}};
    ObjectiveState state;
    state.setArrow(0, "key", true);
    state.setArrow(0, "key", true);
    require(state.arrows()[0].size() == 1 && near(state.arrows()[0][0].timer, 0.5f), "added once with timer 0.5 (0x2c289c)");
    require(objectiveArrowAlphaByte(state.arrows()[0][0], 255) == 0, "fading in: 1 - 2 * 0.5 gives alpha 0");
    state.update(0.25f, world.view());
    require(objectiveArrowAlphaByte(state.arrows()[0][0], 255) == 127, "half way: int(0.5 * 255)");
    state.update(0.25f, world.view());
    require(objectiveArrowAlphaByte(state.arrows()[0][0], 255) == 255, "faded in after 0.5 s; timer 0 shows the base alpha");
    state.requestArrowHide("key");
    state.update(0.1f, world.view());
    require(state.arrows()[0][0].fadingOut && near(state.arrows()[0][0].timer, 0.5f), "a hide request turns it: timer 0.5 - 0");
    require(objectiveArrowAlphaByte(state.arrows()[0][0], 255) == 255, "fading out starts at 2 * 0.5");
    state.update(0.1f, world.view());
    require(!state.arrows()[0][0].fadingOut && near(state.arrows()[0][0].timer, 0.0f), "the request lasts one update: 0.5 - 0.5");
    world.markers["key"] = {1, 0, 3.8f};
    state.update(0.1f, world.view());
    require(state.arrows()[0][0].fadingOut, "the player within 4.0 of the use marker hides it (0x2ed8d8)");
    require(objectiveArrowAlphaByte(state.arrows()[0][0], 128) == 255, "fading out ignores the base alpha");
    world.hidden.insert("key");
    state.update(0.1f, world.view());
    require(state.arrows()[0].empty(), "a disabled target leaves the list");
    state.setArrow(1, "goon", true);
    state.setArrow(1, "boss", true);
    state.setArrow(1, "goon", false);
    require(state.arrows()[1].size() == 1 && state.arrows()[1][0].target == "boss", "a disable moves the last entry into its place (0x1f5fa8)");

    const float node[3] = {2, 10, 3};
    const std::array<float, 3> top = objectiveArrowAnchor(node, 1.15f, 0.85f, 0.0f);
    require(near(top[0], 2) && near(top[1], 13.2f) && near(top[2], 3), "box top 2.0 plus 1.2 (0x375d2c)");
    const float quarter = 0.2f * 3.14159265f * 0.5f;
    require(near(objectiveArrowAnchor(node, 1.15f, 0.85f, quarter)[1], 13.4f), "bob sin(t / 0.2) * 0.2");
    const float right[3] = {1, 0, 0}, up[3] = {0, 1, 0};
    const auto quad = objectiveArrowQuad({0, 3, 0}, right, up);
    require(near(quad[0][0], -0.5f) && near(quad[0][1], 3.5f) && near(quad[2][0], 0.5f) && near(quad[2][1], 2.5f),
            "0x1589fc: corners at -+0.5 along the camera axes, top left first, bottom right third");
    const float marker[3] = {0, 5, 0};
    require(near(objectiveArrowUseMarkerAnchor(marker, 0.0f)[1], 6.2f), "use marker point plus 1.2");
}
}

int main() {
    try {
        records();
        seenByte();
        arrows();
    } catch (const std::exception& error) {
        std::cerr << "sub-objectives-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

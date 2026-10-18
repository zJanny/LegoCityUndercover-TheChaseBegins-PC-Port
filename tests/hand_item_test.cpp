// Hand item track rules on made-up animations (src/runtime/hand_item.cpp, docs/re/abilities-detective.md): the names behind the track, the step
// sampling and the rise / fall events of 0x185668 (branch 0xba1e5b53, REA pseudo code). No game data needed.
#include "runtime/hand_item.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::Animation clip(std::initializer_list<std::pair<float, float>> keys) {
    assets::Animation animation;
    assets::AnimTrack track;
    track.hash = runtime::kHandItemTrackHash;
    for (const auto& [time, value] : keys) {
        assets::CurveKey key;
        key.time = time;
        key.value = value;
        track.keys.push_back(key);
    }
    animation.subAnimations.push_back({track});
    animation.subAnimationTargets.push_back({{0, "Ability_MagnifyingGlass"}, {4, "weaponRight"}});
    return animation;
}

void names() {
    const assets::Animation animation = clip({{0, 1}, {1, 1}});
    const auto item = runtime::handItem(animation);
    require(item && item->object == "Ability_MagnifyingGlass" && item->bone == "weaponRight", "object at offset 0, bone at offset 4");
    require(!runtime::handItem(assets::Animation()), "no track, no item");
    assets::Animation swapped = animation;
    swapped.subAnimationTargets[0] = {{4, "Ability_MagnifyingGlass"}, {0, "weaponRight"}};
    const auto other = runtime::handItem(swapped);
    require(other && other->object == "weaponRight" && other->bone == "Ability_MagnifyingGlass", "the offsets decide, not the order");
    swapped.subAnimationTargets[0] = {{0, "Ability_MagnifyingGlass"}};
    require(!runtime::handItem(swapped), "a missing bone name gives no item");
}

void sampling() {
    const assets::Animation animation = clip({{0, 0}, {0.36f, 1}, {1, 1}});
    require(runtime::handItemTrackValue(animation, 0.0f) == 0.0f, "before the second key");
    require(runtime::handItemTrackValue(animation, 0.359f) == 0.0f, "just before it");
    require(runtime::handItemTrackValue(animation, 0.36f) == 1.0f, "at the key");
    require(runtime::handItemTrackValue(assets::Animation(), 0.5f) == 0.0f, "no track");
}

void events() {
    const assets::Animation enter = clip({{0, 0}, {0.36f, 1}, {1, 1}});
    runtime::HandItemTrack track;
    require(!track.update(enter, 0.0f, true), "the first update of a clip fires nothing");
    require(!track.update(enter, 0.2f, false), "flat zero fires nothing");
    const auto shown = track.update(enter, 0.4f, false);
    require(shown && shown->show && shown->item.object == "Ability_MagnifyingGlass" && shown->item.bone == "weaponRight", "the rise shows the item");
    require(!track.update(enter, 0.9f, false), "flat one fires nothing");

    const assets::Animation leave = clip({{0.001f, 1}, {0.37f, 0}, {1, 0}});
    require(!track.update(leave, 0.0f, true), "a restart takes the first sample without an event");
    const auto again = track.update(leave, 0.2f, false);
    require(again && again->show, "detect_in's first key is after time 0: the step sample is 0 at time 0 and 1 from its key, a rise");
    const auto hidden = track.update(leave, 0.4f, false);
    require(hidden && !hidden->show, "then the fall hides it");
}

void falling() {
    const assets::Animation leave = clip({{0, 1}, {0.37f, 0}, {1, 0}});
    runtime::HandItemTrack track;
    require(!track.update(leave, 0.0f, true), "restart");
    const auto hidden = track.update(leave, 0.4f, false);
    require(hidden && !hidden->show, "the fall hides the item");
    require(!track.update(leave, 0.8f, false), "flat zero after the fall");
}
}

int main() {
    try {
        names();
        sampling();
        events();
        falling();
    } catch (const std::exception& error) {
        std::cerr << "hand-item-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "hand-item-test: ok\n";
    return 0;
}

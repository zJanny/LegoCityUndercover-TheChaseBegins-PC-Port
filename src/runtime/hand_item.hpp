#pragma once
// Items that an animation shows in the character's hand: the track 0xba1e5b53 of a clip names an object (a child of the character, e.g.
// Ability_MagnifyingGlass in the detect_* clips of the player) and a bone. The character's event handler 0x185668 (branch for the track hash,
// 0x1d5c24) attaches the object to the bone and shows it when the track value rises and hides it when it falls (docs/re/abilities-detective.md).
// No SDL, no rendering.
#include "assets/assets.hpp"
#include <cstdint>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr std::uint32_t kHandItemTrackHash = 0xba1e5b53;
constexpr const char* kMagnifyingGlassObject = "Ability_MagnifyingGlass";

struct HandItem {
    std::string object, bone;
};

// The object and bone names of the clip's hand item track; nothing when the clip has none.
std::optional<HandItem> handItem(const assets::Animation& animation);

// Value of the track at the normalised animation time, sampled as a step function (the footstep track's sampling,
// assets::evaluateTrack is not used: STAND-IN DETECT-S7). Zero when the clip has no hand item track.
float handItemTrackValue(const assets::Animation& animation, float normalisedTime);

struct HandItemEvent {
    bool show = false;
    HandItem item;
};

// The edge detector of one animation slot: the event of 0x1d5c24 is a rise (show) or a fall (hide) of the track value between two updates.
class HandItemTrack {
public:
    // One update of the slot playing `animation` at `normalisedTime`; `restarted` when the slot just started a clip.
    std::optional<HandItemEvent> update(const assets::Animation& animation, float normalisedTime, bool restarted);

private:
    float previous_ = 0;
};

}

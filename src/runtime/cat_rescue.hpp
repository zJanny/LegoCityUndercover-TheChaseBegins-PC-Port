#pragma once
// The cat rescue use spot (props/ability_freeplay_fireman_cat_usespot, kind 124, docs/re/objects.md): create 0x1829ac, link reader 0x176e04,
// clip loader 0x182a5c, update 0x182b38 -> state change 0x1c6aa8. Its `Cat` object plays the clip of the spot's state. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/natives_animation.hpp"

#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kCatRescueClass = "props/ability_freeplay_fireman_cat_usespot";

enum CatRescueState : unsigned {
    kCatRescueIdle = 1,
    kCatRescueJump = 4,
    kCatRescueJumpIdle = 6,
    kCatRescueRescued = 7,
    kCatRescueGone = 8,
};

struct CatRescueSpot {
    std::string instance;  // lower case
    std::string cat;       // lower case instance of the `Cat` link, empty when the link does not resolve
    unsigned state = 0;
    unsigned requested = kCatRescueIdle;
    float timer = 0;
};

class CatRescueSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    // 0x1c6aa8: a spot whose state differs from the requested one changes state (the clip of the new state starts on the cat, state 8
    // disables the spot and the cat); otherwise its timer advances by dt. Returns the clip starts in spot order.
    std::vector<AnimationPlay> update(float dt);
    // The requested state is record +4, written by the player's rescue code (not ported).
    void request(const std::string& lowerSpot, unsigned state);
    // Spots and cats that state 8 disabled since the last call.
    std::vector<std::string> takeDisabled();
    // Lower-case instances of the linked cats, the objects that need a skinned prop.
    std::vector<std::string> cats() const;
    const std::vector<CatRescueSpot>& spots() const { return spots_; }

private:
    std::vector<CatRescueSpot> spots_;
    std::vector<std::string> disabled_;
};

}

#pragma once
// The items a character's clips have shown or hidden so far: one entry per object name that a hand item track event (hand_item.hpp) named.
// The original finds the object by name under the character at each event (0x2450f0), attaches it to the event's bone (0x22acdc) and shows
// or hides it (0x242cf0, 0x2428c0); the set keeps that state for every name, so no item is special (docs/re/abilities-detective.md).
// Names compare without case. No SDL, no rendering.
#include "runtime/hand_item.hpp"
#include <string>
#include <vector>

namespace chase::runtime {

std::string handItemKey(const std::string& name);

struct HandItemState {
    std::string object, bone;
    bool shown = false;
};

class HandItemSet {
public:
    struct Applied {
        const HandItemState* state;
        bool created;
    };
    Applied apply(const HandItemEvent& event);
    const HandItemState* find(const std::string& object) const;
    const std::vector<HandItemState>& items() const { return items_; }

private:
    std::vector<HandItemState> items_;
};

}

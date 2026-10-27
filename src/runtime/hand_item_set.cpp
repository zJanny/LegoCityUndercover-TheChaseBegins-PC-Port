#include "runtime/hand_item_set.hpp"
#include <algorithm>
#include <cctype>

namespace chase::runtime {

std::string handItemKey(const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return key;
}

HandItemSet::Applied HandItemSet::apply(const HandItemEvent& event) {
    const std::string key = handItemKey(event.item.object);
    for (HandItemState& state : items_) {
        if (state.object != key) continue;
        state.shown = event.show;
        if (event.show) state.bone = handItemKey(event.item.bone);
        return {&state, false};
    }
    HandItemState state;
    state.object = key;
    state.shown = event.show;
    if (event.show) state.bone = handItemKey(event.item.bone);
    items_.push_back(std::move(state));
    return {&items_.back(), true};
}

const HandItemState* HandItemSet::find(const std::string& object) const {
    const std::string key = handItemKey(object);
    for (const HandItemState& state : items_)
        if (state.object == key) return &state;
    return nullptr;
}

}

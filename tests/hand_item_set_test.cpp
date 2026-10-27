// The state of the items that hand item track events show and hide (src/runtime/hand_item_set.cpp): every object name is one entry, names compare
// without case, the bone of the last show stays. No game data needed.
#include "runtime/hand_item_set.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

runtime::HandItemEvent event(bool show, const char* object, const char* bone) {
    runtime::HandItemEvent result;
    result.show = show;
    result.item.object = object;
    result.item.bone = bone;
    return result;
}

void showAndHide() {
    runtime::HandItemSet set;
    require(set.items().empty() && !set.find("Ability_Handcuffs"), "nothing before the first event");
    const auto first = set.apply(event(true, "Ability_Handcuffs", "weaponRight"));
    require(first.created && first.state->shown && first.state->object == "ability_handcuffs" && first.state->bone == "weaponright", "a show creates the entry with its bone");
    const auto again = set.apply(event(true, "ability_handcuffs", "WEAPONRIGHT"));
    require(!again.created && set.items().size() == 1, "the same name without case is the same entry");
    const auto hidden = set.apply(event(false, "Ability_Handcuffs", "weaponRight"));
    require(!hidden.created && !hidden.state->shown && hidden.state->bone == "weaponright", "a hide keeps the bone");
    require(set.find("ABILITY_HANDCUFFS") && !set.find("ABILITY_HANDCUFFS")->shown, "lookup without case");
}

void severalItems() {
    runtime::HandItemSet set;
    set.apply(event(true, "Ability_Hammer", "weaponRight"));
    set.apply(event(true, "Ability_Whistle", "weaponRight"));
    set.apply(event(true, "Ability_Fish", "weaponLeft"));
    set.apply(event(false, "Ability_Whistle", "weaponRight"));
    require(set.items().size() == 3, "one entry per name");
    require(set.find("Ability_Hammer")->shown && !set.find("Ability_Whistle")->shown && set.find("Ability_Fish")->shown, "the items are independent");
    require(set.find("Ability_Fish")->bone == "weaponleft", "each item has its own bone");
    const auto lateHide = set.apply(event(false, "Ability_TeaCup", "weaponRight"));
    require(lateHide.created && !lateHide.state->shown && lateHide.state->bone.empty(), "a hide of an unknown item shows nothing and sets no bone");
}
}

int main() {
    try {
        showAndHide();
        severalItems();
    } catch (const std::exception& error) {
        std::cerr << "hand-item-set-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "hand-item-set-test: ok\n";
    return 0;
}

// The arrows of the carry system (src/runtime/carry_arrows.cpp) on a synthetic set: the carry object update 0x16d420 (usable zone: carried leaves,
// free and not FreeplayObject joins), the carry target update 0x19cf5c (joins while the player carries its UseObject) and the hide of the use
// hint 0x1cba8c / 0x2b9c28. Values from the disassembly (llvm-objdump of game.arm.elf, 20261025-recorded-C, 2026-10-25), docs/re/carrying.md.
#include "runtime/carry_arrows.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

CarryObject object(const std::string& name, float x) {
    CarryObject carry;
    carry.instance = name;
    carry.home[0] = carry.position[0] = x;
    carry.zoneHalf[0] = carry.zoneHalf[1] = carry.zoneHalf[2] = 1.5f;
    carry.hasZone = true;
    return carry;
}

bool wants(const CarryArrowUpdate& update, const std::string& name, bool enable) {
    return std::any_of(update.requests.begin(), update.requests.end(), [&](const CarryArrowRequest& r) { return r.instance == name && r.enable == enable; });
}

bool asks(const CarryArrowUpdate& update, const std::string& name) {
    return std::any_of(update.requests.begin(), update.requests.end(), [&](const CarryArrowRequest& r) { return r.instance == name; });
}

CarryInput at(float x, bool press) {
    CarryInput input;
    input.player[0] = x;
    input.actionPressed = press;
    return input;
}
}

int main() {
    try {
        CarrySet set;
        set.add(object("key", 0));
        CarryObject freeplay = object("freeplay", 20);
        freeplay.freeplay = true;
        set.add(freeplay);
        CarryObject unusable = object("unusable", 40);
        unusable.usable = false;
        set.add(unusable);
        set.add(object("hidden", 60));
        CarryTarget hole;
        hole.instance = "hole";
        hole.useObject = "key";
        hole.position[0] = 10;
        hole.zoneHalf[0] = hole.zoneHalf[1] = hole.zoneHalf[2] = 2;
        hole.hasZone = true;
        set.add(hole);
        chase::assets::PlacedObjectState state;
        state.disabled.insert("hidden");

        const float far[3] = {100, 0, 0}, atKey[3] = {0.5f, 0, 0}, atHole[3] = {10.5f, 0, 0};
        CarryArrowUpdate update = carryArrows(set, state, far, true);
        require(wants(update, "key", true), "a free, usable carry object joins the carry group and list 2 (0x16d690)");
        require(!asks(update, "freeplay"), "a free FreeplayObject (record +0x9e bit 7) makes no request");
        require(!asks(update, "unusable"), "without the usable bit (record +4 bit 4, UseStartActive) the update makes no request");
        require(!asks(update, "hidden"), "STAND-IN LEVOBJ-S99: a disabled object makes no request");
        require(wants(update, "hole", false), "a target leaves while the player does not carry its UseObject (0x19cffc)");
        require(update.hide.empty(), "no hide away from every zone");

        update = carryArrows(set, state, atKey, true);
        require(update.hide == "key", "the use hint hides the arrow of the object the A press would pick up (0x2b9c28)");
        require(carryArrows(set, state, atKey, false).hide.empty(), "no hide while the hint gate is closed");

        set.update(state, at(0.5f, true));
        for (unsigned i = 0; i <= CarryTiming{}.pickupEnd; ++i) set.update(state, at(0.5f, false));
        require(set.carriedIndex() == 0, "the key is carried");
        update = carryArrows(set, state, atHole, true);
        require(wants(update, "key", false), "a carried object leaves the group and the list (0x16d65c)");
        require(wants(update, "hole", true), "the target joins while the player carries its UseObject (0x19cfd4)");
        require(update.hide == "hole", "the use hint hides the arrow of the target that takes the carried object");

        chase::assets::Level level;
        ScriptRunner scripts(level, state);
        applyCarryArrows(carryArrows(set, state, far, true), scripts);
        const auto& list = scripts.objectives().arrows()[2];
        require(list.size() == 1 && list[0].target == "hole", "only the target sits in arrow list 2 while the key is carried");
        const SubObjectiveGroup* group = scripts.objectives().group("", 2);
        require(group && group->targets.size() == 1 && group->targets[0] == "hole", "the carry group (type 2) of the carry-it key holds the target");
    } catch (const std::exception& error) {
        std::cerr << "carry-arrows-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "carry-arrows-test: ok\n";
    return 0;
}

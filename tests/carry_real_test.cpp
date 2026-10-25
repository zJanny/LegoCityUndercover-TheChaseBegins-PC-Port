// Carrying in lvl_01_cherrytreehills (src/runtime/carry.cpp): the class values of 1-1_DonutsDEMO and 1-1_DonutTarget_DEMO, the use zone of the
// box measured in the oracle save state carrying the donut box in mission 1-1 (docs/re/carrying.md), and mission 1-1 steps 9 and 10: after 1-1_Carry_Object_Tutorial
// the box is picked up (ObjectTriggered of 1-1_SO_Donuts_PickedUp) and placed into the target (1-1_Deliver_Donuts by ObjectBoundEntered,
// ObjectTriggered of 1-1_SO_MissionEnd). Player positions are those where the oracle runs pressed the action button. Needs the extracted
// Mission 1-3: the key and the keyhole, their zones (oracle) and the carry arrow. Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/carry.hpp"
#include "runtime/carry_arrows.hpp"
#include "runtime/level_start.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: carry-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "carry-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        runtime::CarrySet set;
        set.load(archive, level, classOf);
        const runtime::CarryObject* box = set.find("1-1_donutsdemo");
        require(box != nullptr, "1-1_DonutsDEMO is a carry object");
        require(box->triggerPickup == "1-1_so_donuts_pickedup", "TriggerPickup " + box->triggerPickup);
        require(!box->small && !box->strengthOnly, "ATTR_SmallObject 0 (a big object), not strength only");
        require(box->resetUpdates == 150, "ResetTime 5 gives 150 updates");
        const float half[3] = {1.7607f, 1.6064f, 1.3828f};
        for (int a = 0; a < 3; ++a) require(near(box->zoneHalf[a], half[a], 0.01f), "zone half " + std::to_string(a) + ": " + std::to_string(box->zoneHalf[a]));
        require(near(box->zoneCentre[1], 0.4073f, 0.01f), "zone centre y " + std::to_string(box->zoneCentre[1]));
        require(!set.targets().empty(), "targets");
        const runtime::CarryTarget* target = nullptr;
        for (const runtime::CarryTarget& candidate : set.targets())
            if (candidate.instance == "1-1_donuttarget_demo") target = &candidate;
        require(target != nullptr, "1-1_DonutTarget_DEMO is a carry target");
        require(target->useObject == "1-1_donutsdemo", "UseObject " + target->useObject);
        require(target->triggerObject == "1-1_so_missionend", "TriggerObject " + target->triggerObject);
        require(target->sfx == 72, "ATTR_SFX_DROP 72");
        require(target->hasZone && near(target->zoneHalf[0], 2.0f, 0.001f) && near(target->zoneCentre[1], 0.5f, 0.001f), "UseBound of the class file");

        const float pickupPoint[3] = {817.58f, 23.924f, 1071.94f}, farPoint[3] = {817.58f, 23.924f, 1075.5f};
        require(runtime::CarrySet::inZone(box->zoneCentre, box->zoneHalf, box->position, pickupPoint), "the oracle pickup position is in the zone");
        require(!runtime::CarrySet::inZone(box->zoneCentre, box->zoneHalf, box->position, farPoint), "3.5 m away is outside");
        const float beforeTarget[3] = {881.396f, 16.597f, 1118.883f}, inTarget[3] = {881.897f, 16.597f, 1120.189f};
        require(!runtime::CarrySet::inZone(target->zoneCentre, target->zoneHalf, target->position, beforeTarget),
                "the save state in front of the donut delivery target of mission 1-1 stands outside the use zone (the oracle's A press there dropped the box)");
        require(runtime::CarrySet::inZone(target->zoneCentre, target->zoneHalf, target->position, inTarget), "the oracle placing position is in the zone");

        assets::PlacedObjectState state = runtime::levelStartState(level, classOf);
        runtime::ScriptRunner scripts(level, state);
        scripts.setObjectPositionSource([&](const std::string& instance, float out[3]) { return set.position(instance, out); });
        require(state.isDisabled("1-1_DonutsDEMO"), "the box is off before the tutorial");
        require(scripts.start("1-1_Carry_Object_Tutorial"), "1-1_Carry_Object_Tutorial exists");
        for (int i = 0; i < 4000 && scripts.threadCount() > 0; ++i) scripts.update(0.034f);
        require(!state.isDisabled("1-1_DonutsDEMO"), "the tutorial enables the box");

        const auto step = [&](const float player[3], bool press) {
            runtime::CarryInput input;
            for (int a = 0; a < 3; ++a) input.player[a] = player[a];
            input.actionPressed = press;
            runtime::applyCarryActions(set, set.update(state, input), state, scripts);
            scripts.checkTriggers(player);
            scripts.update(0.034f);
        };
        step(pickupPoint, true);
        bool pickedScript = false;
        for (unsigned i = 0; i <= runtime::CarryTiming{}.pickupEnd; ++i) {
            step(pickupPoint, false);
            pickedScript = pickedScript || scripts.running("1-1_SO_Donuts_PickedUp", false);
        }
        require(pickedScript, "the pickup starts 1-1_SO_Donuts_PickedUp");
        require(set.carryingMode() == 2, "carrying mode 2 after the pickup");

        step(inTarget, true);
        const auto deliverTip = [&]() { return scripts.text().tip().active && scripts.text().tip().key == 4094106651u; };
        bool deliver = deliverTip(), missionEnd = false;
        for (unsigned i = 0; i <= runtime::CarryTiming{}.placeEnd; ++i) {
            step(inTarget, false);
            deliver = deliver || deliverTip();
            missionEnd = missionEnd || scripts.running("1-1_SO_MissionEnd", false);
        }
        require(deliver, "the box in the delivery bound starts 1-1_Deliver_Donuts");
        require(missionEnd, "the target starts 1-1_SO_MissionEnd");
        require(state.isDisabled("1-1_DonutsDEMO"), "the target switches the box off");
        require(set.carryingMode() == 0, "nothing carried afterwards");

        // Mission 1-3, the key (props/prop_carry with the class mesh Prop_Key) and the keyhole (props/prop_keyhole, ObjectType 123). Oracle values of
        // RPC request 20261025-recorded-C-1 (20261025-recorded-servant, 2026-10-25, recorded from a save state in front of the key and one with the key in hand):
        // zone record centre (0, 0.3248, 0.0509), half (1.1213, 1.4386, 1.4763); keyhole zone centre (0, 0.5, 1.2), half (1, 1.5, 1); record +0x1c of the
        // keyhole is the key (PlacedObject), +0x24 dropAnim 710.
        const runtime::CarryObject* key = set.find("1-3_garage_key");
        require(key != nullptr && key->hasZone, "1-3_Garage_Key has a use zone from its class mesh");
        require(key->small && key->usable && !key->freeplay && key->triggerPickup.empty(), "the key is small, usable, not freeplay, without TriggerPickup");
        const float keyHalf[3] = {1.1213f, 1.4386f, 1.4763f}, keyCentre[3] = {0.0f, 0.3248f, 0.0509f};
        for (int a = 0; a < 3; ++a)
            require(near(key->zoneHalf[a], keyHalf[a], 0.001f) && near(key->zoneCentre[a], keyCentre[a], 0.001f), "key zone axis " + std::to_string(a));
        const runtime::CarryTarget* hole = nullptr;
        for (const runtime::CarryTarget& candidate : set.targets())
            if (candidate.instance == "1-3_garage_key_base") hole = &candidate;
        require(hole != nullptr, "the keyhole is a carry target (ObjectType 123, class props/prop_keyhole)");
        require(hole->useObject == "1-3_garage_key" && hole->placedObject == "1-3_garage_key" && hole->triggerObject == "1-3_garage_key_base_script",
                "keyhole references: UseObject and PlacedObject the key, TriggerObject 1-3_Garage_Key_Base_Script");
        require(hole->hasZone && near(hole->zoneCentre[2], 1.2f, 0.001f) && near(hole->zoneHalf[1], 1.5f, 0.001f), "keyhole UseBound");
        const float userSpot[3] = {987.808f, 16.644f, 1143.306f}, walked[3] = {986.756f, 16.624f, 1142.715f};
        require(!runtime::CarrySet::inZone(key->zoneCentre, key->zoneHalf, key->position, userSpot),
                "the save state in front of the key stands 1.71 from it, outside the zone: A does nothing there in the original either");
        require(runtime::CarrySet::inZone(key->zoneCentre, key->zoneHalf, key->position, walked), "the oracle's pickup position is inside the zone");

        assets::PlacedObjectState keyState = runtime::levelStartState(level, classOf);
        require(keyState.isDisabled("1-3_Garage_Key"), "the key is hidden until the dirt mound reveals it");
        keyState.disabled.erase("1-3_garage_key");
        require(runtime::carryArrows(set, keyState, userSpot, true).hide.empty(), "no hide outside the zone");
        const runtime::CarryArrowUpdate free = runtime::carryArrows(set, keyState, walked, true);
        const auto requested = [](const runtime::CarryArrowUpdate& update, const std::string& name, bool enable) {
            for (const runtime::CarryArrowRequest& request : update.requests)
                if (request.instance == name && request.enable == enable) return true;
            return false;
        };
        require(requested(free, "1-3_garage_key", true) && requested(free, "1-3_garage_key_base", false),
                "before the pickup the carry arrow (list 2) and the carry group are on the key (oracle: list 2 = {key})");
        require(free.hide == "1-3_garage_key", "inside the zone the use hint hides the key's arrow");
    } catch (const std::exception& error) {
        std::cerr << "carry-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "carry-real-test: ok\n";
    return 0;
}

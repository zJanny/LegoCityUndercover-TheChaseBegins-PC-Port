// Carrying rules (src/runtime/carry.cpp) on a synthetic level: pickup in the use zone, the carried state, drop, placing into the target that
// names the object, the delayed effect of the target and the reset of a dropped object. Values: docs/re/carrying.md.
#include "runtime/carry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

CarrySet makeSet() {
    CarrySet set;
    CarryObject box;
    box.instance = "box";
    box.home[0] = box.position[0] = 10;
    box.home[2] = box.position[2] = 10;
    box.zoneCentre[1] = 0.4f;
    box.zoneHalf[0] = 1.76f;
    box.zoneHalf[1] = 1.6f;
    box.zoneHalf[2] = 1.38f;
    box.hasZone = true;
    box.small = false;
    box.resetUpdates = 150;
    box.triggerPickup = "picked";
    set.add(box);
    CarryTarget target;
    target.instance = "target";
    target.useObject = "box";
    target.triggerObject = "done";
    target.position[0] = 30;
    target.position[2] = 30;
    target.zoneCentre[1] = 0.5f;
    target.zoneHalf[0] = target.zoneHalf[1] = target.zoneHalf[2] = 2;
    target.hasZone = true;
    set.add(target);
    return set;
}

CarryInput at(float x, float z, bool press) {
    CarryInput input;
    input.player[0] = x;
    input.player[2] = z;
    input.actionPressed = press;
    return input;
}

unsigned count(const std::vector<CarryAction>& actions, CarryActionKind kind) {
    unsigned n = 0;
    for (const CarryAction& action : actions) n += action.kind == kind;
    return n;
}
}

int main() {
    try {
        chase::assets::PlacedObjectState state;
        {
            CarrySet set = makeSet();
            require(set.update(state, at(14, 10, true)).empty() && !set.busy(), "outside the zone: nothing");
            require(set.update(state, at(11.4f, 10.7f, false)).empty() && !set.busy(), "inside without the button: nothing");
            set.update(state, at(11.4f, 10.7f, true));
            require(set.phase() == CarryPhase::PickingUp && set.busy(), "button in the zone starts the pickup");
            unsigned picked = 0;
            for (unsigned update = 0; update < 30 && set.busy(); ++update)
                picked += count(set.update(state, at(11.4f, 10.7f, true)), CarryActionKind::PickedUp);
            require(picked == 1, "the pickup fired once");
            require(set.carryingMode() == 2 && set.carriedIndex() == 0, "big object: carrying mode 2");
            require(!set.busy(), "the pickup ended");
        }
        {
            CarrySet set = makeSet();
            chase::assets::PlacedObjectState empty;
            empty.disabled.insert("box");
            set.update(empty, at(11, 10, true));
            require(!set.busy(), "a disabled object cannot be picked up");
        }
        {
            CarrySet set = makeSet();
            set.update(state, at(11, 10, true));
            unsigned attachedAfter = 0, updates = 0;
            while (set.busy() && updates < 50) {
                ++updates;
                if (count(set.update(state, at(11, 10, false)), CarryActionKind::PickedUp)) attachedAfter = updates;
            }
            require(attachedAfter == CarryTiming{}.pickupAttach, "attached after " + std::to_string(attachedAfter));
            require(updates == CarryTiming{}.pickupEnd, "pickup ended after " + std::to_string(updates));
            float position[3];
            require(set.position("box", position), "position of the box");
            set.update(state, at(20, 20, true));
            require(set.phase() == CarryPhase::Dropping, "button far from the target drops");
            unsigned released = 0;
            updates = 0;
            while (set.busy() && updates < 50) {
                ++updates;
                if (count(set.update(state, at(20, 20, false)), CarryActionKind::Dropped)) released = updates;
            }
            require(released == CarryTiming{}.dropRelease, "dropped after " + std::to_string(released));
            require(set.carryingMode() == 0, "nothing carried after the drop");
            set.position("box", position);
            require(position[0] == 20 && position[2] == 20, "dropped at the player");
            unsigned respawn = 0;
            for (unsigned i = 1; i <= 200 && !respawn; ++i)
                if (count(set.update(state, at(0, 0, false)), CarryActionKind::Respawned)) respawn = i;
            require(respawn > 0, "the dropped object returns to its start");
            set.position("box", position);
            require(position[0] == 10 && position[2] == 10, "back at the start position");
        }
        {
            // Provenance: the respawn function 0x22fc84 calls 0x244f40(record +0x14 = TriggerOnRespawn, object) first (llvm-objdump, docs/re/carrying.md).
            chase::assets::Level level;
            chase::assets::LevelScript script;
            script.nameHash = chase::assets::nameHash("on_respawn");
            script.calls.push_back({});
            level.scripts.push_back(script);
            chase::assets::LevelTrigger trigger;
            trigger.event = "ObjectTriggered";
            trigger.objectName = "Reset_Hook";
            trigger.scriptName = "on_respawn";
            level.triggers.push_back(trigger);
            chase::assets::PlacedObjectState hookState;
            hookState.disabled.insert("reset_hook");
            ScriptRunner runner(level, hookState);
            CarrySet set = makeSet();
            CarryObject hooked = set.objects()[0];
            hooked.instance = "hooked";
            hooked.triggerOnRespawn = "reset_hook";
            hooked.triggerPickup.clear();
            CarrySet hookedSet;
            hookedSet.add(hooked);
            applyCarryActions(hookedSet, {{CarryActionKind::Respawned, 0, 0}}, hookState, runner);
            require(!hookState.isDisabled("reset_hook"), "the respawn trigger enables its object");
            const float away[3] = {0, 0, 0};
            runner.checkTriggers(away);
            require(runner.threadCount() == 1, "the respawn fires TriggerOnRespawn");
        }
        {
            chase::assets::Level level;
            chase::assets::PlacedObjectState placeState;
            placeState.disabled.insert("place_marker");
            ScriptRunner runner(level, placeState);
            CarrySet set = makeSet();
            CarryTarget placing = set.targets()[0];
            placing.placeObject = "place_marker";
            CarrySet placeSet;
            placeSet.add(placing);
            applyCarryActions(placeSet, {{CarryActionKind::Used, 0, 0}}, placeState, runner);
            require(!placeState.isDisabled("place_marker"), "using the target enables its PlaceObject (0x25e354 calls 0x242cf0 on +0x1c)");
        }
        {
            CarrySet set = makeSet();
            set.update(state, at(11, 10, true));
            while (set.busy()) set.update(state, at(11, 10, false));
            set.update(state, at(31.5f, 31.0f, true));
            require(set.phase() == CarryPhase::Placing, "button in the target zone places");
            unsigned placed = 0, used = 0, updates = 0;
            while (updates < 60) {
                ++updates;
                const std::vector<CarryAction> actions = set.update(state, at(31.5f, 31.0f, false));
                if (count(actions, CarryActionKind::Placed)) placed = updates;
                if (count(actions, CarryActionKind::Used)) used = updates;
                if (used) break;
            }
            require(placed == CarryTiming{}.placeRelease, "placed after " + std::to_string(placed));
            require(used == placed + 1, "the target effect runs one update after the release");
            require(set.targets()[0].used, "target used");
        }
        {
            CarrySet set = makeSet();
            set.update(state, at(11, 10, true));
            while (set.busy()) set.update(state, at(11, 10, false));
            set.update(state, at(34.5f, 30.0f, true));
            require(set.phase() == CarryPhase::Dropping, "outside the 2 m zone of the target the button drops");
        }
        {
            CarrySet set = makeSet();
            CarryInput busy = at(11, 10, true);
            busy.playerFree = false;
            set.update(state, busy);
            require(!set.busy(), "a player in another state does not take the button");
        }
        {
            CarrySet set = makeSet();
            CarryTiming slow;
            slow.pickupAttach = 3;
            slow.pickupEnd = 5;
            set.setTiming(false, slow);
            float turn[3];
            set.update(state, at(11, 10, true));
            require(set.turnTarget(turn) && turn[0] == 10 && turn[2] == 10, "the pickup turns the player towards the box");
            require(set.phaseClip() == kAnimationPickupBig, "the pickup plays the big pickup clip");
            set.update(state, at(11, 10, false));
            require(!set.turnTarget(turn), "the turn is set once");
            for (unsigned i = 0; i < 2; ++i) set.update(state, at(11, 10, false));
            require(set.carryingMode() == 2, "attached after the configured update");
            CarryInput hand = at(11, 10, false);
            hand.hasHand = true;
            hand.hand[0] = 1;
            hand.hand[1] = 2;
            hand.hand[2] = 3;
            set.update(state, hand);
            float position[3];
            set.position("box", position);
            require(position[0] == 1 && position[1] == 2 && position[2] == 3, "a carried object follows the hand position");
            while (set.busy()) set.update(state, hand);
            set.update(state, at(30, 30, true));
            require(set.turnTarget(turn) && turn[0] == 30, "placing turns the player towards the target");
            require(set.phaseClip() == kAnimationPutdownBig, "placing plays the put-down clip");
        }
        {
            std::vector<float> rotation = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 2, 3, 4, 1};
            chase::assets::Model model;
            model.nodeNames = {"Box", "carryhandle"};
            model.nodeMatrices = {std::array<float, 16>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, {}};
            std::copy(rotation.begin(), rotation.end(), model.nodeMatrices[1].begin());
            const std::array<float, 16> offset = carryHandOffset(model);
            const float expected[16] = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, -3, 2, -4, 1};
            for (int i = 0; i < 16; ++i) require(std::fabs(offset[std::size_t(i)] - expected[i]) < 1e-6f, "hand offset element " + std::to_string(i));
            require(carryHandOffset(chase::assets::Model())[0] == 1, "no handle node gives the identity");
        }
    } catch (const std::exception& error) {
        std::cerr << "carry-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "carry-test: ok\n";
    return 0;
}

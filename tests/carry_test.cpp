// Carrying rules (src/runtime/carry.cpp) on a synthetic level: pickup in the use zone, the carried state, drop, placing into the target that
// names the object, the delayed effect of the target and the reset of a dropped object. Values: docs/re/carrying.md.
#include "runtime/carry.hpp"
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
            require(attachedAfter == kCarryPickupAttachUpdates, "attached after " + std::to_string(attachedAfter));
            require(updates == kCarryPickupEndUpdates, "pickup ended after " + std::to_string(updates));
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
            require(released == kCarryDropReleaseUpdates, "dropped after " + std::to_string(released));
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
            require(placed == kCarryPlaceReleaseUpdates, "placed after " + std::to_string(placed));
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
    } catch (const std::exception& error) {
        std::cerr << "carry-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "carry-test: ok\n";
    return 0;
}

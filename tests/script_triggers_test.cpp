// Trigger check functions (src/runtime/script_triggers.cpp). Reference values are the branch structure of the disassembly
// of game.arm.elf (docs/re/script-triggers.md): 0x28aef4, 0x26a20c, 0x282028, 0x28af68, 0x2ca6b8, 0x2cb578, 0x2c194c,
// 0x2bc67c, 0x2aae30, 0x2b7ecc.
#include "runtime/script_triggers.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

using chase::runtime::TriggerEvent;
using chase::runtime::TriggerRecord;
using chase::runtime::TriggerSample;
using chase::runtime::applyTriggerCheck;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

TriggerSample player(bool inside, bool vehicle = false) {
    TriggerSample sample;
    sample.playerInside = inside;
    sample.playerInVehicle = vehicle;
    return sample;
}

TriggerSample object(bool inside, bool usable = true) {
    TriggerSample sample;
    sample.objectUsable = usable;
    sample.objectInside = inside;
    return sample;
}

void classification() {
    using chase::runtime::classifyTriggerEvent;
    require(classifyTriggerEvent("boundentered") == TriggerEvent::BoundEntered, "BoundEntered");
    require(classifyTriggerEvent("player1boundentered") == TriggerEvent::BoundEntered, "Player1BoundEntered is BoundEntered");
    require(classifyTriggerEvent("objectboundleft") == TriggerEvent::ObjectBoundLeft, "ObjectBoundLeft");
    require(classifyTriggerEvent("aimove") == TriggerEvent::Unmodelled, "AIMove has no bound check");
    require(classifyTriggerEvent("objecttriggered") == TriggerEvent::Unmodelled, "ObjectTriggered is fired by code");
}

void boundEnteredIsAnEdgeAfterTheFirstCheck() {
    TriggerRecord record;
    require(!applyTriggerCheck(TriggerEvent::BoundEntered, record, player(true)), "first check (skip) never fires");
    require(record.inside && !record.skip, "first check stores the state and clears skip");
    require(!applyTriggerCheck(TriggerEvent::BoundEntered, record, player(true)), "staying inside does not fire");
    applyTriggerCheck(TriggerEvent::BoundEntered, record, player(false));
    require(applyTriggerCheck(TriggerEvent::BoundEntered, record, player(true)), "outside to inside fires");
}

void boundLeftNeedsAPreviousInside() {
    TriggerRecord record;
    require(!applyTriggerCheck(TriggerEvent::BoundLeft, record, player(false)), "skip");
    require(!applyTriggerCheck(TriggerEvent::BoundLeft, record, player(false)), "outside without a stored inside does not fire");
    applyTriggerCheck(TriggerEvent::BoundLeft, record, player(true));
    require(!applyTriggerCheck(TriggerEvent::BoundLeft, record, player(true)), "inside does not fire");
    require(applyTriggerCheck(TriggerEvent::BoundLeft, record, player(false)), "inside to outside fires");
    require(!applyTriggerCheck(TriggerEvent::BoundLeft, record, player(false)), "and only once");
}

void boundLeftFiresWhenTheFirstCheckSawInsideAndTheBoundIsSwitchedOff() {
    TriggerRecord record;
    applyTriggerCheck(TriggerEvent::BoundLeft, record, player(true));
    require(applyTriggerCheck(TriggerEvent::BoundLeft, record, player(false)), "a switched-off bound reads as outside");
}

void boundInsideAndOutsideAreLevels() {
    TriggerRecord inside;
    require(applyTriggerCheck(TriggerEvent::BoundInside, inside, player(true)), "BoundInside fires on the very first check");
    require(applyTriggerCheck(TriggerEvent::BoundInside, inside, player(true)), "and on every following one");
    require(!applyTriggerCheck(TriggerEvent::BoundInside, inside, player(false)), "not outside");
    TriggerRecord outside;
    require(applyTriggerCheck(TriggerEvent::BoundOutside, outside, player(false)), "BoundOutside fires on the first check");
    require(applyTriggerCheck(TriggerEvent::BoundOutside, outside, player(false)), "and again");
    require(!applyTriggerCheck(TriggerEvent::BoundOutside, outside, player(true)), "not inside");
}

void footAndVehicleVariantsSplitOnTheVehicle() {
    TriggerRecord foot;
    require(!applyTriggerCheck(TriggerEvent::PlayerOnFootBoundEntered, foot, player(true, true)) && foot.skip,
            "on foot variant in a vehicle changes nothing");
    applyTriggerCheck(TriggerEvent::PlayerOnFootBoundEntered, foot, player(false));
    require(applyTriggerCheck(TriggerEvent::PlayerOnFootBoundEntered, foot, player(true)), "on foot variant fires on foot");
    TriggerRecord vehicle;
    require(!applyTriggerCheck(TriggerEvent::PlayerVehicleBoundEntered, vehicle, player(true)) && vehicle.skip,
            "vehicle variant on foot changes nothing");
    applyTriggerCheck(TriggerEvent::PlayerVehicleBoundEntered, vehicle, player(false, true));
    require(applyTriggerCheck(TriggerEvent::PlayerVehicleBoundEntered, vehicle, player(true, true)), "vehicle variant fires in a vehicle");
}

void player2NeverFires() {
    TriggerRecord record;
    record.inside = true;
    require(!applyTriggerCheck(TriggerEvent::Player2BoundEntered, record, player(true)) && !record.inside && !record.skip,
            "Player2BoundEntered clears the record");
}

void objectBoundEntered() {
    TriggerRecord record;
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundEntered, record, object(true, false)) && record.skip,
            "an unusable object or bound changes nothing");
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundEntered, record, object(false)), "skip check");
    require(applyTriggerCheck(TriggerEvent::ObjectBoundEntered, record, object(true)), "outside to inside fires");
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundEntered, record, object(true)), "once");
}

void objectBoundLeftStoresOutside() {
    TriggerRecord record;
    applyTriggerCheck(TriggerEvent::ObjectBoundLeft, record, object(true));
    require(!record.inside, "the stored word is the outside flag (0 while inside)");
    require(applyTriggerCheck(TriggerEvent::ObjectBoundLeft, record, object(false)), "inside to outside fires");
    require(record.inside, "outside stores 1");
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundLeft, record, object(false)), "only once");
    TriggerRecord startsOutside;
    applyTriggerCheck(TriggerEvent::ObjectBoundLeft, startsOutside, object(false));
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundLeft, startsOutside, object(false)), "an object that never was inside never leaves");
}

void objectBoundInsideIsALevelAfterSkip() {
    TriggerRecord record;
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundInside, record, object(true)), "skip on the first check");
    require(applyTriggerCheck(TriggerEvent::ObjectBoundInside, record, object(true)), "then every check while inside");
    require(!applyTriggerCheck(TriggerEvent::ObjectBoundInside, record, object(false)), "not outside");
}

}

int main() {
    try {
        classification();
        boundEnteredIsAnEdgeAfterTheFirstCheck();
        boundLeftNeedsAPreviousInside();
        boundLeftFiresWhenTheFirstCheckSawInsideAndTheBoundIsSwitchedOff();
        boundInsideAndOutsideAreLevels();
        footAndVehicleVariantsSplitOnTheVehicle();
        player2NeverFires();
        objectBoundEntered();
        objectBoundLeftStoresOutside();
        objectBoundInsideIsALevelAfterSkip();
    } catch (const std::exception& error) {
        std::cerr << "script-triggers: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-triggers: ok\n";
    return 0;
}

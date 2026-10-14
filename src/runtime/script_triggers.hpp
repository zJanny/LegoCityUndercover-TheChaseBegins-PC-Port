#pragma once

#include <string>

namespace chase::runtime {

// The check function of a trigger type (type+0x28, registered by event name in 0x16babc / 0x15b888); docs/re/script-triggers.md.
enum class TriggerEvent {
    Unmodelled,
    BoundEntered,                 // 0x28aef4; BoundBothEntered 0x2b0cd0 and Player1BoundEntered 0x2c185c give the same result for one player
    BoundLeft,                    // 0x26a20c
    BoundInside,                  // 0x282028
    BoundOutside,                 // 0x28af68
    PlayerOnFootBoundEntered,     // 0x2ca6b8: Player1BoundEntered while 0x24255c(player) is 0
    PlayerVehicleBoundEntered,    // 0x2cb578: Player1BoundEntered while 0x24255c(player) is not 0
    Player2BoundEntered,          // 0x2c194c: clears skip and state, never fires
    ObjectBoundEntered,           // 0x2bc67c
    ObjectBoundLeft,              // 0x2aae30
    ObjectBoundInside,            // 0x2b7ecc
};

// `lowerCaseName` is the event name of the file record in lower case.
TriggerEvent classifyTriggerEvent(const std::string& lowerCaseName);

// Runtime trigger record bytes +1 (skip) and +0x1c (the stored word). `inside` is the last result of the check; ObjectBoundLeft stores the opposite.
struct TriggerRecord {
    bool skip = true;
    bool inside = false;
};

// What one check reads. `playerInside` is 0x201a54: the bound's bytes +0xa and +0xb are set and the player node lies in
// it. `objectUsable` is the head of the object variants: bound byte +0xa set and the object's flags 0x21 clear; without
// it the check changes nothing. `objectInside` is 0x22f650 for the object's node.
struct TriggerSample {
    bool playerInside = false;
    bool playerInVehicle = false;
    bool objectUsable = false;
    bool objectInside = false;
};

// Runs the check of `event` on `record` and returns whether it calls 0x21a560 (which may still refuse: enable byte,
// owner entity, a script that already runs).
bool applyTriggerCheck(TriggerEvent event, TriggerRecord& record, const TriggerSample& sample);

}

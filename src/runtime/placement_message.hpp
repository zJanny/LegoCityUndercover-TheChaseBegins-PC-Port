#pragma once

namespace chase::runtime {

// What PositionPlayer 0x1da0dc and PositionObject 0x1d9e6c leave on the player besides the fall floats
// (docs/re/script-natives-flags.md, entry of 2026-10-19 on message 8 and ctrl+0x315).
//
// settleStepPending: bits 0x14 of ctrl+0x315 are cleared (0x1d9f64 / 0x1da3e0) and the locomotion step 0x20cd30 sets them
// again at its end (0x20d5e0). While they are clear that step takes the 0x20b48c(..., flag 16) branch at bit 4 (0x20d3bc)
// and skips the 0x20cf20 test of bits 2 and 3. STAND-IN SCRIPT-S275: nothing in the port's locomotion reads the flag yet.
//
// Message 8 to the player (entity event function 0x18e4c8, case 0x18ed9c), sent only when the player is not in a vehicle
// (PositionPlayer skips it at 0x1da3fc when 0x24255c found one): 0x2414b4(entity, ctrl+0x50, 1, 0) requests class state 1
// (0x2eafc8 stores the entry at ctrl+0x5c and sets the pending byte ctrl+0x64), 0x23f734(node, 1) clears mask 0x60 of the
// node flags, 0x242cf0 clears the disabled bit of the entity (a return at once for the player: its bit is never set in the port).
// The node show reaches the player's scene instance through takeNodeShow. STAND-IN SCRIPT-S274: the port has no state machine
// entry for the request (SCRIPT-335), so defaultStateRequested is recorded and read by nothing.
struct PlacementEffects {
    bool settleStepPending = false;
    bool defaultStateRequested = false;
    bool nodeShown = true;
    bool entityEnabled = true;
    bool nodeShowPending = false;
};

bool takeNodeShow(PlacementEffects& effects);

void placePlayerEffects(PlacementEffects& effects, bool inVehicle);

// The end of the locomotion step 0x20cd30 (0x20d5e0: ctrl+0x315 |= 0x14).
void locomotionStepEnded(PlacementEffects& effects);

}  // namespace chase::runtime

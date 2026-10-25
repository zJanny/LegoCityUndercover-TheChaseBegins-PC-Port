#pragma once

#include "runtime/placement_message.hpp"
#include "runtime/player_vertical.hpp"
#include <cstdint>
#include <vector>

namespace chase::runtime {

// Entity kind byte (entity+0x12) of the player entity and of the second built-in controller kind of 0x21aae0.
// Oracle: save state outside the police station after mission 1-1, byte at 0x085526f2 (entity 0x085526f0, [0x3772b8]) = 0x1b.
constexpr std::uint8_t kPlayerEntityKind = 27;
constexpr std::uint8_t kSecondControllerEntityKind = 149;
constexpr std::uint16_t kEntityFlagController = 0x4000;

// The kind list 0x236b6c searches (byte count at 0x3743ec, kinds at 0x515a94; count 0 in the file, filled at run time
// by code that was not read). STAND-IN SCRIPT-S97: the one entry measured in the oracle (docs/re/script-natives-flags.md).
inline const std::vector<std::uint8_t>& measuredControllerKindList() {
    static const std::vector<std::uint8_t> list = {159};
    return list;
}

// 0x21aae0: true for kind 27 or 149, a kind in the list of 0x236b6c, or entity flag 0x4000 (entity+0xc).
bool entityHasController(std::uint8_t kind, std::uint32_t flags, const std::vector<std::uint8_t>& kindList);

// The controller reset of PositionObject 0x1d9e6c (0x1d9f68..0x1d9f6c) and PositionPlayer 0x1da0dc (0x1da3e4..0x1da3e8):
// ctrl+0x2b4 (fall distance) and ctrl+0x2b8 (fall speed) become 0.0. The same code clears bits 0x14 of ctrl+0x315 and, when the
// player is not in a vehicle, sends message 8; both are recorded in `state.placement` (placement_message.hpp, SCRIPT-S274,
// SCRIPT-S275).
void resetControllerAfterPlacement(VerticalState& state, bool inVehicle = false);

// PositionPlayer 0x1da0dc ground probe: a gather 0x237f30 around the target (query mask 4, the character kinds 179, 183 and
// 243) must find a character before the probe segment of length 5.0 along the global float3 at 0x56d424 runs.
constexpr float kPlacementProbeLength = 5.0f;
// Oracle: save state outside the police station after mission 1-1, 0x56d424 = (0, 1, 0).
constexpr float kPlacementProbeDirection[3] = {0.0f, 1.0f, 0.0f};
bool isProbeCharacterKind(std::uint8_t kind);

// 0x23b92c: end = start + direction * length.
void placementProbeEnd(const float start[3], float out[3]);

}  // namespace chase::runtime

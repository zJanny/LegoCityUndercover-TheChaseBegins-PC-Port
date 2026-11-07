#pragma once
// The ground ray cast of the traffic probe (0x20ae2c -> 0x22b108 -> 0x266834 -> 0x22f978 -> 0x293188, docs/re/traffic-ground-cast.md):
// a vertical segment against the level's collision with the probe's descriptor (include mask 1, exclude 0, no surface mask, first hit ends).
// Supplies the `cast` of `TrafficGroundWorld` (traffic_leave_snap.hpp). No SDL.
#include "runtime/collision.hpp"
#include "runtime/traffic_leave_snap.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace chase::runtime {

constexpr CollisionLayer kTrafficGroundLayer{1, 0};  // 0x20ae2c stack words +4 / +8 of the descriptor: include 1, exclude 0
constexpr float kTrafficGroundDeterminantMinimum = 1.0e-5f;  // 0x293490 (0x3727c5ac)

// 0x22b108 with the probe's filter on a segment whose x and z agree. A box entry that the segment meets ends the cast without a
// triangle (none). STAND-IN TRAFFIC-S23: the entries are tried in the order of the world, not in the order of the grid cells of 0x237f30,
// and without its cap of 100 entries. Throws for a segment that is not vertical (the original takes 0x265c34 there, not read).
std::optional<TrafficGroundHit> castTrafficGround(const CollisionWorld& world, const TrafficGroundSegment& segment);

// Command 1 of 0x228528 with the arguments {colour, -1} (0x23e110), as the mesh node class handler 0x106980 (case 1, 0x1069e8) takes it for
// one mesh node whose layers have mesh data: bytes +0x118 / +0x119 / +0x11a of the node become the low, middle and high byte of the colour.
// STAND-IN TRAFFIC-S24: the handler also paints the white vertex colours of layers whose word +0x104 / +0x108 is set, and nothing reads the
// three bytes yet (their use in the draw is not read); the port keeps them per node and sends the message to the nodes the caller names.
struct TrafficNodeTint {
    std::array<std::uint8_t, 3> bytes{};
};

void applyTrafficColourMessage(TrafficNodeTint& node, std::uint32_t colour);

// The world for `snapTrafficTarget`: `cast` over `world`, `sendColour` as given (the receiver of command 1 is TRAFFIC-S24).
TrafficGroundWorld trafficGroundWorld(const CollisionWorld& world, std::function<void(std::uint32_t colour)> sendColour);

}

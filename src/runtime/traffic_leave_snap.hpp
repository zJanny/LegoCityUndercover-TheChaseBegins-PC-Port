#pragma once
// Two calls of the route mover (docs/re/traffic-ai-driver.md): the leave call 0x21d6b8 of a car whose route leads into no junction
// (hook `leaveTraffic`) and the ground colour probe 0x20ae2c (hook `snapTarget`), with the triangle weights 0x2b51b8 and the colour of the hit
// 0x221064. Pure steps; the ray cast 0x22b108 and the consumer of the colour (0x23e110 -> 0x228528, command 1) are supplied by the caller
// (TRAFFIC-S16). No SDL.
#include "runtime/ai_driver_input.hpp"
#include "runtime/traffic_manager.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr float kTrafficGroundProbeHalfHeight = 1.5f;  // 0x20af1c (0x3fc00000)
constexpr float kTrafficGroundWeightScale = 0.125f;    // 0x2b52f8 (0x3e000000)
constexpr std::int32_t kTrafficGroundMaxBits = 0x437f0000;  // 0x22134c: 255.0
constexpr std::int32_t kTrafficCommandColour = 1;      // 0x23e110: command 1 of 0x228528 with the arguments {colour, -1}

// 0x21d6b8 once the car is found in the manager's table; false when the table has no such car.
bool leaveTraffic(TrafficManager& manager, std::size_t car, std::vector<TrafficEvent>* events);

struct TrafficGroundTriangle {
    AiVec3 normal{};                          // triangle +0 .. +8
    std::array<AiVec3, 3> corners{};          // the three vectors that triangle +16 .. +24 point to
    std::array<std::uint8_t, 3> colourIndex{};  // triangle +0x1d .. +0x1f
};

struct TrafficGroundMesh {
    bool hasColours = false;                  // mesh +8 is set
    std::vector<std::uint32_t> colours;       // the table mesh +4 points to
};

struct TrafficGroundHit {
    TrafficGroundTriangle triangle;
    TrafficGroundMesh mesh;
    AiVec3 point{};                           // out +12 .. +24
};

struct TrafficGroundSegment {
    AiVec3 from{};
    AiVec3 to{};
};

// 0x20ae2c: from the target raised by 1.5 to the target lowered by 1.5.
TrafficGroundSegment trafficGroundSegment(const AiVec3& target);

// 0x2b51b8: the weights of `point` in the triangle; none when the determinant is zero.
std::optional<std::array<float, 3>> trafficTriangleWeights(const TrafficGroundTriangle& triangle, const AiVec3& point);

// 0x221064: the colour (blue in the low byte, red in bits 16 to 23, alpha 255) of the hit; none when the weights fail.
std::optional<std::uint32_t> trafficGroundColour(const TrafficGroundMesh& mesh, const TrafficGroundTriangle& triangle, const AiVec3& point);

struct TrafficGroundWorld {
    // 0x22b108 with the filter of 0x20ae2c (group word 512, mask 3, flag byte 1): the hit of the ray, none when it hits no triangle.
    std::function<std::optional<TrafficGroundHit>(const TrafficGroundSegment& segment)> cast;
    // 0x23e110(entity +0x3c, colour, -1, 1).
    std::function<void(std::uint32_t colour)> sendColour;
};

// 0x20ae2c(entity) on the driver record: true when a colour was sent.
bool snapTrafficTarget(const AiDriverRecord& driver, const TrafficGroundWorld& world);

}

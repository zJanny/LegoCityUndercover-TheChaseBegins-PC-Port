#pragma once
// Segment queries of the follow camera's reset 0x2313c8: the line test 0x22b108 (0x266834) answers whether anything lies between
// two points, the ray test 0x21ab20 (0x27cab4) returns the nearest hit. By default both use the camera's object descriptor (the layer
// and owner flags of CollisionWorld::Entry), the surface exclusion mask K+0x170 and exclude the followed entity; other callers pass
// their own descriptor's masks. Mesh entries are cut
// triangle by triangle as in 0x17e0f8 (front faces only), box entries by the segment against the box in its own space (0x2040e8).
// No SDL dependency.
#include "runtime/camera_sweep.hpp"
#include "runtime/collision.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace chase::runtime {

class CameraResetProbe {
public:
    CameraResetProbe(const CollisionWorld& world, std::string excludedOwner) : world_(world), excludedOwner_(std::move(excludedOwner)) {}
    // The same queries with another descriptor: its include and exclude masks and the surface exclusion mask at descriptor +0x30.
    CameraResetProbe(const CollisionWorld& world, std::string excludedOwner, CollisionLayer layer, std::uint64_t excludedSurfaces)
        : world_(world), excludedOwner_(std::move(excludedOwner)), layer_(layer), excludedSurfaces_(excludedSurfaces) {}

    // 0x22b108 on a segment from `from` to `to`: true when any accepted entry is hit.
    bool segmentBlocked(const float from[3], const float to[3]) const;
    // 0x21ab20: the hit nearest to `from` along the segment.
    std::optional<std::array<float, 3>> nearestHit(const float from[3], const float to[3]) const;

private:
    const CollisionWorld& world_;
    std::string excludedOwner_;
    CollisionLayer layer_ = kCameraLayer;
    std::uint64_t excludedSurfaces_ = kCameraExcludedSurfaces;
};

// The segment against a box in its own space (0x2040e8): the fraction of the segment at which it enters the box, 0 when the start is inside.
bool segmentMeetsBox(const float from[3], const float to[3], const float centre[3], const float half[3], float& entry);

// 0x17e0f8 without the surface test: the distance along the unit direction at which the ray from `origin` meets the triangle,
// or a negative value. Triangles seen from behind and parallel ones (determinant below 1e-5) are missed; hits beyond `limit` too.
float rayTriangleDistance(const float v0[3], const float v1[3], const float v2[3], const float origin[3], const float direction[3], float limit);

}

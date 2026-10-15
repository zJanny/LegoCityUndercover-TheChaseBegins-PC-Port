#pragma once
// Obstruction test of the follow camera: the sweep object at K+0x180 of the camera system (setup 0x2d5208 / 0x2d4ea8 /
// 0x2eb288, triangle callback 0x2d4fd8, slicing 0x2eb9a0, segment processing 0x2eafdc, re-evaluation 0x2d4e64,
// funnel depth 0x2ebe10). Two thin slabs (a horizontal and a vertical one) are laid along the line from the camera to the
// look point; the static collision triangles cut the slabs into 2D segments, and the segments are projected from
// an apex behind the look point onto the camera end of the slab. The result is the widest free gap in [-1, 1].
// Evidence and recovered constants: docs/re/camera.md. No SDL dependency.
#include "runtime/collision.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

struct CameraSweepGap {
    float low = -1.0f, high = 1.0f, middle = 0.0f;
};

struct CameraSweepSettings {
    float reach = 5.0f;           // in+0xcc: the apex of the slab lies this far behind the look point
    float sideHalfWidth = 2.5f;   // in+0xd0: half width of the horizontal slab at the camera end
    float upHalfWidth = 1.25f;    // in+0xd4: half height of the vertical slab at the camera end
    std::string excludedOwner;    // the entity the camera follows is not an obstacle (descriptor word K+0x18c, 0x196990): lower-case instance name
    float previousNear = -1.0f;   // K+0x1c8 as the previous update left it; the small-object rules read it while the gather runs. Negative: this update's
};

// Surfaces the camera query ignores: low mask word at K+0x170 (copied to K+0x1b0), 1 << surface tested as in 0x1dc528.
constexpr std::uint32_t kCameraExcludedSurfaces = 0x01000010;

class CameraSweep {
public:
    enum Shape { Side = 0, Up = 1 };

    struct Segment {
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    };

    // The slab frame is laid from `camera` to `look` (0x2d4ea8); `fullDistance` is sqrt(D^2 + H^2) of the preset.
    void prepare(const CollisionWorld& world, const float camera[3], const float look[3], float fullDistance,
                 const CameraSweepSettings& settings = {});

    const CameraSweepGap& gap(Shape shape) const { return shapes_[shape].gap; }
    // 0x2d4e64 for a camera moved along the line to `distance` from the look point (horizontal slab only).
    CameraSweepGap sideGapAt(float distance) const;
    // 0x2ebe10: the largest depth of an obstacle inside the funnel of half width scale * slab width.
    float deepestObstacle(Shape shape, float scale) const;

    const float* sideAxis() const { return shapes_[Side].lateral; }
    const float* upAxis() const { return shapes_[Up].lateral; }
    float farDepth() const { return farLimit_; }
    float nearDepth() const { return nearLimit_; }

    const std::vector<Segment>& rawSegments(Shape shape) const { return shapes_[shape].raw; }

private:
    struct SliceShape {
        float lateral[3] = {}, plane[3] = {};
        float size = 0;
        std::vector<Segment> raw, pieces;
        CameraSweepGap gap;
    };

    bool slice(SliceShape& shape, const float triangle[3][3]) const;
    void sliceBox(SliceShape& shape, const float matrix[16], const float centre[3], const float half[3]) const;
    float slabDepth(const float point[3]) const;
    void process(SliceShape& shape) const;

    float origin_[3] = {}, depth_[3] = {};
    float apex_ = 0, nearLimit_ = 0, farLimit_ = 0, fullDistance_ = 0;
    SliceShape shapes_[2];
};

}

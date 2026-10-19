#pragma once
// The rope of a zipline (object type 101, draw 0x1a9dc4 -> 0x297b7c -> strip 0x1f9590): a textured ribbon between the node of an anchor and the node
// of its target, faded out near the camera. Rules, constants and stand-ins: docs/re/player-traversal.md. No SDL or OpenGL dependency.
#include "assets/assets.hpp"
#include "runtime/zipline.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr float kRopeRenderWidthDefault = 0.03f;  // 0x1a999c: literal 0x3cf5c28f when the object has no RenderWidth (+0x158)
constexpr float kRopeFadeStart = 2.0f;            // 0x1a9dc4: the rope is not drawn at or below this camera distance from the line
constexpr float kRopeFadeEnd = 5.0f;              // at or beyond it the rope is opaque (alpha 255)
constexpr float kRopeFadeScale = 0.33333334f;     // literal 0x3eaaaaab at 0x1a9f0c
constexpr float kRopeTextureRepeatY = 2.0f;       // 0x1f9590: the height difference counts twice in the texture length

struct ZiplineRope {
    std::string texture;           // RopeTexture (+0x15c); empty: the anchor draws nothing
    float width = kRopeRenderWidthDefault;  // RenderWidth (+0x158), half the width of the ribbon
    bool fadeNearCamera = false;   // FadeNearCamera (+0x163 bit 1)
};

struct RopeVertex {
    float position[3];
    float uv[2];
    std::uint8_t rgba[4];
};

struct RopeDraw {
    std::string texture;
    std::uint8_t alpha = 255;
    std::array<RopeVertex, 4> strip;  // triangle strip: (start, +side), (start, -side), (end, +side), (end, -side)
};

// One entry per anchor of `anchors` (same order).
std::vector<ZiplineRope> loadZiplineRopes(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<ZiplineAnchor>& anchors);

// 0x1a9dc4 for fadeNearCamera: the alpha byte for a camera at `camera`, or -1 when the rope is not drawn (distance of the camera from the
// infinite line through the anchor and its target, in the frame built by 0x19b074 / 0x22859c).
int ziplineRopeAlpha(const ZiplineAnchor& anchor, const ZiplineAnchor& target, const float camera[3]);

// The four vertices of 0x1f9590 for the rope from `start` to `end`. `viewDirection` stands in for row 2 of the matrix cache at 0x529fec
// (TRAV-S29).
std::array<RopeVertex, 4> ziplineRopeStrip(const float start[3], const float end[3], float width, const float viewDirection[3], std::uint8_t alpha);

// The draws of one update: every anchor with a rope texture and an enabled object. Anchors have no update handler, so the gate 0x226644 calls the
// target live only when it has no rope texture (byte +0x13 = 2); then the lower id anchor leaves the line to the target (0x1a9dc4). Otherwise both
// anchors draw the line, as the original does (TRAV-S32: flag bits 2 and 5 of the target are taken as clear).
std::vector<RopeDraw> ziplineRopeDraws(const std::vector<ZiplineAnchor>& anchors, const std::vector<ZiplineRope>& ropes, const float camera[3],
                                       const float viewDirection[3], const std::function<bool(const ZiplineAnchor&)>& enabled);

}

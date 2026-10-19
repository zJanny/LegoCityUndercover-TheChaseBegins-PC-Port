#pragma once
// Root motion of placed props (docs/re/objects.md): the prop update 0x15aa70 -> 0x16b06c asks 0x2327b4 for the root delta of the
// playing slot and moves the node by it. 0x270778 samples the clip's extra joint curve (joint table bit 28) as a translation and a yaw
// (0x2473ec on the third matrix row); 0x2327b4 turns the previous sample A and the current one B into B * inverse(A) (the translation
// in the frame of A and the yaw difference) and stores B; the first update of a slot only stores. 0x16b06c turns the node by the yaw
// and 0x23d6c4 moves it by the translation taken through the node's rotation before the turn. No SDL dependency.
#include "assets/assets.hpp"

#include <optional>

namespace chase::runtime {

struct RootSample {
    float translation[3] = {0, 0, 0};
    float yaw = 0;  // radians about y, the convention of assets::placementMatrix
};

// The extra curve at a frame, clamped to the stored samples. STAND-IN LEVOBJ-S83: a fractional frame interpolates the two
// neighbouring samples linearly (the decoder 0x287124 was not read for the fraction). Empty when the clip has no extra curve.
std::optional<RootSample> rootSample(const assets::Animation& animation, double frame);

// B * inverse(A) of 0x2327b4 for one slot at weight 1.
RootSample rootDelta(const RootSample& previous, const RootSample& current);

// The node after one delta (row vectors, translation last): rotation Ry(yaw) * old rotation, position + translation * old rotation.
// STAND-IN LEVOBJ-S82: all four axes (x, y, z, yaw) are applied to every clip; the axis bits of the animation record (+8, tested at
// 0x16b2d4) were measured (0xf) only for dog_cage_open, and the test 0x245f84() != 6 of 0x16b06c is not applied.
assets::Matrix applyRootDelta(const assets::Matrix& node, const RootSample& delta);

}

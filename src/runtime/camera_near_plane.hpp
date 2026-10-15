#pragma once
// Near plane of the follow camera and the tracker snap that uses it (0x2e0e28 and the tail of 0x2e0bf0; docs/re/camera-vehicle.md).
// The raw segments of both sweep slabs are seen from a camera position looking at the tracked look point, clipped to the view frustum
// (z > 0 and the four side planes of the projection scales), and the nearest depth gives the near plane. No SDL dependency.
#include "runtime/camera_sweep.hpp"

namespace chase::runtime {

// K+0x148: the near plane is at most this far.
constexpr float kCameraNearLimit = 1.0f;

// 0x2e0e28: min(limit + 0.1, nearest depth of a segment inside the frustum) - 0.1, at least 0.2. `xScale` and `yScale` are the projection
// scales of the render camera (camera +0xbc and +0xc0).
float cameraNearDepth(const CameraSweep& sweep, const float eye[3], const float look[3], float xScale, float yScale,
                      float limit = kCameraNearLimit);

}

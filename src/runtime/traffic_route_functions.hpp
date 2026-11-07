#pragma once
// Slots +8 (`ahead`) and +0xc (`nearest`) of the route type table 0x3581d4 (five words per path type byte, path record +8; the same words
// as 0x357f34 and 0x358ed8) for the traffic routes and the junction lanes (docs/re/traffic-route-functions.md). Type 0 (Catmull-Rom, every
// shipped traffic route and every lane) has 0x29fb6c and 0x21d5b0, type 1 (linear) the stubs 0x29fa58 and 0x2878f0, which return 0. The
// inner samples are slot 0 of the type (table 0x358ed8) without tangent and with argument 1; its node transform (path +0x18) is not
// applied, as in sampleTrafficRoute. Any other type throws std::invalid_argument, as does a type 0 path without
// points. No SDL dependency.
#include "assets/assets.hpp"

#include <array>

namespace chase::runtime {

constexpr float kTrafficRouteAheadSquaredBelow = 100.0f;  // 0x29fcc8

// 0x29fb6c(path, s0 ahead, s1 step, s2 from): from `from`, the parameter moves on by `step` while the walked length is below `ahead` and
// the parameter below the point count (count - 1 for an open path, u32 to float). Each step adds the distance (0x2507dc) between the
// samples before and after it; when `ahead` is below 100 the squared distances (0x24ef70) are summed and compared with ahead squared.
// Returns the parameter.
float trafficRouteAhead(const assets::LevelPath& path, float ahead, float step, float from);

// 0x21d5b0(path, r1 position, r2 samples, r3, s0 scale): the parameter k / samples (k = 1 .. (count - 1 for an open path, else count) x
// samples) whose sample has the smallest length of (sample - position) x (1 / scale); the first of equal ones, and 0 when no sample is
// below 3.4028235e38 or there is none. Parameter 0 itself is never sampled.
float trafficRouteNearest(const assets::LevelPath& path, const std::array<float, 3>& position, unsigned samples, float scale);

}

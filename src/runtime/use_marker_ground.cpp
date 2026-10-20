#include "runtime/use_marker_ground.hpp"

namespace chase::runtime {

std::array<float, 3> useMarkerGround(const CollisionWorld& world, const std::array<float, 16>& matrix, const float zoneCentre[3]) {
    float from[3], to[3];
    for (int a = 0; a < 3; ++a) {
        const std::size_t i = std::size_t(a);
        from[a] = matrix[12 + i] + zoneCentre[0] * matrix[i] + zoneCentre[1] * matrix[4 + i] + zoneCentre[2] * matrix[8 + i];
        to[a] = from[a] - kUseMarkerGroundSearch * matrix[4 + i];
    }
    if (const auto hit = CameraResetProbe(world, "").nearestHit(from, to)) return {(*hit)[0], (*hit)[1], (*hit)[2]};
    return {from[0], from[1], from[2]};
}

}

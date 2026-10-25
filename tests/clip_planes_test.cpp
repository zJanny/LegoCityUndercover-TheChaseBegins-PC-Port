// Near and far plane handed to the projection and the fog table (src/runtime/clip_planes.hpp, docs/re/camera-vehicle.md).
#include "runtime/clip_planes.hpp"
#include "runtime/fog.hpp"

#include <cmath>
#include <stdexcept>

using namespace chase::runtime;

namespace {
void require(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
}
}

int main() {
    // Oracle the save state in front of the police car of mission 1-1 hand-back, memtrace of the render camera +0xc4 / +0xc8 (docs/re/camera-vehicle.md §2026-10-15T16:45).
    const float recordedNear[] = {1.0f, 0.722414f, 0.563713f, 0.419943f, 0.303219f, 0.213484f, 0.2f};
    for (float near : recordedNear) {
        const ClipPlanes planes = followClipPlanes(near);
        require(planes.nearZ == near, "near plane is the near plane test result");
        require(planes.farZ == 260.0f, "far plane is K+0x14c = 260");
    }

    // Oracle matrix of the save state outside the police station after mission 1-1 (docs/re/rendering-fog.md §2026-10-14T18:59): A = far / (far - near) = 1.0038609 for (1, 260).
    const ClipPlanes planes = followClipPlanes(1.0f);
    require(std::fabs(planes.farZ / (planes.farZ - planes.nearZ) - 1.0038609f) < 1e-6f, "depth scale of the recorded matrix");

    const FogLut atOne = buildFogLut(20.0f, 120.0f, followClipPlanes(1.0f));
    const FogLut atTwoTenths = buildFogLut(20.0f, 120.0f, followClipPlanes(0.2f));
    require(atOne != atTwoTenths, "the fog table follows the near plane");
}

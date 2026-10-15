#include "runtime/camera_near_plane.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace chase;
using namespace chase::runtime;

namespace {
void expect(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
}

assets::CollisionItem wall(float z, float halfWidth) {
    assets::CollisionItem item;
    item.name = "wall";
    const float quad[4][3] = {{-halfWidth, -halfWidth, z}, {halfWidth, -halfWidth, z}, {halfWidth, halfWidth, z}, {-halfWidth, halfWidth, z}};
    for (const auto& v : quad) item.vertices.insert(item.vertices.end(), {v[0], v[1], v[2]});
    assets::CollisionTriangle a, b;
    a.vertex[0] = 0, a.vertex[1] = 2, a.vertex[2] = 1;
    b.vertex[0] = 0, b.vertex[1] = 3, b.vertex[2] = 2;
    item.triangles = {a, b};
    assets::CollisionNode root;
    root.centre[2] = z;
    root.half[0] = root.half[1] = halfWidth;
    root.half[2] = 0.1f;
    root.info = (2u << 1) | 1u;
    item.nodes = {root};
    return item;
}

float nearDepth(float wallZ, float halfWidth, float eyeZ, float xScale = 2.2f, float yScale = 3.66f) {
    CollisionWorld world;
    world.add(wall(wallZ, halfWidth));
    const float camera[3] = {0, 0, 0}, look[3] = {0, 0, 10};
    CameraSweep sweep;
    sweep.prepare(world, camera, look, 10.0f);
    const float eye[3] = {0, 0, eyeZ};
    return cameraNearDepth(sweep, eye, look, xScale, yScale);
}
}

int main() {
    try {
        // Provenance: hand calculation with the rules of 0x2e0e28 (start at K+0x148 + 0.1 = 1.1, nearest view depth of a raw sweep segment
        // inside the frustum, minus 0.1, at least 0.2); the geometry is this test's.
        expect(std::fabs(nearDepth(0.6f, 50.0f, 0.0f) - 0.5f) < 1e-5f, "a wall 0.6 in front gives 0.5");
        expect(std::fabs(nearDepth(0.6f, 50.0f, 0.45f) - 0.2f) < 1e-6f, "a wall 0.15 in front is clamped to 0.2");
        expect(nearDepth(3.0f, 50.0f, 0.0f) == 1.0f, "a wall beyond 1.1 leaves the limit");
        expect(nearDepth(0.6f, 50.0f, 1.0f) == 1.0f, "a wall behind the eye is outside the frustum");
        // A wall piece 0.4 wide at depth 0.6, shifted aside: the side planes x = z / xScale cut it off (0.6 / 2.2 = 0.27 < 0.3).
        CollisionWorld world;
        assets::CollisionItem piece = wall(0.6f, 0.2f);
        for (std::size_t i = 0; i < piece.vertices.size(); i += 3) piece.vertices[i] += 0.5f;
        piece.nodes[0].centre[0] = 0.5f;
        world.add(piece);
        const float camera[3] = {0, 0, 0}, look[3] = {0, 0, 10};
        CameraSweep sweep;
        sweep.prepare(world, camera, look, 10.0f);
        expect(!sweep.rawSegments(CameraSweep::Side).empty(), "the side slab cuts the shifted piece");
        expect(cameraNearDepth(sweep, camera, look, 2.2f, 3.66f) == 1.0f, "a piece outside the side planes does not count");
        expect(std::fabs(cameraNearDepth(sweep, camera, look, 1.0f, 3.66f) - 0.5f) < 1e-5f, "a wider frustum sees it");
    } catch (const std::exception& error) {
        std::cerr << "camera-near-plane: " << error.what() << '\n';
        return 1;
    }
    std::cout << "camera-near-plane: ok\n";
    return 0;
}

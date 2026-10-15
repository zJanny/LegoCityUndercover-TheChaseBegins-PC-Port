#include "runtime/camera_reset_probe.hpp"
#include "runtime/follow_camera.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace chase;
using namespace chase::runtime;

namespace {
void expect(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
}

assets::CollisionItem wall(float z, bool facesPositive = false) {
    assets::CollisionItem item;
    item.name = "wall";
    const float quad[4][3] = {{-50, -50, z}, {50, -50, z}, {50, 50, z}, {-50, 50, z}};
    for (const auto& v : quad) item.vertices.insert(item.vertices.end(), {v[0], v[1], v[2]});
    assets::CollisionTriangle a, b;
    a.vertex[0] = 0, a.vertex[1] = facesPositive ? 1 : 2, a.vertex[2] = facesPositive ? 2 : 1;
    b.vertex[0] = 0, b.vertex[1] = facesPositive ? 2 : 3, b.vertex[2] = facesPositive ? 3 : 2;
    item.triangles = {a, b};
    assets::CollisionNode root;
    root.centre[2] = z;
    root.half[0] = root.half[1] = 50.0f;
    root.half[2] = 0.1f;
    root.info = (2u << 1) | 1u;
    item.nodes = {root};
    return item;
}
}

int main() {
    try {
        // Provenance: hand calculation. The triangle (0,0,0), (1,0,0), (0,1,0) has the normal e1 x e2 = +z; a ray along -z from z = 5
        // meets its front face. 0x17e0f8 misses back faces and hits beyond the limit.
        const float v0[3] = {0, 0, 0}, v1[3] = {1, 0, 0}, v2[3] = {0, 1, 0};
        const float origin[3] = {0.25f, 0.25f, 5.0f}, down[3] = {0, 0, -1}, up[3] = {0, 0, 1};
        expect(std::fabs(rayTriangleDistance(v0, v1, v2, origin, down, 10.0f) - 5.0f) < 1e-6f, "front face hit at 5");
        expect(rayTriangleDistance(v0, v1, v2, origin, down, 4.0f) < 0.0f, "limit shortens the ray");
        const float behind[3] = {0.25f, 0.25f, -5.0f};
        expect(rayTriangleDistance(v0, v1, v2, behind, up, 10.0f) < 0.0f, "back face is culled");

        // The wall quad at z = 10 is wound so that e1 x e2 points to -z: a front face for rays that travel towards +z.
        CollisionWorld world;
        world.add(wall(10.0f));
        const CameraResetProbe probe(world, "");
        const float from[3] = {0, 0, 0}, near[3] = {0, 0, 5}, far[3] = {0, 0, 15}, sideways[3] = {200, 0, 5}, fromBehind[3] = {0, 0, 20};
        expect(!probe.segmentBlocked(from, near), "segment ends before the wall");
        expect(probe.segmentBlocked(from, far), "segment crosses the wall");
        expect(!probe.segmentBlocked(from, sideways), "segment passes beside the wall");
        const auto hit = probe.nearestHit(from, far);
        expect(hit && std::fabs((*hit)[2] - 10.0f) < 1e-4f, "nearest hit on the wall plane");
        expect(!probe.segmentBlocked(fromBehind, from), "the wall is one-sided");

        // Provenance: candidate order of 0x2313c8 (table at 0x359780: 180, 0, 90, ...), the pull-in of 0x23b92c with the margin in+0x8c
        // = 0.25 (ground offset of the preset) and the branches by the stored heading K+0x178; the geometry is this test's.
        const float player[3] = {0, 0, 0};
        CollisionWorld open;
        open.add(wall(1000.0f));
        FollowCamera free;
        free.setCollision(&open);
        free.snap(player, 0.0f);
        expect(std::fabs(free.eye()[2] + 10.0f) < 1e-3f, "unobstructed: the first candidate is 10 m behind the player");

        CollisionWorld behindWall;
        behindWall.add(wall(-4.0f, true));
        FollowCamera pulled;
        pulled.setCollision(&behindWall);
        pulled.snap(player, 0.0f);
        const float horizontal = -pulled.eye()[2];
        expect(horizontal < 4.0f && horizontal > 3.5f, "ray branch pulls the camera in before the wall");

        FollowCamera lineBranch;
        lineBranch.setCollision(&behindWall);
        lineBranch.reset(player, 0.0f, 0.0f);
        lineBranch.takeOver(player, 0.0f, player, false);
        expect(lineBranch.eye()[2] > 5.0f, "line branch rejects the blocked candidate and takes 0 degrees (in front)");

        FollowCamera cleared;
        cleared.setCollision(&behindWall);
        cleared.reset(player, 0.0f, 0.0f);
        cleared.clearStoredHeading();
        cleared.takeOver(player, 0.0f, player, false);
        expect(cleared.eye()[2] < 0.0f && cleared.eye()[2] > -4.0f, "stored heading 0 takes the ray branch");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

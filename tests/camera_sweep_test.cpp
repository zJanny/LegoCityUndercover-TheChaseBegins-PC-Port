// Synthetic checks of the camera obstruction test (CameraSweep, docs/re/camera.md). The expected gaps are derived by hand
// from the recovered projection t = x * R / ((R - y) * size): a wall across the view line at depth 5 of a slab with
// camera distance 10, apex reach 5 (R = 15) and half width 2.5 shows at half width 2.5 * 10 / 15 = 1.667, so t = 0.6 x.
// The recorded behaviour on level data is checked by camera-real-data.
#include "runtime/camera_sweep.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

chase::assets::CollisionItem quadItem(const std::array<float, 12>& quad, std::uint8_t surface) {
    chase::assets::CollisionItem item;
    item.name = "Collision_Test";
    item.vertices.assign(quad.begin(), quad.end());
    for (const std::array<std::uint32_t, 3>& corner : {std::array<std::uint32_t, 3>{0, 1, 2}, {0, 2, 3}}) {
        chase::assets::CollisionTriangle triangle;
        triangle.surface = surface;
        for (int a = 0; a < 3; ++a) triangle.vertex[a] = corner[std::size_t(a)];
        item.triangles.push_back(triangle);
    }
    chase::assets::CollisionNode node;
    for (int a = 0; a < 3; ++a) node.half[a] = 100;
    node.info = 1 | (std::uint32_t(item.triangles.size()) << 1);
    item.nodes.push_back(node);
    return item;
}

// A wall in the plane z = -5 over x in [x0, x1] and y in [-5, 10].
CollisionWorld wall(float x0, float x1, std::uint8_t surface = 0) {
    CollisionWorld world;
    world.add(quadItem({x0, -5, -5, x1, -5, -5, x1, 10, -5, x0, 10, -5}, surface));
    return world;
}

const float kCamera[3] = {0, 0, -10}, kLook[3] = {0, 0, 0};
}

int main() {
    try {
        CameraSweep sweep;

        sweep.prepare(CollisionWorld(), kCamera, kLook, 10.0f);
        require(near(sweep.gap(CameraSweep::Side).low, -1.0f) && near(sweep.gap(CameraSweep::Side).high, 1.0f) &&
                    near(sweep.gap(CameraSweep::Side).middle, 0.0f),
                "free view has the gap [-1, 1]");
        require(near(sweep.farDepth(), 9.75f) && near(sweep.nearDepth(), 0.1f), "slab limits: 0.1 behind the camera, 0.25 short of the look point");

        sweep.prepare(wall(-20, 20), kCamera, kLook, 10.0f);
        require(sweep.rawSegments(CameraSweep::Side).size() == 2, "a wall quad gives one segment per triangle");
        require(near(sweep.gap(CameraSweep::Side).low, 0.0f) && near(sweep.gap(CameraSweep::Side).high, 0.0f), "full wall leaves no gap");
        require(near(sweep.gap(CameraSweep::Up).low, 0.0f) && near(sweep.gap(CameraSweep::Up).high, 0.0f), "full wall leaves no vertical gap");
        require(near(sweep.deepestObstacle(CameraSweep::Side, 1.0f), 5.0f), "the wall is 5 deep");

        // The side axis is dir x up = (-1, 0, 0): the wall over x in [0, 20] covers t in [-1, 0], so the gap is [0.001, 1] and the
        // camera is pushed by lo + 1 = 1.001 along the axis.
        sweep.prepare(wall(0, 20), kCamera, kLook, 10.0f);
        require(near(sweep.sideAxis()[0], -1.0f) && near(sweep.sideAxis()[1], 0.0f) && near(sweep.sideAxis()[2], 0.0f), "side axis");
        require(near(sweep.gap(CameraSweep::Side).low, 0.001f) && near(sweep.gap(CameraSweep::Side).high, 1.0f) &&
                    near(sweep.gap(CameraSweep::Side).middle, 1.001f),
                "half wall gap");

        // The same wall on the other side gives the mirrored gap [-1, -0.001] and a push of hi - 1.
        sweep.prepare(wall(-20, 0), kCamera, kLook, 10.0f);
        require(near(sweep.gap(CameraSweep::Side).low, -1.0f) && near(sweep.gap(CameraSweep::Side).high, -0.001f) &&
                    near(sweep.gap(CameraSweep::Side).middle, -1.001f),
                "mirrored half wall gap");

        // A camera moved in to 4 units has its near plane behind the wall (depth 10 - (4 - 0.1) = 6.1): no obstruction.
        sweep.prepare(wall(-20, 20), kCamera, kLook, 10.0f);
        require(near(sweep.sideGapAt(4.0f).low, -1.0f) && near(sweep.sideGapAt(4.0f).high, 1.0f), "camera in front of the wall");
        require(near(sweep.sideGapAt(6.0f).low, 0.0f) && near(sweep.sideGapAt(6.0f).high, 0.0f), "camera behind the wall");

        // Surfaces 4 and 24 are not part of the camera query (mask 0x01000010 read in the oracle).
        sweep.prepare(wall(-20, 20, 4), kCamera, kLook, 10.0f);
        require(sweep.rawSegments(CameraSweep::Side).empty(), "surface 4 is ignored");
        sweep.prepare(wall(-20, 20, 24), kCamera, kLook, 10.0f);
        require(sweep.rawSegments(CameraSweep::Side).empty(), "surface 24 is ignored");
        sweep.prepare(wall(-20, 20, 5), kCamera, kLook, 10.0f);
        require(sweep.rawSegments(CameraSweep::Side).size() == 2, "surface 5 counts");
    } catch (const std::exception& error) {
        std::cerr << "camera-sweep-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "camera-sweep-test: ok\n";
    return 0;
}

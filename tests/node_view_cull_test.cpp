// Tests of src/runtime/node_view_cull.*: the view volume planes 0x224194, the sphere test 0x24f184 and the visibility function 0x2072ec of
// the mesh node type. Every expected value follows from the disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of
// artifacts/game/game.arm.elf, read by 20261111-chase-G on 2026-11-11, docs/re/traffic-car-culling.md): the plane equations of 0x224194, the
// test of 0x24f184 (a x + b y + c z + d above the radius is outside) and the camera constructor 0x258744 (+0xbc = 0x3f99999a = 1.2,
// +0xc0 = 1.2 x 400 / 240 = 2.0, shifts 0), evaluated by hand in single precision. No emulator was used.
#include "runtime/node_view_cull.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

ViewRows atPosition(float x, float y, float z) { return {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {x, y, z}}}; }

void testPlanesOfTheDefaultCamera() {
    const ViewPlanes planes = viewPlanes(ViewCamera{});
    const float k = 1.0f / std::sqrt(1.0f + 1.2f * 1.2f);
    require(near(planes[0].a, -1.2f * k) && planes[0].b == 0.0f && near(planes[0].c, -1.0f * k) && planes[0].d == 0.0f, "left plane (-a k, 0, -k, 0)");
    require(near(planes[1].a, 1.2f * k) && planes[1].b == 0.0f && near(planes[1].c, -1.0f * k) && planes[1].d == 0.0f, "right plane (a k, 0, -k, 0)");
    const float ky = 1.0f / std::sqrt(1.0f + 2.0f * 2.0f);
    require(planes[2].a == 0.0f && near(planes[2].b, -2.0f * ky) && near(planes[2].c, -1.0f * ky), "bottom plane (0, -b k, -k, 0)");
    require(planes[3].a == 0.0f && near(planes[3].b, 2.0f * ky) && near(planes[3].c, -1.0f * ky), "top plane (0, b k, -k, 0)");
    require(planes[4].a == 0.0f && planes[4].b == 0.0f && planes[4].c == -1.0f && planes[4].d == -1.0f, "near plane (0, 0, -1, -near)");
    require(planes[5].a == 0.0f && planes[5].b == 0.0f && planes[5].c == 1.0f && planes[5].d == -500.0f, "far plane (0, 0, 1, -far)");
}

void testShiftMovesTheSidePlanes() {
    ViewCamera camera;
    camera.shiftX = 0.5f;
    camera.shiftY = -0.25f;
    const ViewPlanes planes = viewPlanes(camera);
    const float k = 1.0f / std::sqrt(1.5f * 1.5f + 1.2f * 1.2f);
    require(near(planes[0].c, (-1.0f - 0.5f) * k), "left plane z coefficient (-1 - shift) k");
    const float ky = 1.0f / std::sqrt(1.25f * 1.25f + 2.0f * 2.0f);
    require(near(planes[3].c, -(1.0f + 0.25f) * ky), "top plane z coefficient -(1 - shift) k");
}

void testScalesFromTheProjection() {
    float scaleX = 0.0f, scaleY = 0.0f;
    viewScales(2.0f * std::atan(0.5f), 400.0f / 240.0f, scaleX, scaleY);
    require(near(scaleY, 2.0f, 1e-4f) && near(scaleX, 1.2f, 1e-4f), "a vertical field of view with tan(fov / 2) = 0.5 at 400 x 240 gives the scales 1.2 and 2.0 of 0x258744");
}

void testSphereInsideAndOutside() {
    const ViewCamera camera;
    const ViewPlanes planes = viewPlanes(camera);
    const ViewVec3 origin{0, 0, 0};
    require(viewSphereVisible(camera, planes, atPosition(0, 0, 10), origin, 1.0f), "straight ahead");
    require(!viewSphereVisible(camera, planes, atPosition(0, 0, 600), origin, 1.0f), "beyond the far plane");
    require(viewSphereVisible(camera, planes, atPosition(0, 0, 499.5f), origin, 1.0f), "the radius reaches into the far plane");
    require(!viewSphereVisible(camera, planes, atPosition(-30, 0, 10), origin, 1.0f), "far left of the view");
    require(viewSphereVisible(camera, planes, atPosition(-30, 0, 10), origin, 40.0f), "a large sphere reaches in from the left");
    require(!viewSphereVisible(camera, planes, atPosition(30, 0, 10), origin, 1.0f), "far right of the view");
    require(!viewSphereVisible(camera, planes, atPosition(0, 40, 10), origin, 1.0f), "far above");
    require(!viewSphereVisible(camera, planes, atPosition(0, -40, 10), origin, 1.0f), "far below");
    require(viewSphereVisible(camera, planes, atPosition(0, 0, -0.05f), origin, 0.1f), "0.05 behind the camera with radius 0.1: the side planes' value 0.032 is not above it");
    require(!viewSphereVisible(camera, planes, atPosition(0, 0, -2.0f), origin, 0.1f), "2 behind the camera is outside the side planes");
}

void testSphereUsesTheCameraFrameAndTheLocalCentre() {
    ViewCamera camera;
    camera.frame = {{{0, 0, -1}, {0, 1, 0}, {1, 0, 0}, {100, 0, 0}}};
    const ViewPlanes planes = viewPlanes(camera);
    require(viewSphereVisible(camera, planes, atPosition(110, 0, 0), {0, 0, 0}, 1.0f), "a camera turned to +x sees a node 10 ahead along +x");
    require(!viewSphereVisible(camera, planes, atPosition(90, 0, 0), {0, 0, 0}, 1.0f), "and not one 10 behind");
    require(viewSphereVisible(camera, planes, atPosition(90, 0, 0), {20, 0, 0}, 1.0f), "a local centre 20 ahead of the node brings it into view");
    ViewRows turned = {{{0, 0, 1}, {0, 1, 0}, {-1, 0, 0}, {0, 0, 0}}};
    require(viewSphereVisible(camera, planes, turned, {110, 0, 0}, 1.0f) == false, "the centre goes through the node's rows (a 90 degree turn puts (110, 0, 0) at (0, 0, 110), behind the turned camera)");
}

void testNodeRules() {
    const ViewCamera camera;
    const ViewPlanes planes = viewPlanes(camera);
    ViewNode node;
    node.matrix = atPosition(0, 0, 10);
    node.radius = 1.0f;
    require(!viewNodeCulled(camera, planes, node), "a node in view is not culled");
    node.fade = 0;
    require(viewNodeCulled(camera, planes, node), "fade byte +0xdd 0 culls");
    node.fade = 255;
    node.alpha = 0;
    require(viewNodeCulled(camera, planes, node), "node alpha +0x11b 0 culls");
    node.alpha = 255;
    node.matrix = atPosition(0, 0, 600);
    require(viewNodeCulled(camera, planes, node), "a node beyond the far plane is culled");
    node.sphereTest = false;
    require(!viewNodeCulled(camera, planes, node), "without the sphere flag the bounds are not tested");
}

}

int main() {
    try {
        testPlanesOfTheDefaultCamera();
        testShiftMovesTheSidePlanes();
        testScalesFromTheProjection();
        testSphereInsideAndOutside();
        testSphereUsesTheCameraFrameAndTheLocalCentre();
        testNodeRules();
    } catch (const std::exception& error) {
        std::cerr << "node-view-cull-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "node-view-cull-test: ok\n";
    return 0;
}

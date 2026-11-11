#pragma once
// The culling bit 18 of a mesh node (docs/re/traffic-car-culling.md): the scene walk 0x20eb58 of the render pass clears the bit and calls
// the node type's visibility function (the mesh node type 2 registers 0x2072ec); the bit is set when it returns 2. For a mesh node that is
// the case when the fade byte +0xdd or the node alpha +0x11b is 0 or when the node's bounding sphere (0x10e628 -> 0x24f184) lies outside
// the view volume of the camera: six planes of the camera record (0x224194) in the camera's frame. No SDL, no GL.
#include <array>
#include <cstdint>

namespace chase::runtime {

using ViewVec3 = std::array<float, 3>;
// Matrix rows as the game keeps them (row vectors, translation in row 3).
using ViewRows = std::array<ViewVec3, 4>;

// The fields of the camera record 0x529e30 the view volume reads: the frame (rows 0..2 the screen right, up and view direction, row 3 the
// position), the projection scales +0xbc / +0xc0, the near and far planes +0xc4 / +0xc8 and the shifts +0xcc / +0xd0.
// The defaults are those of the camera constructor 0x258744 for the 3DS top screen (400 x 240): 1.2, 1.2 x 400 / 240, shifts 0.
struct ViewCamera {
    ViewRows frame{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}};
    float scaleX = 1.2f;
    float scaleY = 2.0f;
    float nearZ = 1.0f;
    float farZ = 500.0f;
    float shiftX = 0.0f;
    float shiftY = 0.0f;
};

// The projection scales of a perspective camera of the vertical field of view `fovY` (radians) and `aspect` = width / height. STAND-IN
// TRAFFIC-S35: the mapping of +0xbc / +0xc0 to the projection (x scale, y scale) follows from the plane equations of 0x224194 and the
// defaults of 0x258744; the camera manager's writers of the two fields (0x25bf18, 0x25bf44) were not read.
void viewScales(float fovY, float aspect, float& scaleX, float& scaleY);

// A plane (a, b, c, d) of the view volume: a point (x, y, z) in the camera's frame is outside when a x + b y + c z + d exceeds the radius.
struct ViewPlane {
    float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
};
constexpr std::size_t kViewPlanes = 6;
using ViewPlanes = std::array<ViewPlane, kViewPlanes>;

// 0x224194 for a perspective camera (byte +0xb8 not 1): left, right, bottom, top, near, far.
ViewPlanes viewPlanes(const ViewCamera& camera);

// 0x24f184 for the list 0 volume (kind mask 15): the sphere of local centre `centre` and `radius` of a node of world matrix `node` is
// inside when no plane has a x + b y + c z + d above the radius.
bool viewSphereVisible(const ViewCamera& camera, const ViewPlanes& planes, const ViewRows& node, const ViewVec3& centre, float radius);

// A mesh node as the visibility function 0x2072ec sees it.
struct ViewNode {
    ViewRows matrix{};
    ViewVec3 centre{};     // +0xa0: the node's bounds centre, local
    float radius = 0.0f;   // +0x9c
    std::uint8_t fade = 255;   // +0xdd
    std::uint8_t alpha = 255;  // +0x11b
    bool sphereTest = true;    // node word bit 14: the sphere test 0x10e628 runs (flag 0x200)
};

// The bit 18 of the node after 0x20eb58: whether 0x2072ec returned 2. STAND-IN TRAFFIC-S35: the per-item tests of the mesh's parts
// (0x10e028, the end of 0x2072ec) are not ported, so a node whose sphere is inside is not culled.
bool viewNodeCulled(const ViewCamera& camera, const ViewPlanes& planes, const ViewNode& node);

}

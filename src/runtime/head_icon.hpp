#pragma once
// The head icon: the X button prompt over the player next to an enterable vehicle (docs/re/vehicles-boarding.md). 0x1835d0 asks for icon 9
// (`0x22d7c0(9, 0)`, row 9 of TablesCharacters sheet 5: sprites/buttons/dsbuttonx) while the vehicle search 0x225d94 finds a vehicle, the
// object "HeadIcons" (0x517ddc) fades it with 0.15 s and holds it 0.1 s after the last request, and the draw callback 0x1a0954 puts a
// camera-facing 1 x 1 quad at the player's node, 1 + 2 * (entity +0x70) above it, faded by the distance to the camera. No SDL dependency.
#include "runtime/camera_lock_icon.hpp"

#include <array>

namespace chase::runtime {

constexpr const char* kHeadIconButtonX = "sprites/buttons/dsbuttonx.btga";
constexpr float kHeadIconFadeSeconds = 0.15f;  // 0x2e71d8, the fade of the overlay registered by the HeadIcons loader 0x2e7104
constexpr float kHeadIconHoldSeconds = 0.1f;   // 0x22d804, the hold of 0x23cc00 in 0x22d7c0
constexpr float kHeadIconNearDistance = 6.0f;  // 0x374704: invisible below, fading in up to the far distance
constexpr float kHeadIconFarDistance = 10.0f;  // 0x374708
constexpr float kHeadIconLift = 1.0f;          // s17 of 0x1a0954
constexpr float kHeadIconScaleFactor = 2.0f;   // literal 0x1a0be4: lift = 1 + 2 * (entity +0x70)
// entity +0x70 of the player: 0.85 in all 990 updates of the 18 fall recordings of the player entity (Azahar oracle, state gameplay-start, 2026-10-13, tools/oracle/fall-*.txt); its writer is not read.
// STAND-IN HUD-S51.
constexpr float kPlayerEntityScaleMeasured = 0.85f;
constexpr float kHeadIconQuadSize = 1.0f;

class HeadIcon {
public:
    HeadIcon() : fade_(kHeadIconFadeSeconds, kHeadIconHoldSeconds) {}
    // 0x22d7c0 each update while the prompt is wanted.
    void request() { fade_.trigger(); }
    void update(float dt) { fade_.update(dt); }
    // 0x235e2c.
    float fade() const { return fade_.alpha(); }

private:
    CameraLockIcon fade_;
};

// The alpha byte of 0x1a0954: int(fade * 255), 0 below the near distance, scaled by (distance - 6) / 4 up to the far distance.
int headIconAlphaByte(float fade, float distance);

// The quad's centre: the player's node position lifted by 1 + 2 * scale.
std::array<float, 3> headIconPosition(const float node[3], float entityScale);

struct HeadIconScreen {
    float centreX = 0, centreY = 0;
    float width = 0, height = 0;
    bool visible = false;
};
// The centre and the size of the 1 x 1 world quad on the 400 x 240 top screen for a camera at `eye` looking along (yaw, pitch) with the
// vertical field of view (the projection of 0x24c1d0 inverted: scale y = 1 / tan(fov / 2), scale x = scale y / (400 / 240)).
HeadIconScreen projectHeadIcon(const std::array<float, 3>& position, const float eye[3], float yaw, float pitch, float verticalFov);

}

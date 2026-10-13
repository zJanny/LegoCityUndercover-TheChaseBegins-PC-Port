#pragma once
// Third-person follow camera of the original game ("OnFoot" preset), recovered from the camera system
// object at 0x589a70 (update 0x2e0bf0, strategy 0x330e4c / 0x3310d0 / 0x3313c0, tracker 0x1ed9ac) and the
// camera manager at 0x376870 (update 0x23f174); docs/re/camera.md. No SDL dependency.
//
// Recovered: the three tracked points, the exponential smoothing, the horizontal distance rope, the
// vertical spring, the L/R and stick yaw input and the yaw word 0x376894. Not recovered (see
// docs/re/camera.md): wall and player push-out (collision rays), the pitch clamp 0x2e0a50, the zoom
// input, the periodic re-snap, other presets (vehicle, skydive) and the Room/Bound pose blend that the
// game uses in indoor rooms. Those parts are absent here, so the camera is only exact in open space.
#include <cstdint>

namespace chase::runtime {

// 0x1e3730: quadratic through (0, a), (0.5, b), (1, c).
float quadraticSpline(float a, float b, float c, float t);

// Preset block of the "OnFoot" camera, 0x376f6c..0x376fdc (static data, offsets from the block start).
struct FollowCameraPreset {
    float distance[3] = {3.0f, 10.0f, 15.0f};      // +0x00 horizontal distance camera - player, zoom 0 / 1 / 2
    float height[3] = {-1.6f, 1.7f, 6.4f};         // +0x0c camera height above the look point
    float groundOffset = 0.25f;                    // +0x18 look point sits (body height - this) above the feet
    float lookHeight[3] = {0.3f, 0.2f, -0.2f};     // +0x1c look point lift (added to the body height)
    float lookDepth[3] = {0.5f, 0.0f, 0.5f};       // +0x28 look point shift along the player's facing
    float turnGain = 0.6f;                         // +0x40 yaw per update = (stick + shoulder) * dt * this
    float positionBase = 0.75f;                    // +0x48 camera position smoothing: 1 - base^(30 dt) per update
    float targetBase = 0.75f;                      // +0x4c look point smoothing
};

// Entity +0x64 + +0x70 of the player (1.15 + 0.85, read in the oracle).
constexpr float kPlayerBodyHeight = 2.0f;

// Entity types whose stick x turns the camera (0x200ae0): 1..5 and 0x11c.
bool stickTurnsCamera(std::uint16_t entityType);
// DAT_00589bac increment per update (0x200af0): dead zone 0.25 outside the channel dead zone, gain 4/3.
float stickTurnInput(float stickX, std::uint16_t entityType);
// DAT_00589bb0 increment per update (0x200ab0): R turns the camera towards +yaw, L towards -yaw, 2 each.
// The "invert camera" option (byte 0x51b493 bit 1) flips the sign.
float shoulderTurnInput(float left, float right, bool invert);

struct FollowCameraInput {
    float dt = 0;
    float target[3] = {0, 0, 0};       // player position (scene node world translation)
    float targetFacing = 0;            // player facing in radians, forward = (sin, cos) in (x, z)
    float stickTurn = 0;               // stickTurnInput of this update
    float shoulderTurn = 0;            // shoulderTurnInput of this update
};

class FollowCamera {
public:
    explicit FollowCamera(const FollowCameraPreset& preset = {}, float bodyHeight = kPlayerBodyHeight);

    // Place the camera behind the target at the given view yaw with all tracked points at rest.
    void reset(const float target[3], float yaw, float facing);
    void update(const FollowCameraInput& input);
    // Continue from a recorded state: wanted and tracked camera position, tracked look point (target + offset).
    void restore(const float wantedEye[3], const float trackedEye[3], const float trackedLook[3],
                 const float target[3], float facing);

    const float* eye() const { return position_; }
    const float* lookAt() const { return look_; }
    // The word 0x376894: atan2(forward.x, forward.z) of the view direction, radians.
    float yaw() const;
    float pitch() const;
    // PROVISIONAL: the pose of the street camera carries FOV 40 (C+0x7c in the oracle); how it is set
    // for this controller was not traced.
    static constexpr float kFieldOfViewDegrees = 40.0f;
    // Projection 0x23458c: y scale = cot(fov / 2) * 4/3, x scale = y scale / aspect. Returns the vertical field of
    // view in radians that this y scale corresponds to (30.5 degrees for a pose FOV of 40).
    static float verticalFieldOfView(float poseDegrees = kFieldOfViewDegrees);

private:
    struct Layout {
        float distance, height, lookUp, lookAhead, bodyUp;
    };
    Layout layout() const;
    void refreshPose();

    FollowCameraPreset preset_;
    float bodyHeight_;
    float zoom_ = 1.0f;
    // A: wanted camera position (K+4) and its tracker (K+0x34); B: wanted look point (K+0x64) and tracker
    // (K+0x94); C: look point offset (K+0xc4) and tracker (K+0xf4).
    float a_[3] = {}, aTracked_[3] = {};
    float b_[3] = {}, bTracked_[3] = {};
    float c_[3] = {}, cTracked_[3] = {};
    float position_[3] = {}, look_[3] = {};
};

}

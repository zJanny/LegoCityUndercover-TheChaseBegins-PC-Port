#pragma once
// Third-person follow camera of the original game ("OnFoot" preset), recovered from the camera system
// object at 0x589a70 (update 0x2e0bf0, strategy 0x330e4c / 0x3310d0 / 0x3313c0, tracker 0x1ed9ac) and the
// camera manager at 0x376870 (update 0x23f174); docs/re/camera.md. No SDL dependency.
//
// Recovered: the three tracked points, the exponential smoothing, the horizontal distance rope, the
// vertical spring, the L/R and stick yaw input, the yaw word 0x376894 and the obstruction handling of
// 0x3313c0 (CameraSweep: pull-in along the view line, sideways and vertical push). Not recovered (see
// docs/re/camera.md): collision with placed objects, the pitch clamp 0x2e0a50, the zoom input, the
// periodic re-snap, other presets (vehicle, skydive) and the Room/Bound pose blend that the game uses in
// indoor rooms.
#include "runtime/camera_sweep.hpp"
#include <cstdint>
#include <string>

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
    CameraSweepSettings sweep;                     // +0x58 / +0x5c / +0x60: apex reach, side push gain, vertical push gain
    float pitchLow = 0.25f;                        // +0x64 (in+0xd8): pitch clamp, look point at most this times the view depth below the head
    float pitchHigh = 0.25f;                       // +0x68 (in+0xdc): and above the feet
    float autoTurnGain = 0.0f;                     // +0x38: yaw per second towards the back of the target (1.0 for InVehicle)
    bool vehicleLayout = false;                    // byte +0x6e: the input provider 0x32f334 adds 10 to the distance and 2.5 to the height
                                                   // and 0x330e4c takes the look lift from the target model's half height
    static FollowCameraPreset onFoot() { return {}; }
    // InVehicle block at 0x376fdc; the overrides are the vehicle record's ATTR_Camera* floats (+0x2f0 PosY, +0x2f4 PosZ, +0x2f8
    // LookY, +0x2fc LookZ) that 0x1597c8 writes into the middle value of the distance, height, look lift and look depth triples when
    // record byte +0x311 bit 0 is set. PanScale (+0x300) is not read by that function.
    static FollowCameraPreset inVehicle();
    FollowCameraPreset& withVehicleOverrides(float posY, float posZ, float lookY, float lookZ);
};

// Entity +0x64 + +0x70 of the player (1.15 + 0.85, read in the oracle).
constexpr float kPlayerBodyHeight = 2.0f;

// Entity types whose stick x turns the camera (0x200ae0): 1..5 and 0x11c.
bool stickTurnsCamera(std::uint16_t entityType);
// DAT_00589bac increment per update (0x200af0): dead zone 0.25 outside the channel dead zone, gain 4/3.
// `seatedVehicleType` is the type byte (+0x12) of the vehicle the player drives (0 on foot): 0x200b0c clears the term for type 183.
float stickTurnInput(float stickX, std::uint16_t entityType, std::uint8_t seatedVehicleType = 0);
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

    // The subject the next updates follow: the preset block (0x23f488 stores K+0x160), the top of the target's collision box (entity
    // +0x64 + +0x70) and the half height of its model bounds (node +0xb0, read by vehicle presets).
    void setSubject(const FollowCameraPreset& preset, float bodyHeight, float modelHalfHeight);

    // Level collision the camera keeps out of (0x3313c0); without it the camera only follows.
    void setCollision(const CollisionWorld* world) { world_ = world; }
    // The placed object the camera follows (a vehicle) does not obstruct it: lower-case instance name, empty for none.
    void setExcludedOwner(const std::string& owner) { excludedOwner_ = owner; }

    // Place the camera behind the target at the given view yaw with all tracked points at rest.
    void reset(const float target[3], float yaw, float facing);
    void update(const FollowCameraInput& input);
    // The follow command taking over from another camera command (0x1b4864): the reset 0x2313c8 places the camera behind the
    // player's facing at the strategy distance, scaled by the clearance factor left by the last sweep (the strategy reset 0x3310b0 then sets it to 1), and
    // the camera starts at `currentEye` instead when the look point is closer to it than twice the preset distance. That near rule
    // only applies while the top command is a record pose (0x2a8c4c tests the routine 0x298cd8): `previousIsRecordPose`. Returns the
    // near case (the filter factors then ramp at 0.1 per update).
    bool takeOver(const float target[3], float facing, const float currentEye[3], bool previousIsRecordPose);
    // Continue from a recorded state: wanted and tracked camera position, tracked look point (target + offset).
    // `clearance` is the pull-in factor S+4 and `springHold` the vertical push timer S+0x24 of the recorded state.
    void restore(const float wantedEye[3], const float trackedEye[3], const float trackedLook[3],
                 const float target[3], float facing, float clearance = 1.0f, float springHold = 0.0f);
    // SnapCamera 0x1a30fc: the reset 0x2313c8 with argument 0, which tests each candidate with the ray test 0x21ab20 and pulls the
    // camera in with 0x23b92c. Seeds the camera behind the player's facing like `takeOver`, without the near rule.
    void snap(const float target[3], float facing);
    // K+0x178 is 360 after any reset; StartDCam 0x194fd8, StartDCamTrack 0x1da810, StartDCamTrackAuto 0x2861ac and StartNodelessDCamTrack
    // 0x2ad9f4 store 0 after pushing their director command. With 0, argument 360 takes the ray test branch.
    void clearStoredHeading();
    float clearance() const { return clearance_; }
    // Projection scales of the render camera the near plane test reads (camera +0xbc / +0xc0, written by 0x23458c from the previous pose).
    void setProjectionScales(float xScale, float yScale) { xScale_ = xScale, yScale_ = yScale; }
    // Camera +0xc4 written by 0x23f168 at the end of every update: the near plane distance (0x2e0e28).
    float nearPlane() const { return nearPlane_; }

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
    // The two scales of 0x23458c for the top screen (aspect 400 / 240).
    static void projectionScales(float poseDegrees, float& xScale, float& yScale);

private:
    struct Layout {
        float distance, height, lookUp, lookAhead, bodyUp;
        float baseDistance, baseHeight;  // before the clearance factor: in+0x50, in+0x54
    };
    Layout layout() const;
    void refreshPose();
    // The candidate loop of 0x2313c8 for the look point `b_` (A := B + candidate offset, then the line test 0x22b108 or the ray test
    // 0x21ab20 with the pull-in 0x23b92c); `lineTest` is the branch of argument 360 with the stored heading K+0x178 = 360.
    void placeCandidates(const Layout& layout, float facing, bool lineTest);
    float automaticTurn(const FollowCameraInput& input) const;
    void clampPitch(const FollowCameraInput& input, float poseScale);
    void snapToClearView();
    void avoidObstacles(const FollowCameraInput& input, const Layout& layout, bool moving, float push[3], const float swing[3]);

    FollowCameraPreset preset_;
    float bodyHeight_;
    float zoom_ = 1.0f;
    const CollisionWorld* world_ = nullptr;
    std::string excludedOwner_;
    float previousNear_ = -1.0f;   // K+0x1c8 left by the previous sweep (CameraSweepSettings::previousNear)
    float clearance_ = 1.0f;     // S+4: the camera sits at this fraction of its distance after obstructions
    float springHold_ = 0.0f;    // S+0x24: after a vertical push the height spring is off until it runs out
    float sideHold_ = 0.0f;      // S+0x1c: after a side push the automatic turn is off (factor 0.4 - this) until it runs out
    float modelHalfHeight_ = 0.0f;
    CameraSweep sweep_;
    bool swept_ = false;
    float xScale_, yScale_;
    float nearPlane_ = 1.0f;
    unsigned storedHeading_ = 0;  // K+0x178: 360 after any reset
    float previousLook_[3] = {}; // K+0x70: the look point of the previous update
    // A: wanted camera position (K+4) and its tracker (K+0x34); B: wanted look point (K+0x64) and tracker
    // (K+0x94); C: look point offset (K+0xc4) and tracker (K+0xf4).
    float a_[3] = {}, aTracked_[3] = {};
    float b_[3] = {}, bTracked_[3] = {};
    float c_[3] = {}, cTracked_[3] = {};
    float position_[3] = {}, look_[3] = {};
};

}

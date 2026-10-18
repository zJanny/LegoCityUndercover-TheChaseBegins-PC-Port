#pragma once
// The break of a vehicle and its wreck (docs/re/vehicles-damage.md): the stuck timer of a driven body (0x33aea8, 0x33b264..0x33b378) and the
// destroy message it makes 0x23019c send, the break 0x16cccc (message 1 of the class handler 0x159bd0), the wreck state 6 of 0x1765cc with the
// flying wheels of 0x1d27f4, the matrix history of the Drive update 0x307cac and the return of the car 0x22b224 / 0x1aff24 state 7.
// No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/game_random.hpp"
#include "runtime/vehicle_body.hpp"
#include "runtime/vehicle_damage.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// 0x33b264..0x33b378: speed limits (0x33b528 = 5.0, and 5.0 - 0x600000 in the float bits = 3.0), the yaw limit (immediate 0x3f800000), the
// seconds and the decay factor 0x33b52c = 0.25.
constexpr float kStuckDestroySpeed = 3.0f;
constexpr float kStuckDecaySpeed = 5.0f;
constexpr float kStuckDestroyYawRate = 1.0f;
constexpr float kStuckDestroySeconds = 3.0f;
constexpr float kStuckDecayRate = 0.25f;
// 0x230200..0x230218: the message 0 of the stuck destroy, word +0xc = 1000 and type byte +0x17 = 6.
constexpr std::uint32_t kStuckDestroyHearts = 1000;

struct DrivenStuckTimer {
    float seconds = 0;           // body+0x88
    bool destroyRequest = false;  // body+0x90 bit 0x40
};

// One world step of a driven body (flag +0xc4 bit 3). `forwardSpeed` is body+0xc0, `accelerate` and `brake` the body inputs +0x1c0 / +0x1c4,
// `yawRate` the rigid body's angular velocity y (+0x150).
void stepDrivenStuckTimer(DrivenStuckTimer& timer, float forwardSpeed, float accelerate, float brake, float yawRate, float dt);

// The head of 0x23019c (0x2301b0..0x230230): a forced stop zeroes the timer; otherwise a set request becomes message 0 (type 6, 1000 hearts) and
// the request is cleared.
std::optional<DamageMessage> stuckDestroyMessage(DrivenStuckTimer& timer, bool forcedStop);

// 0x16cccc: literals 0x16ce90 = 0.5, 0x16ce94 = 0.1, 0x16ce98 = 1.0; only the first four wheels are launched, the others are hidden.
constexpr float kWheelLaunchSideScale = 0.5f;
constexpr float kWheelLaunchJitter = 0.1f;
constexpr float kWheelLaunchLift = 1.0f;
constexpr std::size_t kWheelLaunchCount = 4;

// The launch vector of a wheel (record +0x320 + 12 n): the normalised direction from the vehicle node to the wheel node (0x257f14, 0x257ea0),
// x and z halved, then (r - 0.5) x 0.1 added to x, y (plus 1.0) and z in that order with r from 0x2345dc. Units per update.
Vector3 wheelLaunchVector(const Vector3& vehicleNode, const Vector3& wheelNode, GameRandom& random);

// 0x1765cc: a wreck turns into state 7 once its state timer (record +0x298) is above 2.0 (literal 0x1766c4); a launch vector of squared length up
// to 0.001 (0x1766d4) is not moved; the spin passed every update is the stack triple {0.1, 0.2, 0.14} (0x1766c8..0x1766d0), the bounce factor
// 0.5 (0x1766d8) and the fall per update the global 0x37716c = 0.075 (static initialiser 0x350708).
constexpr float kWreckSeconds = 2.0f;
constexpr float kWheelPieceMinSquare = 0.001f;
constexpr Vector3 kWheelPieceSpin = {0.1f, 0.2f, 0.14f};
constexpr float kWheelPieceBounce = 0.5f;
constexpr float kWheelPieceFall = 0.075f;
// 0x1d27f4: a hit at a speed below 2.0 (0x1d2a70) with |x| and |z| of the new velocity below 0.125 (0x3e000000) ends the update at the contact.
constexpr float kWheelPieceRestSpeed = 2.0f;
constexpr float kWheelPieceRestSide = 0.125f;

struct WheelPieceHit {
    Vector3 point = {0, 0, 0};
    Vector3 normal = {0, 1, 0};
};
// The line test of 0x298274 (0x21ab20 from the centre to centre + velocity + radius along the velocity).
using WheelPieceSweep = std::function<std::optional<WheelPieceHit>(const Vector3& from, const Vector3& to)>;

// One update of a flying wheel, 0x1d27f4 as 0x1765cc calls it: `world` is the wheel node's world matrix (row vectors, translation in [12..14]),
// `centre` and `radius` the node bounds (+0xa0, +0xac), `velocity` the launch vector. Returns true when the wheel came to rest at a contact.
bool stepWheelPiece(assets::Matrix& world, const Vector3& centre, float radius, Vector3& velocity, const WheelPieceSweep& sweep, GameRandom& random);

// 0x1765cc calls 0x1d27f4 only for a launch vector whose squared length is above 0.001.
bool wheelPieceMoving(const Vector3& velocity);

// The wreck of a broken vehicle: the launch vectors and world matrices of the launched wheels and the state timer record +0x298 (0x1aff24 sets it
// to 0 when the state becomes 6 and adds dt in the same update).
struct VehicleWreck {
    bool active = false;
    bool returnDue = false;  // requested state 7
    float stateSeconds = 0;
    std::size_t wheels = 0;
    std::array<Vector3, kWheelLaunchCount> launch{};
    std::array<assets::Matrix, kWheelLaunchCount> wheelWorld{};
};

// The break 0x16cccc for the wheels at `wheelWorld` (node world matrices, the first kWheelLaunchCount are launched) of a vehicle whose node
// stands at `vehicleNode`: requested state 6 is set by the caller (record +4).
VehicleWreck breakVehicle(const assets::Matrix& vehicleNode, const std::vector<assets::Matrix>& wheelWorld, GameRandom& random);

// One update of the wreck state 6 (0x1765cc after 0x1aff24): the timer, then state 7 is requested once it is above kWreckSeconds.
void stepWreckTimer(VehicleWreck& wreck, float dt);

// 0x22b224 with 0x1aff24 case 7 for the player's vehicle: hearts = the table health (+0x30d = +0x30e), health points 100, brick cool-down 0,
// the dead bit (entity +0xc bit 0x10) cleared, and the state 5 at once (the case writes +4 = 5 before +2 follows it). The exception for a vehicle
// with a link (+0x2a4 of type 5 and +0x311 bit 7 keeps the hearts) is not ported.
void restoreVehicleRecord(VehicleRecord& record);

// 0x24257c: record +0x312 bit 3 is clear (the place is good to return to) when every wheel touches the ground (body +0xda0 >= +0x2b4), no wheel is
// in a water volume (0x2d8d08 with 0.5), the node's up row dotted with the world up is at least cos(0x3e32b8c3 = 10 degrees), the body's vertical
// velocity is at least -4.0 and the vehicle is not dead. STAND-IN VEH-S97: no water volumes exist in the port and the area test 0x2a0810 of the
// player's vehicle is not read; both count as passed.
constexpr float kReturnPlaceMaxTilt = 0.1745329350f;
constexpr float kReturnPlaceMinVerticalSpeed = -4.0f;
bool vehicleReturnPlaceGood(std::size_t wheelsOnGround, std::size_t wheelCount, float upDot, float verticalSpeed, bool dead);

// 0x307cac: while the player drives, the vehicle is not in state 6 or 7 and record +0x312 bit 3 is clear, the node matrix goes into the ring of
// seven at record +0x78 (index +0x278) and record +0x38 becomes the oldest entry. STAND-IN VEH-S96: the start value of the ring for the vehicle
// class 183 is not read (the class 179 fills it with its placement in 0x165810); the placement is used.
constexpr std::size_t kVehicleMatrixHistory = 7;

class VehicleMatrixHistory {
public:
    explicit VehicleMatrixHistory(const assets::Matrix& placementStandIn = assets::Matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
    void record(const assets::Matrix& node);
    const assets::Matrix& returnMatrix() const { return return_; }  // record +0x38

private:
    std::array<assets::Matrix, kVehicleMatrixHistory> ring_{};
    std::uint32_t index_ = 0;
    assets::Matrix return_{};
};

}

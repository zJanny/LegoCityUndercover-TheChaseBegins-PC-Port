#pragma once
// The player side of getting into and out of a vehicle (docs/re/vehicles-boarding.md, entry 2026-10-15T08:15:49): the search for a vehicle in reach 0x225d94, the
// enter event 0x322a04, the states Drive Jump In 0x11a (begin 0x31fffc, update 0x3201b0), Drive 0x11c (begin 0x307abc, end 0x307bcc) and
// Drive Jump Out 0x11b (begin 0x3222f4, update 0x322778, end 0x32273c), the leave test 0x322aa4 and the candidate search for the exit
// position. The vehicle's matrix is the caller's; driving moves it (vehicle_drive.hpp). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/player_vertical.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint16_t kStateDriveJumpIn = 0x11a;
constexpr std::uint16_t kStateDriveJumpOut = 0x11b;
constexpr std::uint16_t kStateDrive = 0x11c;

// 0x225d94: a vehicle is taken when its squared distance to the player is below 4.0 (*0x225f4c), the player position is tested against the
// entity box in the vehicle's frame and the height rule uses the slack 0.25 (*0x225f58).
constexpr float kBoardRangeSquared = 4.0f;
constexpr float kBoardHeightSlack = 0.25f;

// 0x3201b0 and 0x322778: the player follows the line between the start and the target plus a sine arc of the player's jump height *0x3742a0.
constexpr float kBoardArcHeight = kJumpHeight;

// 0x3222f4: candidate exit offsets in the vehicle frame. The angles (degrees, table 0x357bfc) are tried in this order; the distance on each axis is
// the larger of 2.0 (*0x322718) and 1.0 (*0x322700) plus the half extent of the entity box.
constexpr std::array<float, 8> kExitAnglesDegrees = {90.0f, 270.0f, 0.0f, 180.0f, 45.0f, 135.0f, 225.0f, 315.0f};
constexpr float kExitMinimumDistance = 2.0f;
constexpr float kExitMargin = 1.0f;
// 0x3222f4 lowers the candidate by 12.0 (*0x32271c) when 0x229e6c(vehicle) is 1 and by 4.0 (*0x322720) otherwise; the vehicles of mission 1-1 are
// taken as the second case (STAND-IN VEH-S8, the meaning of 0x229e6c is not read).
constexpr float kExitDepth = 4.0f;

// One vehicle the player can board. `matrix` is the node world matrix (row vectors, translation last), the box is entity +0x60 / +0x6c.
struct BoardingVehicle {
    std::string instance;  // lower case
    assets::Matrix matrix{};
    float boxCentre[3] = {0, 0, 0};
    float boxHalf[3] = {0, 0, 0};
    float driverPosition[3] = {0, 0, 0};  // record +0x27c
    float centreOfMass[3] = {0, 0, 0};    // class value CentreOfMass: the origin of the physics body, where the hidden player sits (0x307cac)
    bool disabled = false;                // owner flag bit 0
    bool locked = false;                  // record +0x311 bit 1
    bool jumpOutAllowed = true;           // record +0x311 bit 6 clear
};

struct BoardingPlayer {
    float position[3] = {0, 0, 0};
    float boxCentreY = 1.15f;  // entity +100
    float boxHalfY = 0.85f;    // entity +112
    int carryMode = 0;         // 0x225754: 2 is a big object in the hands
};

std::array<float, 3> boardingToLocal(const assets::Matrix& matrix, const float point[3]);
std::array<float, 3> boardingToWorld(const assets::Matrix& matrix, const float point[3]);
// 0x270498: distance from a point to a box on the x and z axes (the y component of the offset is never written, 0 inside).
float distanceToBox(const float point[3], const float centre[3], const float half[3]);

// 0x225d94 with the character excluded argument 0: the nearest vehicle that passes the rules, or none.
std::optional<std::size_t> findVehicleToEnter(const std::vector<BoardingVehicle>& vehicles, const BoardingPlayer& player);

// Position of the physics body origin in the world: the hidden player is placed there while the state is Drive.
std::array<float, 3> bodyPosition(const BoardingVehicle& vehicle);
// Seat position in the world, the target of the jump in and the start of the jump out: the vehicle node transforms the record's DriverPosition.
std::array<float, 3> seatPosition(const BoardingVehicle& vehicle);
// Heading of the vehicle node as 0x24765c computes it from the forward axis (x, z): atan2(x, z), radians.
float vehicleHeading(const BoardingVehicle& vehicle);

// Ground below a candidate: the height of the floor found within `depth` below (x, y, z), or nothing.
using GroundBelow = std::function<std::optional<float>(float x, float y, float z, float depth)>;

// 0x3222f4: the first candidate with floor below it. Without one the original puts the player on top of the vehicle (0x3225a8).
std::array<float, 3> chooseExitPosition(const BoardingVehicle& vehicle, const GroundBelow& ground, float playerBoxHalfY);

enum class BoardingPhase { OnFoot, JumpIn, Seated, JumpOut };

enum class BoardingEventKind {
    JumpInStarted,  // state 0x11a begins
    Entered,        // state 0x11c begins: PlayerVehicleEntered (0x307b6c, 0x23cee8 with *0x375fe4)
    Exiting,        // state 0x11b begins: PlayerVehicleExiting (0x322688 with *0x375fe8)
    Left,           // state 1 again
};

struct BoardingEvent {
    BoardingEventKind kind = BoardingEventKind::JumpInStarted;
    std::size_t vehicle = 0;
};

struct BoardingInput {
    bool action = false;      // the X button went down in this update
    bool playerFree = true;   // a ground state of the player that takes event 200, and no script holds his controls
    BoardingPlayer player;
    std::int16_t facing = 0;  // player heading, 65536 = 2 pi
    std::uint32_t ticks = 100;  // clock ticks of this update (3000 per second)
    GroundBelow ground;
};

struct BoardingStep {
    BoardingPhase phase = BoardingPhase::OnFoot;
    bool takesControl = false;      // the player's own update does not run
    bool placed = false;            // position and facing replace the player's
    std::array<float, 3> position = {0, 0, 0};
    std::int16_t facing = 0;
    bool visible = true;            // 0x23f734(node, 1) / (node, 0)
    bool jumpAnimation = false;     // role 3 plays
    std::vector<BoardingEvent> events;
};

// Length of an animation in clock ticks: (frames - 1) intervals at the animation rate.
std::uint32_t animationLengthTicks(unsigned frames, float framesPerSecond);

class VehicleBoarding {
public:
    // Length of the jump animation in clock ticks: the state ends when the elapsed ticks reach it (0x3201b0 divides by 0x23caa4 of the slot).
    explicit VehicleBoarding(std::uint32_t jumpLengthTicks) : length_(jumpLengthTicks) {}

    BoardingStep update(const std::vector<BoardingVehicle>& vehicles, const BoardingInput& input);

    BoardingPhase phase() const { return phase_; }
    // 0x24255c: the vehicle while the state is 0x11c, none during the jumps.
    std::optional<std::size_t> seatedVehicle() const { return phase_ == BoardingPhase::Seated ? std::optional<std::size_t>(vehicle_) : std::nullopt; }

private:
    BoardingStep step(BoardingPhase phase, const std::array<float, 3>& position, std::int16_t facing, bool visible, bool takesControl) const;
    std::array<float, 3> arc(float u) const;

    std::uint32_t length_;
    BoardingPhase phase_ = BoardingPhase::OnFoot;
    std::size_t vehicle_ = 0;
    std::uint32_t elapsed_ = 0;
    std::array<float, 3> from_ = {0, 0, 0};
    std::array<float, 3> to_ = {0, 0, 0};
    float fromHeading_ = 0, toHeading_ = 0;
};

}

// Script jump in and out of characters other than the player (src/runtime/character_vehicle_entry.cpp, docs/re/vehicles-waypoints.md): event 200
// of VehicleJumpIn 0x1cdec0 taken in the states 1 to 3 (0x322a04), the states 0x11a, 0x11c and 0x11b of the player's boarding, the leave test 0x322aa4 and
// the CharacterLeftVehicle that the end of the jump out gives. Handlers and state bindings read with llvm-objdump by 20261111-chase-C (2026-11-12);
// made-up vehicles and characters, no game data needed.
#include "runtime/character_vehicle_entry.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::BoardingEventKind;
using runtime::BoardingPhase;
using runtime::BoardingVehicle;
using runtime::CharacterAboard;
using runtime::CharacterVehicleEntry;

constexpr std::uint32_t kJumpTicks = 1000;
constexpr std::uint32_t kTicksPerUpdate = 100;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void requireNear(float actual, float expected, const std::string& what) {
    require(std::fabs(actual - expected) <= 1e-4f, what + ": " + std::to_string(actual) + " against " + std::to_string(expected));
}

BoardingVehicle car(const std::string& name, float x, float z) {
    BoardingVehicle vehicle;
    vehicle.instance = name;
    vehicle.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, z, 1};
    vehicle.boxHalf[0] = 1.0f;
    vehicle.boxHalf[1] = 0.75f;
    vehicle.boxHalf[2] = 2.5f;
    vehicle.boxCentre[1] = 0.75f;
    vehicle.driverPosition[0] = -0.4f;
    vehicle.driverPosition[1] = 0.5f;
    vehicle.centreOfMass[1] = 0.6f;
    return vehicle;
}

CharacterAboard standingAt(float x, float z) {
    CharacterAboard aboard;
    aboard.position = {x, 0, z};
    return aboard;
}

const runtime::GroundBelow kFlatGround = [](float, float, float, float) { return std::optional<float>(0.0f); };

bool hasEvent(const runtime::BoardingStep& step, BoardingEventKind kind) {
    for (const runtime::BoardingEvent& event : step.events)
        if (event.kind == kind) return true;
    return false;
}

std::vector<runtime::CharacterVehicleStep> run(CharacterVehicleEntry& entry, const std::vector<BoardingVehicle>& vehicles, const CharacterAboard& aboard,
                                               unsigned updates) {
    std::vector<runtime::CharacterVehicleStep> last;
    for (unsigned i = 0; i < updates; ++i)
        last = entry.update(vehicles, [&](const std::string&) { return aboard; }, kTicksPerUpdate, kFlatGround);
    return last;
}

void jumpInSeatsAndHides() {
    std::vector<BoardingVehicle> vehicles = {car("other", 40, 0), car("stolen", 0, 0)};
    CharacterVehicleEntry entry(kJumpTicks);
    const auto begun = entry.jumpIn("chaser", 1, vehicles, standingAt(10, 0));
    require(begun && begun->character == "chaser", "the event is taken in a standing state, with no search: the vehicle 10 m away is boarded");
    require(begun->step.phase == BoardingPhase::JumpIn && hasEvent(begun->step, BoardingEventKind::JumpInStarted), "0x322a04 pushes 0x11a at once");
    requireNear(begun->step.position[0], 10, "the jump starts where the character stands");
    require(entry.boardedVehicle("chaser") == std::optional<std::size_t>(1) && !entry.seatedVehicle("chaser"), "0x22ec10 answers during the jump, 0x24255c not");
    require(!entry.hidden("chaser"), "shown during the jump in");
    require(!entry.jumpIn("chaser", 0, vehicles, standingAt(10, 0)), "the jump state 0x11a has no Vehicle Enter Event");

    auto steps = run(entry, vehicles, standingAt(10, 0), kJumpTicks / kTicksPerUpdate - 1);
    require(steps.size() == 1 && steps[0].step.phase == BoardingPhase::JumpIn, "the jump lasts the clip's length");
    steps = run(entry, vehicles, standingAt(10, 0), 1);
    require(steps.size() == 1 && hasEvent(steps[0].step, BoardingEventKind::Entered), "the end of the jump pushes the Drive state 0x11c");
    require(entry.seatedVehicle("chaser") == std::optional<std::size_t>(1) && entry.hidden("chaser"), "0x307abc hides the seated character");
    requireNear(steps[0].step.position[1], 0.6f, "the hidden character sits at the body origin (the centre of mass)");

    vehicles[1].matrix[12] = 25.0f;
    steps = run(entry, vehicles, standingAt(10, 0), 1);
    requireNear(steps[0].step.position[0], 25.0f, "the seated character follows the moved vehicle");
}

void notTakenOutsideTheEnterStates() {
    std::vector<BoardingVehicle> vehicles = {car("stolen", 0, 0)};
    CharacterVehicleEntry entry(kJumpTicks);
    CharacterAboard flying = standingAt(3, 0);
    flying.enterEventState = false;
    require(!entry.jumpIn("chaser", 0, vehicles, flying), "only the states 1, 2 and 3 carry the Vehicle Enter Event (binder 0x25e770)");
    require(entry.boardedCharacters().empty(), "nothing is boarded");
    require(!entry.jumpIn("chaser", 1, vehicles, standingAt(3, 0)), "an unknown vehicle is no event argument");
    require(!entry.jumpOut("chaser", vehicles, standingAt(3, 0), kFlatGround), "a character on foot has no Drive state to leave (0x24255c is 0)");
}

void jumpOutGivesLeftVehicle() {
    std::vector<BoardingVehicle> vehicles = {car("stolen", 0, 0)};
    CharacterVehicleEntry entry(kJumpTicks);
    entry.jumpIn("chaser", 0, vehicles, standingAt(4, 0));
    run(entry, vehicles, standingAt(4, 0), kJumpTicks / kTicksPerUpdate);
    require(entry.seatedVehicle("chaser").has_value(), "seated");

    vehicles[0].jumpOutAllowed = false;
    require(!entry.jumpOut("chaser", vehicles, standingAt(0, 0), kFlatGround), "the leave test 0x322aa4 refuses a vehicle that denies the jump out, is stuck or destroyed");
    require(entry.seatedVehicle("chaser").has_value(), "still seated after the refusal");
    vehicles[0].jumpOutAllowed = true;

    const auto leaving = entry.jumpOut("chaser", vehicles, standingAt(0, 0), kFlatGround);
    require(leaving && leaving->step.phase == BoardingPhase::JumpOut && hasEvent(leaving->step, BoardingEventKind::Exiting), "0x322aa4 pushes 0x11b at once");
    require(!entry.hidden("chaser") && entry.takeLeft().empty(), "shown again, not yet on foot");
    auto steps = run(entry, vehicles, standingAt(0, 0), kJumpTicks / kTicksPerUpdate);
    require(steps.size() == 1 && hasEvent(steps[0].step, BoardingEventKind::Left), "the end of the jump out puts the character on foot");
    const std::vector<runtime::CharacterLeftVehicle> left = entry.takeLeft();
    require(left.size() == 1 && left[0].character == "chaser" && left[0].vehicle == "stolen", "CharacterLeftVehicle names the character and the vehicle");
    requireNear(left[0].position[0], 2.0f, "the first exit candidate of 0x3222f4: 90 degrees, the larger of 2.0 and 1.0 plus the half width 1.0");
    requireNear(left[0].position[1], 0.0f, "on the ground below the candidate");
    require(entry.boardedCharacters().empty() && !entry.boardedVehicle("chaser"), "a character on foot is dropped");
    require(entry.takeLeft().empty(), "the event is taken once");
}

}

int main() {
    try {
        jumpInSeatsAndHides();
        notTakenOutsideTheEnterStates();
        jumpOutGivesLeftVehicle();
    } catch (const std::exception& error) {
        std::cerr << "character-vehicle-entry: " << error.what() << '\n';
        return 1;
    }
    std::cout << "character-vehicle-entry: ok\n";
    return 0;
}

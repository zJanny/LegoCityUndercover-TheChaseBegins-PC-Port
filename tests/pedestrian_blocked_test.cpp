// The pedestrian's vehicle test 0x1ffcd4 and its list loop 0x26cd38 (docs/re/pedestrians-inputs.md). Provenance of the pinned values:
// llvm-objdump of artifacts/game/game.arm.elf, 0x1ffcd4 .. 0x20013c. Margins 0.9 (0x200048) on the half x and 1.8 (0x20004c) on
// the half z; reach = half z + 1.8 + 8.0 (0x200050) x |speed x dt|; the distance gate (vehicle +0x5c + pedestrian +0x5c + reach)
// squared against the next point (0x24ef70); |speed x dt| at most 0.1 (0x200054, bits) takes the slow branch: inside the
// margins it returns 4 when (vehicle - position) . (target - position) > 0 and slides the vector off the x and z faces it
// crosses (0x22f928 with (row 0 x, 0, row 0 z) and (row 2 x, 0, row 2 z), negated on the near side). The fast branch with walk
// byte +0x62 bit 0 and |local z| below the reach casts 0x22b108 from position + 0.25 up (0x200064) along row 0 x +/-5.0
// (*0x37449c = 5.0 from the static initialiser 0x347904; the sign follows local x) and requests 3 / 4 (table 0x357e1c), the
// dodge side +0x5f = random % 2 (0x235498(2)) for the first side while reach > speed, else 1; result 3. A pedestrian still
// in state 2 inside the box itself requests 6 and clears +0x62. The loop skips kinematic bodies (rigid +0xcc bit 1) and
// returns the first non-zero result. Expected numbers below are worked by hand from these rules.
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_blocked.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }
bool nearVec(const PedestrianVec3& a, const PedestrianVec3& b) { return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]); }

PedestrianBlockVehicle vehicleAt(const PedestrianVec3& position, float speed) {
    PedestrianBlockVehicle vehicle;
    vehicle.matrix = {PedestrianVec3{1, 0, 0}, PedestrianVec3{0, 1, 0}, PedestrianVec3{0, 0, 1}, position};
    vehicle.halfExtents = {1.0f, 1.0f, 2.0f};
    vehicle.radius = 2.0f;
    vehicle.speed = speed;
    return vehicle;
}

PedestrianBody walkingBody(const PedestrianVec3& target, bool walkStarted) {
    PedestrianBody body;
    body.state = 2;
    body.requested = 2;
    body.target = target;
    body.walkStarted = walkStarted;
    return body;
}

struct Rays {
    std::vector<std::pair<PedestrianVec3, PedestrianVec3>> cast;
    std::vector<bool> answers;
    PedestrianLineTest test() {
        return [this](const PedestrianVec3& from, const PedestrianVec3& to) {
            const bool hit = cast.size() < answers.size() && answers[cast.size()];
            cast.emplace_back(from, to);
            return hit;
        };
    }
};

constexpr float kRadius = 0.5f;

void farVehicleIsIgnored() {
    PedestrianBody body = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    GameRandom random(7);
    Rays rays;
    const int result = pedestrianVehicleBlock(body, kRadius, {20, 0, 0}, vector, 0.1f, vehicleAt({0, 0, 0}, 0), rays.test(), random);
    require(result == 0 && body.requested == 2 && rays.cast.empty(), "beyond 2.5 + 3.8 the vehicle is not tested");
}

void slowVehicleTurnsBackTowardsATargetBehindIt() {
    GameRandom random(7);
    Rays rays;
    PedestrianVec3 vector{0, 0, 0};
    PedestrianBody behind = walkingBody({0, 0, -10}, true);
    require(pedestrianVehicleBlock(behind, kRadius, {0, 0, 3.5f}, vector, 0.1f, vehicleAt({0, 0, 0}, 0), rays.test(), random) == 4,
            "local z 3.5 inside 2 + 1.8, target past the vehicle: 4");
    PedestrianBody ahead = walkingBody({0, 0, 10}, true);
    require(pedestrianVehicleBlock(ahead, kRadius, {0, 0, 3.5f}, vector, 0.1f, vehicleAt({0, 0, 0}, 0), rays.test(), random) == 0,
            "target on the pedestrian's side: 0");
    PedestrianBody outside = walkingBody({0, 0, -10}, true);
    require(pedestrianVehicleBlock(outside, kRadius, {0, 0, 3.9f}, vector, 0.1f, vehicleAt({0, 0, 0}, 0), rays.test(), random) == 0,
            "local z 3.9 is past the 3.8 margin: 0");
    require(rays.cast.empty() && behind.requested == 2, "the slow branch casts no ray and requests nothing");
}

void slowVehicleSlidesTheVectorOffAFace() {
    GameRandom random(7);
    Rays rays;
    PedestrianBody body = walkingBody({5, 0, 0}, true);
    PedestrianVec3 vector{-1.0f, 0, 0.5f};
    const int result = pedestrianVehicleBlock(body, kRadius, {2.5f, 0, 0}, vector, 0.1f, vehicleAt({0, 0, 0}, 0.5f), rays.test(), random);
    require(result == 0, "target away from the vehicle: 0");
    require(nearVec(vector, {0, 0, 0.5f}), "crossing x = 1.9 from 2.5 to 1.5 removes the x part");
}

void slowRotatedVehicleSlidesOffItsNearFace() {
    GameRandom random(7);
    Rays rays;
    PedestrianBlockVehicle vehicle = vehicleAt({10, 0, 0}, 0);
    vehicle.matrix[0] = {0, 0, 1};
    vehicle.matrix[2] = {-1, 0, 0};
    PedestrianBody body = walkingBody({10, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 1.0f};
    const int result = pedestrianVehicleBlock(body, kRadius, {10, 0, -2.5f}, vector, 0.1f, vehicle, rays.test(), random);
    require(result == 0, "rotated: target on the pedestrian's side");
    require(nearVec(vector, {0, 0, 0}), "local x -2.5 to -1.5 crosses the -1.9 face: the negated row 0 removes the move");
}

void fastVehicleRequestsADodge() {
    Rays rays;
    GameRandom random(7);
    GameRandom expected = random;
    const std::uint32_t side = expected.next() % 2;
    PedestrianBody body = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    const int result = pedestrianVehicleBlock(body, kRadius, {1.0f, 0, 5.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 3 && body.requested == 3, "speed 10 x 0.1: reach 11.8 covers local z 5; the right dodge (3)");
    require(body.dodgeSide == side && random.table() == expected.table(), "reach 11.8 > speed 10: the side is random % 2");
    require(rays.cast.size() == 1 && nearVec(rays.cast[0].first, {1.0f, 0.25f, 5.0f}) && nearVec(rays.cast[0].second, {6.0f, 0.25f, 5.0f}),
            "one ray from 0.25 above the position, 5 along row 0");
    require(body.walkStarted, "a dodge keeps the walk byte");
}

void blockedFirstSideTakesTheOther() {
    Rays rays;
    rays.answers = {true, false};
    GameRandom random(7);
    const GameRandom before = random;
    PedestrianBody body = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    const int result = pedestrianVehicleBlock(body, kRadius, {1.0f, 0, 5.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 3 && body.requested == 4 && body.dodgeSide == 1, "second side: state 4, side 1");
    require(random.table() == before.table(), "no draw for the second side");
    require(rays.cast.size() == 2 && nearVec(rays.cast[1].second, {-4.0f, 0.25f, 5.0f}), "the second ray goes -5 along row 0");
}

void leftOfTheVehicleTriesLeftFirst() {
    Rays rays;
    GameRandom random(7);
    PedestrianBody body = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    const int result = pedestrianVehicleBlock(body, kRadius, {-1.0f, 0, 5.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 3 && body.requested == 4, "local x < 0: state 4 first");
    require(nearVec(rays.cast[0].second, {-6.0f, 0.25f, 5.0f}), "local x < 0: -5 along row 0 first");
}

void fastVehicleBeyondItsReachGivesSideOne() {
    Rays rays;
    GameRandom random(7);
    const GameRandom before = random;
    PedestrianBody body = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    const int result = pedestrianVehicleBlock(body, kRadius, {1.0f, 0, 5.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 20.0f), rays.test(), random);
    require(result == 3 && body.requested == 3 && body.dodgeSide == 1, "speed 20: reach 19.8 <= 20, side 1");
    require(random.table() == before.table(), "no draw when the reach does not exceed the speed");
}

void noFreeSideAndNoContactLeavesTheWalk() {
    Rays rays;
    rays.answers = {true, true};
    GameRandom random(7);
    PedestrianBody body = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    const int result = pedestrianVehicleBlock(body, kRadius, {1.0f, 0, 5.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 0 && body.requested == 2 && body.walkStarted, "both sides hit, outside the box: nothing");
}

void insideTheBoxSplats() {
    Rays rays;
    rays.answers = {true, true};
    GameRandom random(7);
    PedestrianBody walking = walkingBody({0, 0, -10}, true);
    PedestrianVec3 vector{0, 0, 0};
    int result = pedestrianVehicleBlock(walking, kRadius, {0.5f, 0, 1.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 0 && walking.requested == 6 && !walking.walkStarted, "inside half 1 x 2 with no free side: splat (6)");
    PedestrianBody fresh = walkingBody({0, 0, -10}, false);
    result = pedestrianVehicleBlock(fresh, kRadius, {0.5f, 0, 1.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 0 && fresh.requested == 6 && rays.cast.size() == 2, "without the walk byte no ray is cast; splat");
    PedestrianBody dodging = walkingBody({0, 0, -10}, false);
    dodging.requested = 3;
    result = pedestrianVehicleBlock(dodging, kRadius, {0.5f, 0, 1.0f}, vector, 0.1f, vehicleAt({0, 0, 0}, 10.0f), rays.test(), random);
    require(result == 0 && dodging.requested == 3, "only a request of 2 turns into a splat");
}

void listSkipsKinematicBodiesAndReturnsTheFirstResult() {
    Rays rays;
    GameRandom random(7);
    PedestrianBlockVehicle kinematic = vehicleAt({3.0f, 0, 0}, 0);
    kinematic.kinematic = true;
    const std::vector<PedestrianBlockVehicle> vehicles{kinematic, vehicleAt({0, 0, 0}, 0), vehicleAt({3.5f, 0, 0}, 0)};
    PedestrianBody body = walkingBody({5, 0, 0}, true);
    PedestrianVec3 vector{-1.0f, 0, 0.5f};
    const int result = pedestrianBlocked(body, kRadius, {2.5f, 0, 0}, vector, 0.1f, vehicles, rays.test(), random);
    require(result == 4, "the third vehicle turns the pedestrian back");
    require(nearVec(vector, {0, 0, 0.5f}), "the second vehicle slid the vector; the kinematic one was skipped");

    PedestrianVec3 still{0, 0, 0};
    require(pedestrianBlocked(body, kRadius, {20, 0, 20}, still, 0.0f, vehicles, rays.test(), random) == 0, "nothing near: 0");
}
}

int main() {
    try {
        farVehicleIsIgnored();
        slowVehicleTurnsBackTowardsATargetBehindIt();
        slowVehicleSlidesTheVectorOffAFace();
        slowRotatedVehicleSlidesOffItsNearFace();
        fastVehicleRequestsADodge();
        blockedFirstSideTakesTheOther();
        leftOfTheVehicleTriesLeftFirst();
        fastVehicleBeyondItsReachGivesSideOne();
        noFreeSideAndNoContactLeavesTheWalk();
        insideTheBoxSplats();
        listSkipsKinematicBodiesAndReturnsTheFirstResult();
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-blocked-test: " << error.what() << "\n";
        return 1;
    }
    std::cout << "pedestrian-blocked-test: ok\n";
    return 0;
}

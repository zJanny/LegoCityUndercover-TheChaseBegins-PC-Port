// Tests of the vehicle body's floor collision (src/runtime/vehicle_collision.*, docs/re/vehicles-physics.md). Pinned values: Azahar GDB reads
// and memory traces of 2026-10-15 (agent VEH40, not committed): the probe points body+0xdc of the trooper
// (state drv-police-car-seated, body 0x08e16100) and of the donut van (m11-5-in-donut-van, body 0x08e12880); drop3.mem: the parked trooper
// with rigid +0x38 and +0x78 poked to 25.0 and mode +0xd8 to 1, fixed dt 0.033, rigid body 0x1d8 bytes per update; in update 20 the body
// lands (ctrace: 48 calls of 0x15c4a4, 6 of 0x19c0a8, one of 0x1b79ac). The real part replays update 20 on lvl_01. Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/vehicle_collision.hpp"
#include "vehicle_trooper.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what, double got = 0.0) {
    if (!ok) {
        std::printf("FAIL: %s (got %.6f)\n", what, got);
        ++failures;
    }
}

void near(double got, double want, double tolerance, const char* what) { expect(std::fabs(got - want) <= tolerance, what, got); }

std::array<float, 4> orientationFromAxes(const Vector3& right, const Vector3& up, const Vector3& forward) {
    const float m[3][3] = {{right[0], up[0], forward[0]}, {right[1], up[1], forward[1]}, {right[2], up[2], forward[2]}};
    const float trace = m[0][0] + m[1][1] + m[2][2];
    const float s = std::sqrt(trace + 1.0f) * 2.0f;
    return {(m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s, 0.25f * s};
}

void probePoints() {
    const VehicleRecord record = trooper::record();
    const VehicleCollisionBody shape = trooper::collisionBody(record);
    // Trooper body+0xdc..+0x158 as read: words ec3678bf 0023853b 020037c0 (point 0) and a1645f3f 19fcd33f 53b81940 (point 5).
    expect(shape.points[0] == Vector3{-0x1.f06dd8p-1f, 0x1.0a46p-8f, -0x1.6e0004p+1f}, "trooper probe point 0");
    expect(shape.points[3] == Vector3{0x1.f06dd8p-1f, 0x1.0a46p-8f, 0x1.55999cp+1f}, "trooper probe point 3");
    near(shape.points[5][0], 0x1.bec942p-1, 1e-6, "trooper probe point 5 x (0.9 of the box)");
    near(shape.points[5][2], 0x1.3370a6p+1, 1e-6, "trooper probe point 5 z (0.9 of the box)");
    expect(shape.points[5][1] == 0x1.a7f832p+0f, "trooper probe point 5 y (box top)");
    Vector3 low, high;
    vehicleShapeBox({0.0f, 1.4032f, 0.0f}, {1.14375f, 2.54695f, 2.66857f}, {0.0f, -0.9f, 0.2f}, low, high);
    near(low[1], -0.24375, 1e-5, "van child box low y (oracle m11-5-in-donut-van, rigid 0x08e136c0 compound child)");
    near(high[1], 4.85015, 1e-5, "van child box high y (oracle m11-5-in-donut-van)");
    const std::array<Vector3, 8> van = vehicleProbePoints(low, high, {});
    near(van[6][2], -2.8685700 * 0.9, 1e-5, "van probe point 6 z (oracle child z -2.86857, times 0.9)");
    near(van[4][1] - 1.0f, 4.85015 - 1.0, 1e-5, "a body without wheels keeps the top");
    near(van[0][1], -0.24375 + 1.0, 1e-5, "a body without wheels raises the bottom by 1 (0x243398)");
    VehicleCollisionBody flags;
    FloorContact steep;
    steep.normal = {1.0f, 0.5f, 0.0f};
    expect(floorContactFlags(flags, 1, steep) == 4, "0x181e68: surface 1 and a steep normal drop the vertical impulse");
    expect(floorContactFlags(flags, 0, steep) == 0, "0x181e68: other surfaces keep it");
    steep.normal = {0.0f, 0.8f, 0.6f};
    expect(floorContactFlags(flags, 0xf, steep) == 0, "0x181e68: a normal flatter than 45 degrees keeps it");
}

// drop.mem (the same poke with 19.0): from update 1 the trooper falls 1.9 m onto its wheels, settles and sleeps in update 40 (rigid +0xdc
// 0.7333 in update 39, 0.7667 and mode 2 in update 40); heights of update 12 (16.85740, lowest), 39 (17.0696068) and 40 (17.0701981).
void overheadLayerAtTeleport(const assets::CollisionItem* floorItem, const WheelRay& ray, const VehicleRecord& record);
void settleAndSleep(const assets::CollisionItem* floorItem, const WheelRay& ray, const VehicleRecord& record) {
    VehicleFloor floor{FloorMesh(*floorItem), FloorTracker{}, trooper::collisionBody(record)};
    VehicleRigidBody body;
    body.orientation = orientationFromAxes({-0.709558964f, 0.000611679628f, 0.704645634f}, {-0.000296625658f, 0.999999285f, -0.00116676034f},
                                           {-0.704645872f, -0.00103690138f, -0.709558249f});
    body.position = {887.377625f, 18.9566669f, 1117.31873f};
    body.velocity = {0.0f, -1.30000007f, 0.0f};
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    DrivetrainState drive = vehicleDrivetrain(record);
    bool awake = true;
    int sleptAt = 0;
    float lowest = body.position[1];
    for (int update = 2; update <= 45 && !sleptAt; ++update) {
        awake = stepVehicleWorld(body, drive, awake, record, BodyInputs{}, ray, 1, &floor);
        lowest = std::min(lowest, body.position[1]);
        if (update == 39) near(body.position[1], 17.0696068, 2e-4, "height before the body sleeps (update 39)");
        if (!awake) sleptAt = update;
    }
    std::printf("drop of 1.9 m: lowest %.5f, asleep in update %d at height %.5f\n", lowest, sleptAt, body.position[1]);
    near(lowest, 16.85740, 2e-4, "lowest point of the bounce (update 12)");
    near(body.position[1], 17.0701981, 2e-4, "height of the sleeping body (update 40)");
    expect(sleptAt == 40, "0x30cdf8: the body sleeps after 0.75 s below 0.8 m/s and 1 rad/s (update 40)", sleptAt);
    expect(body.velocity == Vector3{0, 0, 0} && body.angularVelocity == Vector3{0, 0, 0}, "a sleeping body loses its velocities");
}

// Oracle, 2026-10-16 (agent r14-a, state drv-police-car-seated, A held, rigid +0x34 of the seated trooper poked to (949, 17.8, 1122.7) and its
// body+0x8c set to -1): the tracker 0x1edaac finds cell 12167, which lies about 69 above the street (an overhead layer of the floor graph), and
// the trooper settles to 17.2..17.8 within 4 s instead of being pushed up: 0x1c54f8 drops the push-out of a point far below a cell with flag bit 0
// when 0x164328 finds no wall cell.
void overheadLayerAtTeleport(const assets::CollisionItem* floorItem, const WheelRay& ray, const VehicleRecord& record) {
    VehicleFloor floor{FloorMesh(*floorItem), FloorTracker{}, trooper::collisionBody(record)};
    VehicleRigidBody body;
    body.orientation = orientationFromAxes({-0.709558964f, 0.000611679628f, 0.704645634f}, {-0.000296625658f, 0.999999285f, -0.00116676034f},
                                           {-0.704645872f, -0.00103690138f, -0.709558249f});
    body.position = {949.0f, 17.8f, 1122.7f};
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    trackFloorCell(floor.mesh, floor.tracker, body.position);
    expect(floor.tracker.cell == 12167, "0x1edaac finds the oracle's cell 12167 at the teleport spot", floor.tracker.cell);
    DrivetrainState drive = vehicleDrivetrain(record);
    float highest = body.position[1];
    for (int step = 0; step < 120; ++step) {
        stepVehiclePhysics(body, drive, record, BodyInputs{}, false, ray, kVehicleWorldStep, &floor);
        highest = std::max(highest, body.position[1]);
    }
    expect(highest < 19.0f, "no push-out from the overhead layer", highest);
    expect(body.position[1] > 16.9f && body.position[1] < 17.6f, "the trooper rests on the street", body.position[1]);
}

int real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl");
    if (!entry) {
        std::printf("FAIL: level model not in archive\n");
        return 1;
    }
    const assets::Model model = assets::parseModel(archive.read(*entry));
    CollisionWorld world;
    const assets::CollisionItem* floorItem = nullptr;
    for (const auto& item : model.collision) {
        world.add(item);
        if (item.floor) floorItem = &item;
    }
    if (!floorItem) {
        std::printf("FAIL: no floor graph\n");
        return 1;
    }
    const WheelRay ray = [&world](const Vector3& from, const Vector3& to) { return castWheelRay(world, from, to); };
    const VehicleRecord record = trooper::record();
    VehicleFloor floor{FloorMesh(*floorItem), FloorTracker{}, trooper::collisionBody(record)};

    // drop3.mem update 19: the trooper falls at 24.63 m/s, 0.53 below its rest height; the oracle tracker cell is 22421 throughout.
    VehicleRigidBody body;
    body.orientation = orientationFromAxes({-0.709558964f, 0.000611679628f, 0.704645634f}, {-0.000296625658f, 0.999999285f, -0.00116676034f},
                                           {-0.704645872f, -0.00103690138f, -0.709558249f});
    body.position = {887.377625f, 16.7771549f, 1117.31873f};
    body.velocity = {0.0f, -24.6264267f, 0.0f};
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    trackFloorCell(floor.mesh, floor.tracker, body.position);
    expect(floor.tracker.cell == 22421, "0x1edaac finds the oracle's cell under the trooper", floor.tracker.cell);
    DrivetrainState drive = vehicleDrivetrain(record);
    stepVehiclePhysics(body, drive, record, BodyInputs{}, false, ray, kVehicleWorldStep, &floor);
    std::printf("landing: position %.5f %.5f %.5f velocity %.4f %.4f %.4f angular %.4f %.4f %.4f\n", body.position[0], body.position[1], body.position[2],
                body.velocity[0], body.velocity[1], body.velocity[2], body.angularVelocity[0], body.angularVelocity[1], body.angularVelocity[2]);
    // Update 20: position (887.377563, 16.6059799, 1117.31897), velocity (0.0058, -1.0654, -0.0020), angular velocity 0 (six passes used up).
    const Vector3 position = {887.377563f, 16.6059799f, 1117.31897f};
    const Vector3 velocity = {0.00576185854f, -1.06543541f, -0.00197099755f};
    const Vector3 up = {0.000527024211f, 0.999996185f, -0.00270688487f};
    for (std::size_t a = 0; a < 3; ++a) {
        near(body.position[a], position[a], 2e-4, "position after the collision passes");
        near(body.velocity[a], velocity[a], 1e-3, "velocity after the landing impulses");
        near(bodyAxes(body)[1][a], up[a], 2e-5, "up axis after the landing");
    }
    for (std::size_t a = 0; a < 3; ++a) near(body.angularVelocity[a], 0.0, 0.0, "angular velocity cleared after six passes");
    settleAndSleep(floorItem, ray, record);
    overheadLayerAtTeleport(floorItem, ray, record);
    return 0;
}

}  // namespace

// Manual reading of 0x2384dc / 0x158cb8 (REA assembly of 2026-10-16): the axes drop the dominant normal component; the point test is the three side tests.
void projectionTests() {
    expect(floorProjectionAxes({0.0f, 1.0f, 0.0f}) == std::array<int, 2>{2, 0}, "upward normal projects to (z, x)");
    expect(floorProjectionAxes({0.0f, -1.0f, 0.0f}) == std::array<int, 2>{0, 2}, "downward normal projects to (x, z)");
    expect(floorProjectionAxes({1.0f, 0.0f, 0.0f}) == std::array<int, 2>{1, 2}, "+x wall projects to (y, z)");
    expect(floorProjectionAxes({-1.0f, 0.0f, 0.0f}) == std::array<int, 2>{2, 1}, "-x wall projects to (z, y)");
    expect(floorProjectionAxes({0.0f, 0.0f, 1.0f}) == std::array<int, 2>{0, 1}, "+z wall projects to (x, y)");
    expect(floorProjectionAxes({0.0f, 0.0f, -1.0f}) == std::array<int, 2>{1, 0}, "-z wall projects to (y, x)");
    const std::array<int, 2> axes = {0, 1};
    const FloorPoint a = {0, 0, 0}, b = {2, 0, 0}, c = {0, 2, 0};
    expect(floorProjectedContains(a, b, c, {0.5f, 0.5f, 7.0f}, axes), "a point inside the projected triangle");
    expect(!floorProjectedContains(a, b, c, {1.5f, 1.5f, 0.0f}, axes), "a point beyond the far edge");
    expect(!floorProjectedContains(a, b, c, {-0.5f, 0.5f, 0.0f}, axes), "a point outside the first edge");
}

// 0x155888 by hand from the assembly of 2026-10-16: one floor triangle (0,0,0) (0,0,4) (4,0,0) with the normal up. A ray from (1, 1, 1) straight down
// for 2 crosses the plane after half of it; a start behind the plane misses; a ray that ends above the plane inside the cell misses.
void floorRayTests() {
    assets::CollisionItem item;
    item.vertices = {0, 0, 0, 0, 0, 4, 4, 0, 0};
    assets::CollisionTriangle triangle;
    triangle.normal[1] = 1.0f;
    triangle.vertex[0] = 0;
    triangle.vertex[1] = 1;
    triangle.vertex[2] = 2;
    item.triangles.push_back(triangle);
    item.floor.emplace();
    assets::FloorCell cell;
    cell.itemTriangle = true;
    cell.neighbour[0] = cell.neighbour[1] = cell.neighbour[2] = 0xffff;
    item.floor->cells.push_back(cell);
    const FloorMesh mesh(item);
    const std::optional<FloorRayHit> hit = floorRay(mesh, 0, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, 2.0f);
    expect(hit.has_value(), "the ray down through the cell hits the floor");
    if (hit) {
        near(hit->fraction, 0.5, 1e-7, "floor ray: the plane is crossed after half of the ray");
        near(hit->point[1], 0.0, 1e-7, "floor ray: hit height");
        near(hit->point[0], 1.0, 1e-7, "floor ray: hit x");
    }
    expect(!floorRay(mesh, 0, {1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, 2.0f), "a start behind the plane misses");
    expect(!floorRay(mesh, 0, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, 0.5f), "a ray that ends above the plane inside the cell misses");
    expect(!floorRay(mesh, 0, {3.0f, 1.0f, 3.0f}, {0.0f, -1.0f, 0.0f}, 2.0f), "a ray crossing the plane beyond the leaving edge goes on to the neighbour, none here");
}

int main(int argc, char** argv) {
    probePoints();
    projectionTests();
    floorRayTests();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("vehicle-collision-test: data not available, skipped\n");
            return 77;
        }
        if (real(argv[1], argv[2]) != 0) return 1;
    }
    if (failures != 0) return 1;
    std::printf("vehicle collision tests passed\n");
    return 0;
}

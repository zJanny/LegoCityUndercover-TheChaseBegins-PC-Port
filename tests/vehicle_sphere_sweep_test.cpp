// Tests of the sphere sweep of the vehicle body's floor pass (src/runtime/vehicle_sphere_sweep.*; 0x33a4fc, 0x1b6ef8, 0x16788c with 0x1dabac,
// 0x204ac4, 0x1be3f0, 0x20f6ec, 0x1946a4). Recorded values: the sphere count of the trooper (13) and the donut van (7) is the word at body+0x17c in
// agentVPHYS-out/body0.mem and vbody0.mem (file offset 0x184; Azahar GDB memtrace of 2026-10-15, bodies 0x08e16100 and 0x08e12880). The synthetic cases
// are worked by hand from the assembly of 2026-10-16 (llvm-objdump of artifacts/game/game.arm.elf). The real part replays runA.mem (agentVEH40-out,
// A held from drv-police-car-seated, rigid body per update) from update 43 to 64 on lvl_01: the trooper passes 3 to 8 wall cells of the floor graph
// within the sweep's reach and, as in the oracle, takes no contact from them. Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/vehicle_box_contact.hpp"
#include "runtime/vehicle_sphere_sweep.hpp"
#include "runtime/vehicle_wheels.hpp"
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

struct Synthetic {
    assets::CollisionItem item;

    int addVertex(float x, float y, float z) {
        item.vertices.insert(item.vertices.end(), {x, y, z});
        return int(item.vertices.size() / 3) - 1;
    }
    int addCell(const FloorPoint& a, const FloorPoint& b, const FloorPoint& c, const std::array<float, 3>& normal, float d, std::uint16_t flags,
                const std::array<std::uint16_t, 3>& neighbours) {
        assets::CollisionTriangle triangle;
        triangle.normal[0] = normal[0];
        triangle.normal[1] = normal[1];
        triangle.normal[2] = normal[2];
        triangle.d = d;
        triangle.vertex[0] = std::uint32_t(addVertex(a[0], a[1], a[2]));
        triangle.vertex[1] = std::uint32_t(addVertex(b[0], b[1], b[2]));
        triangle.vertex[2] = std::uint32_t(addVertex(c[0], c[1], c[2]));
        item.triangles.push_back(triangle);
        if (!item.floor) item.floor.emplace();
        assets::FloorCell cell;
        cell.itemTriangle = true;
        cell.triangle = std::uint32_t(item.triangles.size() - 1);
        cell.flags = flags;
        cell.index = std::uint16_t(item.floor->cells.size());
        for (std::size_t k = 0; k < 3; ++k) cell.neighbour[k] = neighbours[k];
        item.floor->cells.push_back(cell);
        return int(cell.index);
    }
};

constexpr std::uint16_t kNone = 0xffff;

void sphereGrid() {
    Vector3 low, high;
    vehicleCollisionBoxStandIn("vehicles/vehicle_police_trooper", low, high);
    const VehicleCollisionSpheres trooper = buildVehicleSpheres(low, high, {2, 1, 6}, 0.6f, false);
    expect(trooper.centres.size() == 13, "trooper: 2 x 1 x 6 spheres and the extra one are the recorded 13 (body+0x17c)", double(trooper.centres.size()));
    expect(trooper.radii.size() == 13, "one radius per sphere");
    near(trooper.centres[0][0], low[0] + 0.6f, 1e-6, "first sphere touches the left side of the box");
    near(trooper.centres[0][1], low[1] + 0.6f, 1e-6, "one row: spheres sit one radius above the box bottom");
    near(trooper.centres[0][2], high[2] - 0.6f, 1e-6, "the grid starts one radius inside the box front");
    near(trooper.centres[1][0], high[0] - 0.6f, 1e-5, "second column touches the right side");
    near(trooper.centres[10][2], low[2] + 0.6f, 1e-5, "sixth row touches the box back");
    expect(trooper.centres[12] == Vector3{0.0f, 0.18f, 0.0f} && trooper.radii[12] == 0.2f, "the last sphere sits at (0, 0.18, 0) with radius 0.2");
    near(trooper.reach, std::sqrt(high[0] * high[0] + high[1] * high[1] + high[2] * high[2]), 1e-5, "reach is the larger corner length (rigid +0x208)");
    Vector3 vanLow, vanHigh;
    vehicleCollisionBoxStandIn("vehicles/vehicle_van_donut", vanLow, vanHigh);
    expect(buildVehicleSpheres(vanLow, vanHigh, {2, 1, 3}, 0.9f, false).centres.size() == 7, "donut van: 2 x 1 x 3 and the extra sphere are the recorded 7");
    const VehicleCollisionSpheres extra = buildVehicleSpheres(low, high, {2, 1, 6}, 0.6f, true);
    expect(extra.centres.size() == 15, "ExtraColSpheres1 adds two spheres");
    expect(extra.centres[13] == Vector3{0.75f, 0.1f, 1.3f} && extra.radii[13] == 0.7f && extra.radii[12] == 0.7f, "two front spheres of radius 0.7");
    expect(extra.centres[12] == Vector3{-0.75f, 0.1f, 1.3f} && extra.centres[14] == Vector3{0.0f, 0.18f, 0.0f}, "front left sphere, then the low centre sphere");
}

void wallCells() {
    Synthetic s;
    s.addCell({0, 0, 0}, {0, 0, 4}, {4, 0, 0}, {0, 1, 0}, 0.0f, 0x30, {1, 1, kNone});
    s.addCell({0, 0, 0}, {0, 0, 4}, {0, 2, 0}, {1, 0, 0}, 0.0f, 0, {kNone, kNone, kNone});
    const FloorMesh mesh(s.item);
    const WallEdgeCells found = findWallEdgeCells(mesh, {1.0f, 0.5f, 2.0f}, 3.0f, 0, 16);
    expect(found.cells == std::vector<int>({0, 0}), "a cell with two wall edges is listed once per edge");
    expect(found.edges[0] == 3 && found.edges[1] == 0, "both edges of cell 0 are flagged");
    expect(findWallEdgeCells(mesh, {1.0f, 0.5f, 2.0f}, 0.5f, 0, 16).cells.empty(), "edges farther than the reach are left out");
    expect(findWallEdgeCells(mesh, {1.0f, -10.0f, 2.0f}, 3.0f, 0, 16).cells.empty(), "edges 5 or more above the centre are left out");
    expect(findWallEdgeCells(mesh, {1.0f, 0.5f, 2.0f}, 3.0f, 0, 2).cells.empty(), "the other mask finds no wall bits");
}

void wallTriangle() {
    Synthetic s;
    s.addCell({3, -10, -10}, {3, 0, 20}, {3, 10, -10}, {-1, 0, 0}, -3.0f, 0, {kNone, kNone, kNone});
    s.addCell({3, -10, -10}, {3, 0, -1}, {3, 10, -10}, {-1, 0, 0}, -3.0f, 0, {kNone, kNone, kNone});
    const FloorMesh mesh(s.item);

    FloorContacts crossing;
    expect(sweepSphereAgainstCell(mesh, 0, 0.5f, 0.3f, {0.0f, 1.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, crossing), "a sphere driven into a wall touches it");
    expect(crossing.items.size() == 1, "one contact", double(crossing.items.size()));
    if (crossing.items.size() == 1) {
        near(crossing.items[0].time, 2.5 / 3.0, 1e-6, "contact time: the sphere's nearest point reaches the plane after 2.5 of 3 m");
        near(crossing.items[0].point[0], 3.0, 1e-5, "contact point on the plane");
        near(crossing.items[0].normal[0], -1.0, 0.0, "contact normal is the plane normal");
        near(crossing.earliest, 2.5 / 3.0, 1e-6, "the list's earliest time");
    }

    FloorContacts away;
    expect(!sweepSphereAgainstCell(mesh, 0, 0.5f, 0.3f, {0.0f, 1.0f, 0.0f}, {-10.0f, 0.0f, 0.0f}, away), "a sphere moving away from the wall has no contact");
    FloorContacts short_;
    expect(!sweepSphereAgainstCell(mesh, 0, 0.5f, 0.1f, {0.0f, 1.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, short_), "a sphere that stops short has no contact");

    FloorContacts touching;
    expect(sweepSphereAgainstCell(mesh, 0, 0.5f, 0.0333f, {2.8f, 1.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, touching), "a sphere inside the wall's reach");
    if (touching.items.size() == 1) {
        near(touching.items[0].time, 0.0, 0.0, "overlap: contact at time 0");
        near(touching.items[0].depth, 0.1, 1e-7, "overlap: the depth is capped at 0.1");
        near(touching.items[0].normal[0], -1.0, 1e-6, "overlap: normal from the closest point to the centre");
        near(touching.items[0].point[0], 3.0, 1e-5, "overlap: closest point on the plane");
    } else {
        expect(false, "overlap gives one contact", double(touching.items.size()));
    }
    FloorContacts laterTime;
    laterTime.earliest = 0.5f;
    laterTime.items.push_back({});
    laterTime.items[0].time = 0.5f;
    expect(!sweepSphereAgainstCell(mesh, 0, 0.5f, 0.3f, {0.0f, 1.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, laterTime), "a crossing later than the earliest contact is dropped");
    expect(laterTime.items.size() == 1 && laterTime.items[0].time == 0.5f, "the earlier contact stays alone", double(laterTime.items.size()));

    FloorContacts edge;
    expect(sweepSphereAgainstCell(mesh, 1, 0.5f, 0.3f, {0.0f, 0.0f, -0.7f}, {10.0f, 0.0f, 0.0f}, edge), "a sphere passing a wall's apex touches the apex");
    if (edge.items.size() == 1) {
        near(edge.items[0].time, 2.6 / 3.0, 1e-5, "edge: a ray from the apex against the sphere hits after 2.6 of 3 m");
        near(edge.items[0].point[0], 3.0, 1e-6, "edge: contact point x (the apex)");
        near(edge.items[0].point[2], -1.0, 1e-6, "edge: contact point z (the apex)");
        near(edge.items[0].normal[0], -1.0, 0.0, "edge: the plane normal");
    } else {
        expect(false, "edge gives one contact", double(edge.items.size()));
    }
}

std::array<float, 4> orientationFromBasis(const std::array<Vector3, 3>& m) {
    const float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return {(m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s, 0.25f * s};
    }
    if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        const float s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        return {0.25f * s, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s, (m[2][1] - m[1][2]) / s};
    }
    if (m[1][1] > m[2][2]) {
        const float s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        return {(m[0][1] + m[1][0]) / s, 0.25f * s, (m[1][2] + m[2][1]) / s, (m[0][2] - m[2][0]) / s};
    }
    const float s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
    return {(m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, 0.25f * s, (m[1][0] - m[0][1]) / s};
}

// runA.mem update 43 (rigid +0x04, +0x14, +0x24, +0x34, +0x13c, +0x14c) and the positions of updates 55, 60 and 64.
const std::array<Vector3, 3> kRun43Basis = {Vector3{-0x1.6b1d18p-1f, 0x1.20fcf4p-7f, -0x1.68eeaap-1f}, Vector3{0x1.067ca8p-10f, 0x1.fff744p-1f, 0x1.78e83ep-7f},
                                            Vector3{0x1.68f5cep-1f, 0x1.e859a4p-8f, -0x1.6b180cp-1f}};
const Vector3 kRun43Origin = {0x1.b973b8p+9f, 0x1.10d99cp+4f, 0x1.163412p+10f};
const Vector3 kRun43Velocity = {-0x1.0efc5cp+3f, -0x1.01b908p-6f, -0x1.10b15p+3f};
const Vector3 kRun43AngularVelocity = {0x1.84b8a6p-6f, -0x1.41cc04p-14f, -0x1.89c902p-6f};
const Vector3 kRun55 = {0x1.b76447p+9f, 0x1.10f1e2p+4f, 0x1.152abep+10f};
const Vector3 kRun60 = {0x1.b658eap+9f, 0x1.10e118p+4f, 0x1.14a434p+10f};
const Vector3 kRun64 = {0x1.b570f2p+9f, 0x1.10c45ap+4f, 0x1.142f98p+10f};

int real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl");
    if (!entry) {
        std::printf("FAIL: level model not in archive\n");
        return 1;
    }
    const assets::Model model = assets::parseModel(archive.read(*entry));
    const std::vector<BulletBox> boxes = levelVehicleBoxes(model);
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
    VehicleRecord record = trooper::record();
    record.collSpheres = std::array<float, 3>{2.0f, 1.0f, 6.0f};
    record.collRadius = 0.6f;
    VehicleBoxContacts contacts = vehicleBoxContacts(record.className, &boxes);
    VehicleFloor floor{FloorMesh(*floorItem), FloorTracker{}, vehicleCollisionBody(record), &contacts};
    expect(floor.shape.sphereSet.centres.size() == 13, "the trooper's class values give 13 spheres", double(floor.shape.sphereSet.centres.size()));
    VehicleRigidBody body;
    body.orientation = orientationFromBasis(kRun43Basis);
    body.position = kRun43Origin;
    body.velocity = kRun43Velocity;
    body.angularVelocity = kRun43AngularVelocity;
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    BodyInputs held;
    held.accelerate = 1.0f;
    // Test stand-in as in vehicle-box-contact: the drivetrain state of runA is not traced; it is the port's own after A held from rest on a flat
    // plane until the forward speed reaches the oracle's.
    DrivetrainState drive = vehicleDrivetrain(record);
    {
        VehicleRigidBody plane = vehicleRigidBody(record, {0.0f, 0.95f, 0.0f}, 0.0f);
        const auto speed = [](const VehicleRigidBody& b) { return std::sqrt(b.velocity[0] * b.velocity[0] + b.velocity[2] * b.velocity[2]); };
        for (int i = 0; i < 400 && speed(plane) < speed(body); ++i)
            stepVehiclePhysics(plane, drive, record, held, false, trooper::flatGround, kVehicleWorldStep);
    }
    bool awake = true;
    std::size_t mostWalls = 0;
    for (int update = 44; update <= 64; ++update) {
        awake = stepVehicleWorld(body, drive, awake, record, held, ray, 1, &floor);
        if (floor.tracker.cell >= 0)
            mostWalls = std::max(mostWalls, findWallEdgeCells(floor.mesh, body.position, floor.shape.sphereSet.reach + 0.5f, floor.tracker.cell, 16).cells.size());
        const Vector3* want = update == 55 ? &kRun55 : update == 60 ? &kRun60 : update == 64 ? &kRun64 : nullptr;
        if (!want) continue;
        for (std::size_t a = 0; a < 3; ++a) near(body.position[a], (*want)[a], 0.01, "the trooper follows runA past the wall cells");
    }
    expect(mostWalls >= 3, "the replay passes wall cells within the sweep's reach", double(mostWalls));
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    sphereGrid();
    wallCells();
    wallTriangle();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("vehicle-sphere-sweep-test: data not available, skipped\n");
            return 77;
        }
        if (real(argv[1], argv[2]) != 0) return 1;
    }
    if (failures != 0) return 1;
    std::printf("vehicle sphere sweep tests passed\n");
    return 0;
}

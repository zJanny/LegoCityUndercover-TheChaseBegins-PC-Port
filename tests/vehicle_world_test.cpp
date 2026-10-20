// Tests of the vehicle world step with several bodies (src/runtime/vehicle_world.*, the two-body solve and the vehicle pair contacts of
// src/runtime/vehicle_box_contact.*; docs/re/vehicles-physics-collision.md). Pinned values: Azahar GDB reads and memory traces of 2026-10-15
// (agent VEH7, not committed). State veh7-poked-trooper-behind-parked (drv-police-car-seated with the trooper
// moved 10 m behind the parked FP_Vehicle_03 and turned to its heading by GDB writes of the rigid transform, a measuring aid), fixed dt 0.033,
// A held: hitP4.mem, per update the trooper's rigid body 0x08e16f40 and the worker's 0x08e90b50 (0x240 bytes each), the dispatcher and the
// pair manifold 0x08472cb0. The trooper hits the parked worker in update 27. world.txt: the shapes and the vehicle bodies (+0x84, +0xb0, +0xc8).
// Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/vehicle_drive.hpp"
#include "runtime/vehicle_world.hpp"
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

// Two equal bodies meeting head-on along x: the two-body rows push them apart with equal and opposite impulses and wake the sleeping one.
void twoBodySolve() {
    VehicleRigidBody a, b;
    a.inverseMass = b.inverseMass = 1.0f / 1000.0f;
    a.inverseInertia = b.inverseInertia = {1e-3f, 1e-3f, 1e-3f};
    a.position = {0, 0, 0};
    b.position = {2, 0, 0};
    a.velocity = {3, 0, 0};
    ContactManifold manifold;
    manifold.breakingThreshold = 0.06f;
    ManifoldPoint point;
    point.normalOnB = {1, 0, 0};
    point.worldB = {1.0f, 0, 0};
    point.worldA = {0.99f, 0, 0};
    point.distance = -0.01f;
    point.friction = 0.25f;
    manifold.points.push_back(point);
    BulletSolverInfo info;
    info.timeStep = kVehicleWorldStep;
    const std::vector<bool> woke = solveContactGroup({&b, &a}, {{&manifold, 0, 1}}, info);
    expect(woke[0] && woke[1], "a contact within the processing threshold activates both bodies");
    near(a.velocity[0] + b.velocity[0], 3.0, 1e-5, "momentum is kept");
    expect(b.velocity[0] > a.velocity[0], "the bodies separate");
    near(a.velocity[1], 0.0, 1e-6, "no sideways velocity");
}

// The inertia of 0x33ee44 times InertiaScale (1, 2.5, 2) and the mass scale: trooper body+0xb0 (3606.363, 9295.157, 1406.109) for Mass 1300 and
// the entity box half (0.96959, 0.82604, 2.76406); the worker's (11150.493, 26005.002, 5240.407) for Mass 3000 and its box; the rigid inverse
// mass 1/2600 (trooper, scale 2) and 1/3000 (worker, scale 1).
void inertia() {
    const std::optional<std::array<float, 3>> scale = std::array<float, 3>{1.0f, 2.5f, 2.0f};
    const Vector3 trooper = vehicleLocalInertia(1300.0f, {-0.96959f, 0.00406f, -2.85938f}, {0.96959f, 1.65614f, 2.66875f}, scale);
    near(trooper[0], 3606.363, 0.05, "trooper inertia x");
    near(trooper[1], 9295.157, 0.05, "trooper inertia y");
    near(trooper[2], 1406.109, 0.05, "trooper inertia z");
}

struct Pose {
    std::array<Vector3, 3> basis;
    Vector3 origin, velocity, angularVelocity;
};
// hitP4.mem update 26 (rigid +0x04 basis rows, +0x34, +0x13c, +0x14c): the trooper one update before the hit and the sleeping worker.
const Pose kTrooper26 = {{Vector3{-0x1.682cf8p-1f, -0x1.7c5586p-10f, 0x1.6be436p-1f}, Vector3{0x1.dbe318p-9f, 0x1.fffd0ep-1f, 0x1.714b68p-8f},
                          Vector3{-0x1.6be332p-1f, 0x1.ace64p-8f, -0x1.682a34p-1f}},
                         {0x1.d5909p+9f, 0x1.14150ep+4f, 0x1.184f1ap+10f},
                         {0x1.9d9cc6p+2f, -0x1.740e84p-4f, -0x1.98c084p+2f},
                         {0x1.7ec03p-6f, 0x1.81585ap-11f, 0x1.6a9128p-6f}};
const Pose kWorker26 = {{Vector3{-0x1.683e24p-1f, 0x1.489738p-15f, 0x1.6bd364p-1f}, Vector3{-0x1.16008ep-14f, 0x1p+0f, -0x1.fa78c8p-14f},
                         Vector3{-0x1.6bd364p-1f, -0x1.14f348p-13f, -0x1.683e24p-1f}},
                        {0x1.d7c04cp+9f, 0x1.161496p+4f, 0x1.173af8p+10f},
                        {0, 0, 0},
                        {0, 0, 0}};
// The worker's rigid +0x190 (inverse inertia at scale 1) and +0xdc (the sleep time it keeps while parked).
const Vector3 kWorkerInverseInertia = {0x1.782774p-14f, 0x1.4293a4p-15f, 0x1.903052p-13f};
// Updates 27 (the hit: the solver throws the trooper back and pushes the worker, which wakes), 28, 30, 35, 40 (rigid +0x34, +0x13c).
struct Sample {
    int update;
    Vector3 trooperPosition, trooperVelocity, workerPosition, workerVelocity;
};
const Sample kSamples[] = {
    {27, {939.3548f, 17.2522f, 1121.0132f}, {2.6823f, -0.0620f, -2.6525f}, {943.5023f, 17.3800f, 1116.9214f}, {3.5358f, -0.0238f, -3.4935f}},
    {28, {939.4855f, 17.2492f, 1120.8840f}, {2.6644f, -0.0732f, -2.6726f}, {943.5891f, 17.3792f, 1116.8356f}, {3.6918f, -0.0367f, -3.6140f}},
    {30, {939.7383f, 17.2401f, 1120.6343f}, {2.8289f, -0.0597f, -2.7914f}, {943.7922f, 17.3735f, 1116.6350f}, {3.9628f, -0.1749f, -3.9163f}},
    {35, {940.3714f, 17.2527f, 1120.0112f}, {3.0385f, 0.2361f, -2.9564f}, {944.3769f, 17.3656f, 1116.0563f}, {4.4645f, -0.0001f, -4.4408f}},
    {40, {941.0385f, 17.2532f, 1119.3604f}, {3.1469f, -0.0697f, -3.0228f}, {945.0061f, 17.3825f, 1115.4290f}, {4.6637f, 0.0472f, -4.6747f}},
};

// hitP4.mem update 27, the hit: both rigid bodies (+0x04, +0x34, +0x13c, +0x14c and the solver deltas +0x20c, +0x21c) and the pair manifold
// 0x08472cb0 (body 0 the worker 0x08e90b50, body 1 the trooper 0x08e16f40; per point +0x20 world B, +0x30 world A, +0x40 normal, +0x50
// distance, +0x70 impulse; friction 0.25, life time 1).
struct SolvedBody {
    Pose pose;
    Vector3 deltaLinear, deltaAngular;
};
const SolvedBody kTrooper27 = {{{Vector3{-0x1.682fc8p-1f, -0x1.200314p-9f, 0x1.6be12ap-1f}, Vector3{0x1.ded3a4p-9f, 0x1.fffc2ep-1f, 0x1.b79a4ap-8f},
                                 Vector3{-0x1.6be062p-1f, 0x1.df6852p-8f, -0x1.682c0cp-1f}},
                                {0x1.d5ad6ap+9f, 0x1.1408dap+4f, 0x1.1840d8p+10f},
                                {0x1.575448p+1f, -0x1.fbb508p-5f, -0x1.538468p+1f},
                                {0x1.85c4f2p-4f, 0x1.55d73p-7f, 0x1.92e18p-4f}},
                               {-0x1.051adep+2f, 0x1.c0ffc2p-6f, 0x1.01fbb4p+2f},
                               {0x1.2711f6p-4f, 0x1.3bd36p-7f, 0x1.372052p-4f}};
const SolvedBody kWorker27 = {{{Vector3{-0x1.683e24p-1f, 0x1.489738p-15f, 0x1.6bd364p-1f}, Vector3{-0x1.16008ep-14f, 0x1p+0f, -0x1.fa78c8p-14f},
                                Vector3{-0x1.6bd364p-1f, -0x1.14f348p-13f, -0x1.683e24p-1f}},
                               {0x1.d7c04cp+9f, 0x1.161496p+4f, 0x1.173af8p+10f},
                               {0x1.c494eep+1f, -0x1.852208p-6f, -0x1.bf2bc4p+1f},
                               {-0x1.106f52p-4f, -0x1.f1edacp-5f, -0x1.230f22p-4f}},
                              {0x1.c494eep+1f, -0x1.852208p-6f, -0x1.bf2bc4p+1f},
                              {-0x1.106f52p-4f, -0x1.f1edacp-5f, -0x1.230f22p-4f}};
struct PairPoint {
    Vector3 worldB, worldA, normal;
    float distance, impulse;
};
const PairPoint kPair27[] = {
    {{0x1.d648f8p+9f, 0x1.2edfe8p+4f, 0x1.179da6p+10f}, {0x1.d63824p+9f, 0x1.2edad2p+4f, 0x1.17a5fap+10f}, {0x1.6be12ap-1f, 0x1.b79a4ap-8f, -0x1.682c0cp-1f}, -0x1.7ad894p-3f, 0.0f},
    {{0x1.d649ep+9f, 0x1.17852cp+4f, 0x1.179d36p+10f}, {0x1.d63822p+9f, 0x1.177fdp+4f, 0x1.17a5fep+10f}, {0x1.6be12ap-1f, 0x1.b79a4ap-8f, -0x1.682c0cp-1f}, -0x1.8f70eep-3f, 0x1.029968p+13f},
    {{0x1.d6f77p+9f, 0x1.1785bp+4f, 0x1.17f4e4p+10f}, {0x1.d6e5bap+9f, 0x1.178056p+4f, 0x1.17fda8p+10f}, {0x1.6be12ap-1f, 0x1.b79a4ap-8f, -0x1.682c0cp-1f}, -0x1.8ebbe8p-3f, 0x1.9ebb66p+12f},
    {{0x1.d6f708p+9f, 0x1.2ec2fcp+4f, 0x1.17f59p+10f}, {0x1.d6e63ap+9f, 0x1.2ebde8p+4f, 0x1.17fde2p+10f}, {0x1.6be12ap-1f, 0x1.b79a4ap-8f, -0x1.682c0cp-1f}, -0x1.7a6752p-3f, 0.0f},
};

VehicleRigidBody bodyAt(const Pose& pose) {
    VehicleRigidBody body;
    body.orientation = orientationFromBasis(pose.basis);
    body.position = pose.origin;
    body.velocity = pose.velocity;
    body.angularVelocity = pose.angularVelocity;
    return body;
}

VehicleRigidBody solvedBody(const SolvedBody& solved, float inverseMass, const Vector3& inverseInertia) {
    VehicleRigidBody body = bodyAt(solved.pose);
    for (std::size_t a = 0; a < 3; ++a) {
        body.velocity[a] -= solved.deltaLinear[a];
        body.angularVelocity[a] -= solved.deltaAngular[a];
    }
    body.inverseMass = inverseMass;
    body.inverseInertia = inverseInertia;
    return body;
}

// The rigid +0x190 of the trooper (scale 2) in drv-police-car-seated.
const Vector3 kTrooperInverseInertia = {0x1.22c1d8p-13f, 0x1.c33c44p-15f, 0x1.74dd5p-12f};

// The two-body solve of update 27 from the traced manifold and the velocities before the solver.
void pairSolve() {
    VehicleRigidBody trooperBody = solvedBody(kTrooper27, 1.0f / 2600.0f, kTrooperInverseInertia);
    VehicleRigidBody workerBody = solvedBody(kWorker27, 1.0f / 3000.0f, kWorkerInverseInertia);
    ContactManifold manifold;
    manifold.breakingThreshold = 0x1.f2a2c6p-5f;
    for (const PairPoint& o : kPair27) {
        ManifoldPoint p;
        p.worldB = o.worldB;
        p.worldA = o.worldA;
        p.normalOnB = o.normal;
        p.distance = o.distance;
        p.friction = 0.25f;
        p.lifeTime = 1;
        p.flags = 2;
        manifold.points.push_back(p);
    }
    BulletSolverInfo info;
    info.timeStep = kVehicleWorldStep;
    const std::vector<bool> woke = solveContactGroup({&workerBody, &trooperBody}, {{&manifold, 0, 1}}, info);
    expect(woke[0], "the contact wakes the parked worker");
    std::printf("pair solve: trooper %.5f %.5f %.5f / %.5f %.5f %.5f worker %.5f %.5f %.5f / %.5f %.5f %.5f impulses %.1f %.1f %.1f %.1f\n",
                trooperBody.velocity[0], trooperBody.velocity[1], trooperBody.velocity[2], trooperBody.angularVelocity[0], trooperBody.angularVelocity[1],
                trooperBody.angularVelocity[2], workerBody.velocity[0], workerBody.velocity[1], workerBody.velocity[2], workerBody.angularVelocity[0],
                workerBody.angularVelocity[1], workerBody.angularVelocity[2], manifold.points[0].appliedImpulse, manifold.points[1].appliedImpulse,
                manifold.points[2].appliedImpulse, manifold.points[3].appliedImpulse);
    for (std::size_t a = 0; a < 3; ++a) {
        near(trooperBody.velocity[a], kTrooper27.pose.velocity[a], 1e-3, "trooper velocity after the pair solve (update 27)");
        near(trooperBody.angularVelocity[a], kTrooper27.pose.angularVelocity[a], 1e-3, "trooper angular velocity after the pair solve");
        near(workerBody.velocity[a], kWorker27.pose.velocity[a], 1e-3, "worker velocity after the pair solve (update 27)");
        near(workerBody.angularVelocity[a], kWorker27.pose.angularVelocity[a], 1e-3, "worker angular velocity after the pair solve");
    }
    for (std::size_t i = 0; i < 4; ++i) near(manifold.points[i].appliedImpulse, kPair27[i].impulse, 1.0, "point impulse kept in the manifold");
}

// The pair contacts of update 27 from the poses: the worker's child box (shape 0x08e8fbc0: offset (0, 1.38617, -0.27701), half extents minus
// margin +0x20) is box 1 of dBoxBox2, the trooper's box 2.
void pairDetect() {
    const VehicleRigidBody trooperBody = bodyAt(kTrooper27.pose);
    const VehicleRigidBody workerBody = bodyAt(kWorker27.pose);
    VehicleBoxContacts trooperShape = trooper::boxContacts(nullptr);
    VehicleBoxContacts workerShape;
    workerShape.childOffset = {0.0f, 0x1.62db86p+0f, -0x1.1ba81cp-2f};
    workerShape.child.transform.basis = {Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}};
    workerShape.child.implicitHalf = {0x1.dad3b4p-1f, 0x1.4200c6p+0f, 0x1.84b2ecp+1f};
    std::optional<ContactManifold> manifold;
    collideVehiclePair(trooperBody, trooperShape, workerBody, workerShape, manifold);
    expect(manifold.has_value() && manifold->points.size() == 4, "four contacts", manifold ? double(manifold->points.size()) : -1.0);
    if (!manifold) return;
    for (const ManifoldPoint& p : manifold->points) {
        std::printf("pair detect: B %.4f %.4f %.4f A %.4f %.4f %.4f n %.4f %.4f %.4f d %.5f life %d\n", p.worldB[0], p.worldB[1], p.worldB[2], p.worldA[0],
                    p.worldA[1], p.worldA[2], p.normalOnB[0], p.normalOnB[1], p.normalOnB[2], p.distance, p.lifeTime);
    }
}

int real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* modelEntry = archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl");
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    const assets::FuseEntry* tableEntry = archive.find("tables/tablescharacters.bxls");
    if (!modelEntry || !levelEntry || !tableEntry) {
        std::printf("FAIL: level data not in archive\n");
        return 1;
    }
    const assets::Model model = assets::parseModel(archive.read(*modelEntry));
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    VehicleSet set;
    set.load(level, classOf, parseVehicleTable(assets::parseTable(archive.read(*tableEntry))));
    const std::vector<assets::Placement> placed = assets::objectPlacements(level, classOf);
    const auto box = [&](const std::string& instance, Vector3& centre, Vector3& half) {
        for (const assets::Placement& p : placed)
            if (p.instance == instance) return assets::modelCollisionBox(archive, assets::meshModelPath(p.mesh), centre.data(), half.data());
        return false;
    };
    const VehicleRecord* police = set.find("1-1_police_car");
    const VehicleRecord* worker = set.find("fp_vehicle_03");
    Vector3 policeCentre, policeHalf, workerCentre, workerHalf;
    if (!police || !worker || !box("1-1_Police_Car", policeCentre, policeHalf) || !box("FP_Vehicle_03", workerCentre, workerHalf)) {
        std::printf("FAIL: vehicles not found\n");
        return 1;
    }
    // world.txt: the worker's compound child at (0, 1.3862, -0.2770) with half extents (0.9674, 1.2978, 3.0767) (box shape +0x20 plus margin).
    Vector3 workerLow, workerHigh, policeLow, policeHigh;
    vehicleShapeBox(workerCentre, workerHalf, worker->centreOfMass, workerLow, workerHigh);
    vehicleShapeBox(policeCentre, policeHalf, police->centreOfMass, policeLow, policeHigh);
    near((workerLow[1] + workerHigh[1]) * 0.5f, 1.3862, 1e-4, "worker child offset y (entity box minus CentreOfMass)");
    near((workerLow[2] + workerHigh[2]) * 0.5f, -0.2770, 1e-4, "worker child offset z");
    near((workerHigh[2] - workerLow[2]) * 0.5f, 3.0767, 1e-4, "worker child half length");
    Vector3 shapeLow, shapeHigh;
    vehicleShapeBox(workerCentre, workerHalf, worker->centreOfMass, shapeLow, shapeHigh);
    const Vector3 workerInertia = vehicleLocalInertia(worker->mass, shapeLow, shapeHigh, worker->inertiaScale);
    near(workerInertia[0], 11150.493, 0.1, "worker inertia x (body+0xb0)");
    near(workerInertia[1], 26005.002, 0.2, "worker inertia y");
    near(workerInertia[2], 5240.407, 0.1, "worker inertia z");

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
    const std::vector<BulletBox> boxes = levelVehicleBoxes(model);
    const WheelRay ray = [&world](const Vector3& from, const Vector3& to) { return castWheelRay(world, from, to); };

    const VehicleRecord trooperRecord = vehicleRecordAtMassScale(*police, 2.0f);
    const VehicleRecord workerRecord = vehicleRecordAtMassScale(*worker, 1.0f);
    VehicleRigidBody trooperBody = bodyAt(kTrooper26);
    Vector3 policeShapeLow, policeShapeHigh;
    vehicleShapeBox(policeCentre, policeHalf, police->centreOfMass, policeShapeLow, policeShapeHigh);
    setVehicleMassScale(trooperBody, *police, policeShapeLow, policeShapeHigh, 2.0f);
    VehicleRigidBody workerBody = bodyAt(kWorker26);
    setVehicleMassScale(workerBody, *worker, shapeLow, shapeHigh, 1.0f);
    near(workerBody.inverseMass, 1.0 / 3000.0, 1e-9, "worker inverse mass at scale 1");
    for (std::size_t a = 0; a < 3; ++a) near(workerBody.inverseInertia[a], kWorkerInverseInertia[a], kWorkerInverseInertia[a] * 2e-5, "worker inverse inertia");
    workerBody.deactivationTime = 0.7666669f;

    BodyInputs held;
    held.accelerate = 1.0f;
    held.driver = true;
    // Test stand-in: the trooper's drivetrain state is not traced; it is the port's own after A held from rest on a flat plane until the
    // forward speed reaches the oracle's (as in vehicle-box-contact).
    DrivetrainState trooperDrive = vehicleDrivetrain(trooperRecord);
    {
        VehicleRigidBody plane = vehicleRigidBody(trooperRecord, {0.0f, 0.95f, 0.0f}, 0.0f);
        for (int i = 0; i < 400 && forwardSpeed(plane) < forwardSpeed(trooperBody); ++i)
            stepVehiclePhysics(plane, trooperDrive, trooperRecord, held, false, trooper::flatGround, kVehicleWorldStep);
    }
    DrivetrainState workerDrive = vehicleDrivetrain(workerRecord);
    VehicleBoxContacts trooperShape = vehicleBoxContacts(policeLow, policeHigh, &boxes);
    VehicleBoxContacts workerShape = vehicleBoxContacts(workerLow, workerHigh, &boxes);
    VehicleFloor trooperFloor{FloorMesh(*floorItem), FloorTracker{}, vehicleCollisionBody(trooperRecord, policeLow, policeHigh, 2.0f), &trooperShape};
    VehicleFloor workerFloor{FloorMesh(*floorItem), FloorTracker{}, vehicleCollisionBody(workerRecord, workerLow, workerHigh, 1.0f), &workerShape};
    bool trooperAwake = true, workerAwake = false;
    std::vector<VehicleWorldBody> bodies(2);
    bodies[0] = {0, &trooperBody, &trooperDrive, &trooperAwake, &trooperRecord, held, &trooperFloor, nullptr};
    int impacts = 0;
    bodies[0].impact = [&impacts](float, const ContactOther& other, const Vector3&) {
        if (other.entity) ++impacts;
    };
    // The parked worker (SpawnParked, state 4) gets the hand brake of 0x23019c and has no driver (body flag +0xc4 = 0x8003, bit 3 clear).
    expect(worker->requestedState == 4, "FP_Vehicle_03 is parked (state 4)", worker->requestedState);
    BodyInputs parked = bodyInputs(commandForState(worker->requestedState, DriveInput{}, false, 0.0f));
    parked.driver = false;
    bodies[1] = {1, &workerBody, &workerDrive, &workerAwake, &workerRecord, parked, &workerFloor, nullptr};
    VehiclePairManifolds pairs;
    int update = 26;
    for (const Sample& sample : kSamples) {
        stepVehicleBodies(bodies, pairs, ray, sample.update - update);
        update = sample.update;
        std::printf("update %d: trooper %.4f %.4f %.4f v %.4f %.4f %.4f | worker %.4f %.4f %.4f v %.4f %.4f %.4f%s\n", update, trooperBody.position[0],
                    trooperBody.position[1], trooperBody.position[2], trooperBody.velocity[0], trooperBody.velocity[1], trooperBody.velocity[2],
                    workerBody.position[0], workerBody.position[1], workerBody.position[2], workerBody.velocity[0], workerBody.velocity[1],
                    workerBody.velocity[2], workerAwake ? " (awake)" : "");
        // Test stand-in: the trooper's drivetrain is the port's own (above), so after the hit the bodies drift from the trace; the hit update is
        // pinned closely, the later ones within 0.2 m.
        const double position = update == 27 ? 0.01 : 0.2;
        const double velocity = update == 27 ? 0.15 : 0.7;
        for (std::size_t a = 0; a < 3; a += 2) {
            near(trooperBody.position[a], sample.trooperPosition[a], position, "trooper position against the trace");
            near(workerBody.position[a], sample.workerPosition[a], position, "worker position against the trace (pushed by the trooper)");
            near(trooperBody.velocity[a], sample.trooperVelocity[a], velocity, "trooper velocity against the trace");
            near(workerBody.velocity[a], sample.workerVelocity[a], velocity, "worker velocity against the trace");
        }
        expect(workerAwake, "the hit wakes the parked worker in the update of the contact");
    }
    expect(impacts > 0, "the pair contacts send the trooper the contact message with the worker as the other entity", impacts);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    twoBodySolve();
    inertia();
    pairSolve();
    pairDetect();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("vehicle-world-test: data not available, skipped\n");
            return 77;
        }
        if (real(argv[1], argv[2]) != 0) return 1;
    }
    if (failures != 0) return 1;
    std::printf("vehicle world tests passed\n");
    return 0;
}

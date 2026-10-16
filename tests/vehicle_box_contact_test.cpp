// Tests of the vehicle body's Bullet contacts with the static boxes (src/runtime/vehicle_box_contact.*, docs/re/vehicles-physics-collision.md).
// Pinned values: Azahar GDB reads and memory traces of 2026-10-15 (agent BULLET, artifacts/oracle/agentBULLET-out, not committed). hitA.mem:
// state bullet-trooper-at-obstacle (the trooper of drv-police-car-seated after the A run, resting against the obstacle), fixed dt 0.033, A
// held; per update the rigid body 0x08e16f40 (0x240 bytes), the dispatcher 0x084aaf00 and its only manifold 0x08472cb0 (trooper against the
// static box 0x08f49810, shape 0x08f49770). runA.mem (agentVEH40-out): the A run from drv-police-car-seated, rigid body per update. The real
// part loads lvl_01 and drives the trooper from runA update 105 into the obstacle. Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/vehicle_box_contact.hpp"
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

struct OraclePoint {
    Vector3 localA, localB, worldB, worldA, normal;
    float distance, impulse;
    int lifeTime;
};
struct OracleFrame {
    std::array<Vector3, 3> basis;
    Vector3 origin, velocity, angularVelocity, deltaLinear, deltaAngular;
    std::array<OraclePoint, 4> points;
};

// hitA.mem updates 107 and 108 (rigid +0x04 basis rows, +0x34, +0x13c, +0x14c, the solver deltas +0x20c and +0x21c, manifold points
// +0x00 local A, +0x10 local B, +0x20 world B, +0x30 world A, +0x40 normal, +0x50 distance, +0x70 impulse, +0x90 life time).
const OracleFrame k107 = {{Vector3{-0x1.4c673cp-1f, 0x1.132a5ep-6f, -0x1.855492p-1f}, Vector3{0x1.706eb4p-9f, 0x1.ffe618p-1f, 0x1.4278fap-6f}, Vector3{0x1.856c32p-1f, 0x1.5cacb4p-7f, -0x1.4c5c9cp-1f}}, {0x1.a7e72cp+9f, 0x1.13fd0cp+4f, 0x1.0d496ap+10f}, {0x1.9bae2ap-2f, 0x1.d1743ep-8f, 0x1.5b505cp-2f}, {-0x1.c6bf1p-10f, 0x1.afe2e2p-13f, 0x1.021ab8p-9f}, {0x1.44dfbap-2f, 0x1.0752d8p-7f, 0x1.15e96p-2f}, {0x1.58465p-7f, 0x1.fe15c2p-15f, -0x1.95fd1cp-7f}, {{
    {{-0x1.f07226p-1f, 0x1.0a453p-8f, 0x1.5599cap+1f}, {-0x1.24495p+1f, 0x1.36df82p+2f, 0x1.e554ap+2f}, {0x1.a73c86p+9f, 0x1.14d9aap+4f, 0x1.0caefap+10f}, {0x1.a734p+9f, 0x1.14d9acp+4f, 0x1.0cab58p+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.6665c4p-4f, 0x1.0fb712p+9f, 129},
    {{-0x1.f06f3p-1f, 0x1.a7f822p+0f, 0x1.559ab6p+1f}, {-0x1.24e09ap+1f, 0x1.9a52b2p+1f, 0x1.e55538p+2f}, {0x1.a73ce8p+9f, 0x1.2f4734p+4f, 0x1.0caec2p+10f}, {0x1.a7378cp+9f, 0x1.2f4734p+4f, 0x1.0cac78p+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.c31b84p-5f, 0.0f, 129},
    {{0x1.f06db6p-1f, 0x1.a7f84p+0f, 0x1.559844p+1f}, {-0x1.6561cp-2f, 0x1.999d2cp+1f, 0x1.e55558p+2f}, {0x1.a69bc8p+9f, 0x1.2f5d88p+4f, 0x1.0d0d28p+10f}, {0x1.a69668p+9f, 0x1.2f5d88p+4f, 0x1.0d0adep+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.c3de4p-5f, 0.0f, 129},
    {{0x1.f06ffp-1f, 0x1.0a51ecp-8f, 0x1.5598dep+1f}, {-0x1.6084dp-2f, 0x1.3684bep+2f, 0x1.e55548p+2f}, {0x1.a69b64p+9f, 0x1.14effep+4f, 0x1.0d0d62p+10f}, {0x1.a692dap+9f, 0x1.14fp+4f, 0x1.0d09bep+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.671a38p-4f, 0x1.0f0a88p+9f, 129},
}}};
const OracleFrame k108 = {{Vector3{-0x1.4c66ccp-1f, 0x1.5e99c6p-6f, -0x1.8545ccp-1f}, Vector3{0x1.6819ep-9f, 0x1.ffd3e8p-1f, 0x1.a68b52p-6f}, Vector3{0x1.856c9cp-1f, 0x1.e0342cp-7f, -0x1.4c51e4p-1f}}, {0x1.a7e386p+9f, 0x1.13fe3ep+4f, 0x1.0d47d8p+10f}, {0x1.237218p-1f, -0x1.578da8p-3f, 0x1.edd5d4p-2f}, {0x1.4cd884p-5f, -0x1.73d0b4p-10f, -0x1.8820a4p-5f}, {0x1.6cc42ap+0f, -0x1.698336p-3f, 0x1.37f5dcp+0f}, {-0x1.46ea06p-4f, -0x1.128204p-10f, 0x1.71ded2p-4f}, {{
    {{-0x1.f06ffap-1f, 0x1.0a5f4p-8f, 0x1.5599dp+1f}, {-0x1.244becp+1f, 0x1.35cea2p+2f, 0x1.e55628p+2f}, {0x1.a73c8ap+9f, 0x1.151de2p+4f, 0x1.0caefap+10f}, {0x1.a73064p+9f, 0x1.151de4p+4f, 0x1.0ca9cap+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.ff397cp-4f, 0x1.32f3acp+11f, 131},
    {{-0x1.f072e6p-1f, 0x1.a7f81ap+0f, 0x1.559b02p+1f}, {-0x1.24e4f8p+1f, 0x1.983892p+1f, 0x1.e554bp+2f}, {0x1.a73cecp+9f, 0x1.2f8a78p+4f, 0x1.0caecp+10f}, {0x1.a734eap+9f, 0x1.2f8a7ap+4f, 0x1.0cab56p+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.50c22cp-4f, 0.0f, 131},
    {{0x1.f06f2ap-1f, 0x1.a7f81ap+0f, 0x1.559a5ep+1f}, {-0x1.6561fp-2f, 0x1.97871cp+1f, 0x1.e55558p+2f}, {0x1.a69bc8p+9f, 0x1.2fa04ap+4f, 0x1.0d0d28p+10f}, {0x1.a693c4p+9f, 0x1.2fa04cp+4f, 0x1.0d09bcp+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.5145f4p-4f, 0.0f, 131},
    {{0x1.f065eap-1f, 0x1.0a2124p-8f, 0x1.559bc8p+1f}, {-0x1.60ca7p-2f, 0x1.3575eep+2f, 0x1.e55436p+2f}, {0x1.a69b68p+9f, 0x1.1533b2p+4f, 0x1.0d0d5ep+10f}, {0x1.a68f3ep+9f, 0x1.1533b4p+4f, 0x1.0d082ep+10f}, {0x1.8573ap-1f, -0x1.99c45p-17f, 0x1.4c5f56p-1f}, -0x1.ff9adap-4f, 0x1.2e638cp+11f, 131},
}}};

// The static box 0x08f49810: world transform +0x04 (rows, origin), shape 0x08f49770 half extents minus margin +0x20, margin +0x30 = 0.04.
BulletBox obstacle() {
    BulletBox box;
    box.transform.basis = {Vector3{-0x1.4c5f56p-1f, 0x1.559af6p-16f, 0x1.8573ap-1f}, Vector3{-0x1.7f254cp-15f, -0x1p+0f, -0x1.99c45p-17f},
                           Vector3{0x1.8573ap-1f, -0x1.65f0c2p-15f, 0x1.4c5f56p-1f}};
    box.transform.origin = {0x1.a39c7p+9f, 0x1.62917ep+4f, 0x1.0be318p+10f};
    box.implicitHalf = {0x1.e93648p+1f, 0x1.55136cp+2f, 0x1.e2c5a4p+2f};
    return box;
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

VehicleRigidBody trooperBody(const std::array<Vector3, 3>& basis, const Vector3& origin) {
    const VehicleRecord record = trooper::record();
    VehicleRigidBody body;
    body.orientation = orientationFromBasis(basis);
    body.position = origin;
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    return body;
}

ContactManifold manifoldOf(const OracleFrame& frame) {
    ContactManifold manifold;
    manifold.box = 0;
    manifold.breakingThreshold = 0x1.f2a2c6p-5f;
    for (const OraclePoint& o : frame.points) {
        ManifoldPoint p;
        p.localA = o.localA;
        p.localB = o.localB;
        p.worldA = o.worldA;
        p.worldB = o.worldB;
        p.normalOnB = o.normal;
        p.distance = o.distance;
        p.appliedImpulse = o.impulse;
        p.lifeTime = o.lifeTime;
        p.friction = 0.25f;
        manifold.points.push_back(p);
    }
    return manifold;
}

void boxBoxSynthetic() {
    BulletTransform identity;
    identity.basis = {Vector3{1, 0, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 1}};
    BulletTransform above = identity;
    above.origin = {0.0f, 1.9f, 0.0f};
    const std::vector<BoxBoxContact> face = boxBoxContacts(above, {0.5f, 1.0f, 0.5f}, identity, {2.0f, 1.0f, 2.0f});
    expect(face.size() == 4, "a box resting on a bigger box: four face contacts", double(face.size()));
    for (const BoxBoxContact& c : face) {
        near(c.normalOnB[1], 1.0, 1e-6, "face contact normal points from B to A");
        near(c.depth, -0.1, 1e-6, "face contact depth (negative: penetration)");
        near(c.pointOnB[1], 1.0, 1e-6, "face contact point on the top face of B");
    }
    above.origin = {0.0f, 2.01f, 0.0f};
    expect(boxBoxContacts(above, {0.5f, 1.0f, 0.5f}, identity, {2.0f, 1.0f, 2.0f}).empty(), "separate boxes: no contact");
    BulletTransform edge;
    const float c = std::sqrt(0.5f);
    edge.basis = {Vector3{c, -c, 0}, Vector3{c, c, 0}, Vector3{0, 0, 1}};
    BulletTransform crossed = identity;
    crossed.basis = {Vector3{1, 0, 0}, Vector3{0, c, -c}, Vector3{0, c, c}};
    edge.origin = {0.0f, 0.0f, 0.0f};
    crossed.origin = {0.0f, 2.0f * c - 0.05f, 0.0f};
    const std::vector<BoxBoxContact> edges = boxBoxContacts(crossed, {1.0f, 0.5f, 0.5f}, edge, {0.5f, 0.5f, 1.0f});
    expect(!edges.empty(), "crossed boxes touch");
    float window = 0;
    for (const BoxBoxContact& e : edges) window = std::min(window, e.depth);
    expect(window < 0.0f, "crossed boxes penetrate");

    BulletBox child = trooper::boxContacts(nullptr).child;
    // Manifold 0x08472cb0 +0x470: 0x3d7951 63 = 0.0608686; the trooper box (disc 3.0434) is smaller than the obstacle's.
    near(std::min(bulletBoxBreakingThreshold(child), bulletBoxBreakingThreshold(obstacle())), 0x1.f2a2c6p-5, 1e-7, "manifold breaking threshold");
}

void narrowphase() {
    VehicleRigidBody body = trooperBody(k108.basis, k108.origin);
    const std::vector<BulletBox> boxes = {obstacle()};
    VehicleBoxContacts contacts = trooper::boxContacts(&boxes);
    contacts.manifolds.push_back(manifoldOf(k107));
    collideVehicleWithBoxes(body, contacts);
    expect(contacts.manifolds.size() == 1 && contacts.manifolds[0].points.size() == 4, "the manifold keeps four points");
    if (contacts.manifolds.size() != 1 || contacts.manifolds[0].points.size() != 4) return;
    for (std::size_t i = 0; i < 4; ++i) {
        const ManifoldPoint& p = contacts.manifolds[0].points[i];
        const OraclePoint& o = k108.points[i];
        for (std::size_t a = 0; a < 3; ++a) {
            near(p.localA[a], o.localA[a], 2e-5, "local point on the trooper");
            near(p.localB[a], o.localB[a], 2e-4, "local point on the box");
            near(p.normalOnB[a], o.normal[a], 1e-5, "contact normal (the box face)");
        }
        near(p.distance, o.distance, 1e-4, "contact distance");
        expect(p.lifeTime == o.lifeTime, "life time: two refreshes per step (compound, then box-box)", p.lifeTime);
        expect(p.appliedImpulse == 0.0f, "a replaced point takes the row impulse 0 (no warm start)", p.appliedImpulse);
    }
}

void processedCallback() {
    VehicleRigidBody body = trooperBody(k108.basis, k108.origin);
    const std::vector<BulletBox> boxes = {obstacle()};
    VehicleBoxContacts contacts = trooper::boxContacts(&boxes);
    contacts.manifolds.push_back(manifoldOf(k107));
    int calls = 0;
    const ManifoldPointObserver count = [&calls](const ManifoldPoint&) {
        calls++;
        return false;
    };
    collideVehicleWithBoxes(body, contacts, &count);
    // Provenance: 0x1e518c refreshContactPoints calls gContactProcessedCallback (0x271764, stored by 0x2e3bec) for each surviving point; the step
    // refreshes the four recorded points twice (before and after the box-box contacts), llvm-objdump 0x1e5468..0x1e5484.
    expect(calls == 8, "the processed callback runs for every surviving point of both refreshes", calls);
    VehicleBoxContacts dropped = trooper::boxContacts(&boxes);
    dropped.manifolds.push_back(manifoldOf(k107));
    const ManifoldPointObserver dropAll = [](const ManifoldPoint&) { return true; };
    collideVehicleWithBoxes(body, dropped, &dropAll);
    expect(dropped.manifolds.size() == 1 && dropped.manifolds[0].points.empty(), "a reply 2000 drops every point at both refreshes");
}

void solver() {
    VehicleRigidBody body = trooperBody(k108.basis, k108.origin);
    for (std::size_t a = 0; a < 3; ++a) {
        body.velocity[a] = k108.velocity[a] - k108.deltaLinear[a];
        body.angularVelocity[a] = k108.angularVelocity[a] - k108.deltaAngular[a];
    }
    std::vector<ContactManifold> manifolds = {manifoldOf(k108)};
    for (ManifoldPoint& p : manifolds[0].points) p.appliedImpulse = 0.0f;
    BulletSolverInfo info;
    info.timeStep = kVehicleWorldStep;
    solveVehicleContacts(body, manifolds, info);
    std::printf("solver: velocity %.5f %.5f %.5f angular %.5f %.5f %.5f\n", body.velocity[0], body.velocity[1], body.velocity[2], body.angularVelocity[0],
                body.angularVelocity[1], body.angularVelocity[2]);
    for (std::size_t a = 0; a < 3; ++a) {
        near(body.velocity[a], k108.velocity[a], 1e-4, "velocity after the solver (update 108)");
        near(body.angularVelocity[a], k108.angularVelocity[a], 1e-4, "angular velocity after the solver (update 108)");
    }
    for (std::size_t i = 0; i < 4; ++i) near(manifolds[0].points[i].appliedImpulse, k108.points[i].impulse, 0.5, "normal impulse kept in the point");
}

// runA.mem updates 105, 111 and 112 (rigid +0x04, +0x34, +0x13c, +0x14c): the trooper at 27.6 m/s 4.6 m before the obstacle, the update before
// the hit and the hit (velocity reversed to (3.22, 0.07, 1.95) by the box contacts).
struct RunState {
    std::array<Vector3, 3> basis;
    Vector3 origin, velocity, angularVelocity;
};
const RunState kRun105 = {{Vector3{-0x1.6ab5fcp-1f, -0x1.8da554p-9f, -0x1.695ca8p-1f}, Vector3{0x1.a55a4p-10f, 0x1.fffd8ap-1f, -0x1.836e42p-8f}, Vector3{0x1.695d44p-1f, -0x1.5ccf42p-8f, -0x1.6ab398p-1f}}, {0x1.a9eb0ep+9f, 0x1.139bccp+4f, 0x1.0e65aap+10f}, {-0x1.35bf2cp+4f, -0x1.df69ep-3f, -0x1.36f8f6p+4f}, {0x1.331412p-4f, -0x1.7f0ffep-19f, -0x1.53c8fp-4f}};
const Vector3 kRun112Velocity = {0x1.9c158p+1f, 0x1.1a71eap-4f, 0x1.f2fd2p+0f};
const Vector3 kRun112Position = {0x1.a7a2acp+9f, 0x1.140514p+4f, 0x1.0d4044p+10f};

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
    // 0x28b070 spawned 217 boxes in the oracle's world (227 objects: 217 static boxes, 4 vehicles, 6 kinematic objects).
    expect(boxes.size() == 217, "the level's collision_vehicle boxes", double(boxes.size()));
    const BulletBox reference = obstacle();
    int found = -1;
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const Vector3 d = {boxes[i].transform.origin[0] - reference.transform.origin[0], boxes[i].transform.origin[1] - reference.transform.origin[1],
                           boxes[i].transform.origin[2] - reference.transform.origin[2]};
        if (d[0] * d[0] + d[1] * d[1] + d[2] * d[2] < 1e-6f) found = int(i);
    }
    expect(found >= 0, "the obstacle of runA is a collision_vehicle box");
    if (found >= 0) {
        for (std::size_t a = 0; a < 3; ++a) {
            near(boxes[std::size_t(found)].implicitHalf[a], reference.implicitHalf[a], 1e-5, "obstacle half extents minus margin");
            for (std::size_t k = 0; k < 3; ++k) near(boxes[std::size_t(found)].transform.basis[a][k], reference.transform.basis[a][k], 1e-6, "obstacle basis");
        }
    }

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
    VehicleBoxContacts contacts = trooper::boxContacts(&boxes);
    VehicleFloor floor{FloorMesh(*floorItem), FloorTracker{}, trooper::collisionBody(record), &contacts};
    VehicleRigidBody body = trooperBody(kRun105.basis, kRun105.origin);
    body.velocity = kRun105.velocity;
    body.angularVelocity = kRun105.angularVelocity;
    BodyInputs held;
    held.accelerate = 1.0f;
    // Test stand-in: the drivetrain state of runA is not traced. It is the port's own after A held from rest on a flat plane until the
    // forward speed reaches the oracle's 27.6 m/s (gear, engine, clutch and wheel spin of a body at that speed).
    DrivetrainState drive = vehicleDrivetrain(record);
    {
        VehicleRigidBody plane = vehicleRigidBody(record, {0.0f, 0.95f, 0.0f}, 0.0f);
        for (int i = 0; i < 400 && forwardSpeed(plane) < forwardSpeed(body); ++i)
            stepVehiclePhysics(plane, drive, record, held, false, trooper::flatGround, kVehicleWorldStep);
    }
    bool awake = true;
    for (int update = 106; update <= 112; ++update) awake = stepVehicleWorld(body, drive, awake, record, held, ray, 1, &floor);
    std::printf("hit: position %.3f %.3f %.3f velocity %.3f %.3f %.3f\n", body.position[0], body.position[1], body.position[2], body.velocity[0],
                body.velocity[1], body.velocity[2]);
    near(body.position[0], kRun112Position[0], 0.02, "the trooper stops at the obstacle (x, update 112)");
    near(body.position[2], kRun112Position[2], 0.02, "the trooper stops at the obstacle (z, update 112)");
    near(body.velocity[0], kRun112Velocity[0], 0.05, "the hit throws the trooper back (x velocity, update 112)");
    near(body.velocity[2], kRun112Velocity[2], 0.05, "the hit throws the trooper back (z velocity, update 112)");
    for (int update = 113; update <= 150; ++update) awake = stepVehicleWorld(body, drive, awake, record, held, ray, 1, &floor);
    std::printf("pushing: position %.3f %.3f %.3f velocity %.3f %.3f %.3f\n", body.position[0], body.position[1], body.position[2], body.velocity[0],
                body.velocity[1], body.velocity[2]);
    // runA update 150: (847.618, 17.257, 1076.984), pushing against the box with A held.
    near(body.position[0], 847.618, 0.02, "the trooper stays at the obstacle with A held (x, update 150)");
    near(body.position[1], 17.257, 0.01, "the trooper stays on its wheels (update 150)");
    near(body.position[2], 1076.984, 0.02, "the trooper stays at the obstacle with A held (z, update 150)");
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    boxBoxSynthetic();
    narrowphase();
    processedCallback();
    solver();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("vehicle-box-contact-test: data not available, skipped\n");
            return 77;
        }
        if (real(argv[1], argv[2]) != 0) return 1;
    }
    if (failures != 0) return 1;
    std::printf("vehicle box contact tests passed\n");
    return 0;
}

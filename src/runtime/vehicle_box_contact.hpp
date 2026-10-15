#pragma once
// The Bullet 2.78 contacts of a vehicle body with the level's static boxes, as the world step 0x30c768 runs them after the vehicle's floor
// pass (docs/re/vehicles-physics-collision.md): the compound algorithm 0x32367c with its child box, the box-box algorithm 0x31fcf0 and the
// detector 0x2ef950 / 0x2cf72c (dBoxBox2), the manifold result 0x2f0888 with the persistent manifold (0x33fc88, 0x306edc, 0x1e518c), and the
// sequential impulse solver of world+0xb0 (0x331814: setup 0x332fcc, contacts 0x3318a0 / 0x332368, friction 0x3329e0, rows 0x333cfc and
// 0x1e32d0, finish 0x333a0c) with the solver info built by 0x30c768. No SDL dependency.
#include "runtime/vehicle_body.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::assets {
struct Model;
}

namespace chase::runtime {

// btTransform: basis rows and origin; world = basis * local + origin.
struct BulletTransform {
    std::array<Vector3, 3> basis{};
    Vector3 origin{};
};
Vector3 bulletApply(const BulletTransform& t, const Vector3& local);
Vector3 bulletInverseApply(const BulletTransform& t, const Vector3& world);
// The rigid body's world transform (rigid +0x04): the rotation of the quaternion, the centre of mass.
BulletTransform bulletBodyTransform(const VehicleRigidBody& body);
// 0x218e5c: a scene matrix (rows right, up, forward, translation) to a transform (the basis is the transposed 3x3 part).
BulletTransform bulletTransformFromMatrix(const float matrix[16]);

// btBoxShape: the shape keeps the half extents minus the margin (+0x20) and the margin (+0x30, 0.04 in every box read in the oracle).
constexpr float kBulletBoxMargin = 0.04f;
// gContactBreakingThreshold (0x379d4c). The dispatcher flag +4 bit 1 (2 in the oracle) makes the manifold threshold the smaller of
// 0.02 times each shape's angular motion disc (0x309cf0, 0x33f998, 0x33f938, 0x33f82c).
constexpr float kBulletContactBreakingThreshold = 0.02f;
// BT_LARGE_FLOAT (0x5d5e0b6b): the contact processing threshold of every body.
constexpr float kBulletLargeFloat = 1e18f;
// btManifoldResult::calculateCombinedFriction (0x2f0888): the product of the frictions (+0xe0) clamped to -10..10.
constexpr float kBulletFrictionLimit = 10.0f;
// 0x28b070 builds each static box with friction 0.5 and restitution 0. The vehicle class loader 0x243f90 reads "BodyFriction" but stores the
// constant 0.5 at body+0xdac and copies it to the rigid body's friction +0xe0; with the boxes' restitution 0 the vehicle's restitution does
// not matter for these contacts.
constexpr float kBulletBoxFriction = 0.5f;
constexpr float kBulletVehicleFriction = 0.5f;

struct BulletBox {
    BulletTransform transform;
    Vector3 implicitHalf{};
    float margin = kBulletBoxMargin;
};
BulletBox bulletBox(const BulletTransform& transform, const Vector3& halfExtents);
Vector3 bulletBoxHalfExtents(const BulletBox& box);
// btTransformAabb of the box under `transform`.
void bulletBoxAabb(const BulletBox& box, const BulletTransform& transform, Vector3& low, Vector3& high);
float bulletBoxBreakingThreshold(const BulletBox& box);

// 0x28b070: every box record of the level's master model whose name starts with "collision_vehicle" (17 characters, 0x22c28c) becomes a static
// Bullet box with the record's half extents (+0x10) and matrix (+0x1c).
std::vector<BulletBox> levelVehicleBoxes(const assets::Model& model);

// One contact of the box-box detector: btManifoldResult::addContactPoint(normal on B, point on B, depth).
struct BoxBoxContact {
    Vector3 normalOnB{};
    Vector3 pointOnB{};
    float depth = 0;
};
// btBoxBoxDetector::getClosestPoints 0x2ef950: dBoxBox2 0x2cf72c of box A against box B with at most 4 contacts; an empty list when the boxes
// are separate.
std::vector<BoxBoxContact> boxBoxContacts(const BulletTransform& a, const Vector3& halfA, const BulletTransform& b, const Vector3& halfB);

// btManifoldPoint (0x118 bytes; the u16 at +0xb4 is the original's own contact flag word, bit 2 keeps only the vertical turn and the
// horizontal push, written only by the contact callback of objects with collision flag 8).
struct ManifoldPoint {
    Vector3 localA{}, localB{}, worldB{}, worldA{}, normalOnB{};
    float distance = 0;
    float friction = 0, restitution = 0;
    float appliedImpulse = 0;
    bool lateralInitialized = false;
    float lateralImpulse1 = 0, lateralImpulse2 = 0;
    int lifeTime = 0;
    Vector3 lateralDir1{}, lateralDir2{};
    std::uint16_t flags = 0;
    std::array<float, 3> rowImpulse{};  // the accumulated impulses of the three constraint rows (+0xd4, +0xf4, +0x114)
};
struct ContactManifold {
    int box = -1;
    std::vector<ManifoldPoint> points;
    float breakingThreshold = 0;
    float processingThreshold = kBulletLargeFloat;
};
// btPersistentManifold::refreshContactPoints 0x1e518c.
void refreshManifold(ContactManifold& manifold, const BulletTransform& a, const BulletTransform& b);
// btManifoldResult::addContactPoint 0x2f0888 for a manifold whose body 0 is A.
void addManifoldContact(ContactManifold& manifold, const BulletTransform& a, const BulletTransform& b, const BoxBoxContact& contact, float friction,
                        float restitution);

// The contact solver info built on the stack of 0x30c768.
struct BulletSolverInfo {
    float timeStep = 0;
    int iterations = 10;
    float erp = 0.2f;
    float erp2 = 0.1f;
    bool splitImpulse = false;
    float splitImpulseThreshold = -0.02f;
    float linearSlop = 0;
    float warmStarting = 0.85f;
    std::uint32_t mode = 0x104;  // SOLVER_SIMD | SOLVER_USE_WARMSTARTING
    int restingRestitutionThreshold = 2;
};
// The solve of 0x331814 for one dynamic vehicle body and the static boxes of its manifolds (body 0 of every manifold is the vehicle). The
// impulses of each contact are kept in the manifold points as 0x333a0c does.
void solveVehicleContacts(VehicleRigidBody& body, std::vector<ContactManifold>& manifolds, const BulletSolverInfo& info);

// The vehicle's Bullet collision shape: a compound with one box child (vtable 0x362444 with a 0x361974 child).
// STAND-IN VEH-S50: the child box is the AABB of the compound read in the oracle (vehicleCollisionBoxStandIn); its centre is the child offset.
struct VehicleBoxContacts {
    const std::vector<BulletBox>* boxes = nullptr;
    Vector3 childOffset{};
    BulletBox child;
    std::vector<ContactManifold> manifolds;
};
VehicleBoxContacts vehicleBoxContacts(const std::string& className, const std::vector<BulletBox>* boxes);
// The collision detection of 0x30c768 for the vehicle against the static boxes (0x32367c): manifolds of boxes whose AABB the child box no longer
// overlaps are released; the others are refreshed, take the box-box contacts and are refreshed again.
void collideVehicleWithBoxes(const VehicleRigidBody& body, VehicleBoxContacts& contacts);
// One world step after the vehicle's floor pass: the collision detection and the solver with the step `dt`.
void stepVehicleBoxContacts(VehicleRigidBody& body, VehicleBoxContacts& contacts, float dt);

}

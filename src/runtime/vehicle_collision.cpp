#include "runtime/vehicle_collision.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

Vector3 sub(const Vector3& a, const Vector3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vector3 cross(const Vector3& a, const Vector3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float dot(const Vector3& a, const Vector3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vector3 scaled(const Vector3& a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }

Vector3 normalOf(const assets::CollisionTriangle& t) { return {t.normal[0], t.normal[1], t.normal[2]}; }
float planeDistance(const assets::CollisionTriangle& t, const Vector3& p) { return dot(normalOf(t), p) - t.d; }

std::uint32_t floatBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

Vector3 bodyArm(const VehicleRigidBody& body, const Vector3& local) { return sub(bodyToWorld(body, local), body.position); }

Vector3 contactVelocity(const VehicleRigidBody& body, const Vector3& arm) { return sub(body.velocity, cross(arm, body.angularVelocity)); }

void addContact(FloorContacts& contacts, const FloorContact& contact) {
    if (contact.time < contacts.earliest) contacts.items.clear();
    contacts.items.push_back(contact);
    contacts.earliest = contacts.items.front().time;
}

float wallContact(const FloorMesh& mesh, int cell, int edge, int contactCell, const Vector3& point, const Vector3& direction, const Vector3& end,
                  float length, FloorContacts& contacts) {
    const FloorPoint a = mesh.vertex(cell, edge);
    const FloorPoint b = mesh.vertex(cell, (edge + 1) % 3);
    const float ex = -(b[0] - a[0]);
    const float ez = b[2] - a[2];
    const float inverse = 1.0f / std::sqrt(ez * ez + 0.0f + ex * ex);
    const Vector3 normal = {ez * inverse, inverse * 0.0f, ex * inverse};
    const float plane = normal[0] * a[0] + normal[1] * a[1] + normal[2] * a[2];
    const float start = (normal[0] * point[0] + normal[1] * point[1] + normal[2] * point[2]) - plane;
    const float time = start / (start - ((normal[0] * end[0] + normal[1] * end[1] + normal[2] * end[2]) - plane));
    if (contacts.earliest < time) return time;
    const float reach = length * time;
    FloorContact contact;
    contact.time = time;
    contact.point = {point[0] + direction[0] * reach, point[1] + direction[1] * reach, point[2] + direction[2] * reach};
    contact.normal = normal;
    contact.cell = contactCell;
    addContact(contacts, contact);
    return time;
}

int leaveEdge(const FloorMesh& mesh, int cell, const Vector3& from, float dx, float dz) {
    const auto side = [&](const FloorPoint& v) { return (v[2] - from[2]) * dx - dz * (v[0] - from[0]); };
    if (side(mesh.vertex(cell, 0)) <= 0.0f) return side(mesh.vertex(cell, 2)) <= 0.0f ? 1 : 2;
    return side(mesh.vertex(cell, 1)) <= 0.0f ? 0 : 1;
}

}

std::array<Vector3, 8> vehicleProbePoints(const Vector3& low, const Vector3& high, const std::vector<VehicleWheel>& wheels) {
    float bottom = low[1];
    if (wheels.size() > 2) bottom = std::min({bottom, wheels[0].suspension.mount[1], wheels[2].suspension.mount[1]});
    else if (wheels.empty()) bottom += 1.0f;
    const float s = 0.9f;
    return {Vector3{low[0], bottom, low[2]},          Vector3{high[0], bottom, low[2]},          Vector3{low[0], bottom, high[2]},
            Vector3{high[0], bottom, high[2]},        Vector3{low[0] * s, high[1], high[2] * s}, Vector3{high[0] * s, high[1], high[2] * s},
            Vector3{low[0] * s, high[1], low[2] * s}, Vector3{high[0] * s, high[1], low[2] * s}};
}

void vehicleCollisionBoxStandIn(const std::string& className, Vector3& low, Vector3& high) {
    const std::string van = "vehicle_van_donut";
    if (className.size() >= van.size() && className.compare(className.size() - van.size(), van.size(), van) == 0) {
        low = {-0x1.24ccc8p+0f, -0x1.884b5cp-2f, -0x1.6f2d6cp+1f};
        high = {0x1.24ccc8p+0f, 0x1.3668d2p+2f, 0x1.3bfa38p+1f};
        return;
    }
    low = {-0x1.f06dd8p-1f, 0x1.0a46p-8f, -0x1.6e0004p+1f};
    high = {0x1.f06dd8p-1f, 0x1.a7f832p+0f, 0x1.55999cp+1f};
}

float sweepFloorPoint(const FloorMesh& mesh, float dt, int& cell, const Vector3& point, const Vector3& velocity, FloorContacts& contacts,
                      bool percentWalls) {
    if (cell < 0) return -1.0f;
    float speed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
    if (speed == 0.0f) return -1.0f;
    float length = speed * dt;
    speed = 1.0f / speed;
    const Vector3 direction = {velocity[0] * speed, velocity[1] * speed, velocity[2] * speed};
    const float depth = planeDistance(mesh.triangle(cell), point);
    if (depth < 0.0f) {
        const assets::CollisionTriangle& triangle = mesh.triangle(cell);
        if (triangle.surface == kFloorSurfaceSoft) return -1.0f;
        if (dot(velocity, normalOf(triangle)) < 0.0f) {
            if (0.0f < contacts.earliest) contacts.items.clear();
            FloorContact contact;
            contact.time = 0.0f;
            contact.point = point;
            contact.normal = normalOf(triangle);
            contact.cell = cell;
            contact.depth = -depth;
            contacts.items.push_back(contact);
            contacts.earliest = contacts.items.front().time;
        }
        return 0.0f;
    }
    const Vector3 end = {point[0] + direction[0] * length, point[1] + direction[1] * length, point[2] + direction[2] * length};
    for (int visited = 0; visited <= kFloorSweepCells; ++visited) {
        const int edge = leaveEdge(mesh, cell, point, direction[0], direction[2]);
        const FloorPoint a = mesh.vertex(cell, edge);
        const FloorPoint b = mesh.vertex(cell, (edge + 1) % 3);
        const float ex = b[0] - a[0];
        const float ez = b[2] - a[2];
        const assets::CollisionTriangle& triangle = mesh.triangle(cell);
        const float endDepth = planeDistance(triangle, end);
        if (endDepth < 0.0f) {
            const float startDepth = planeDistance(triangle, point);
            const float time = startDepth / (startDepth - endDepth);
            const float reach = length * time;
            const Vector3 hit = {point[0] + direction[0] * reach, point[1] + direction[1] * reach, point[2] + direction[2] * reach};
            if (0.0f < (hit[0] - a[0]) * ez - ex * (hit[2] - a[2])) {
                if (triangle.surface == kFloorSurfaceSoft) return -1.0f;
                if (contacts.earliest < time) return time;
                FloorContact contact;
                contact.time = time;
                contact.point = hit;
                contact.normal = normalOf(triangle);
                contact.cell = cell;
                addContact(contacts, contact);
                return time;
            }
        }
        if (0.0f < (end[0] - a[0]) * ez - ex * (end[2] - a[2])) return -1.0f;
        const int next = floorNeighbour(mesh.cell(cell).neighbour[edge]);
        if (next < 0) return wallContact(mesh, cell, edge, cell, point, direction, end, length, contacts);
        const int from = cell;
        cell = next;
        if (percentWalls && mesh.triangle(next).surface == kFloorSurfaceWall && mesh.triangle(from).surface != kFloorSurfaceWall)
            return wallContact(mesh, from, edge, next, point, direction, end, length, contacts);
    }
    return -1.0f;
}

std::uint16_t floorContactFlags(const VehicleCollisionBody& shape, std::uint8_t cellSurface, const FloorContact& contact) {
    if (!shape.spheres || cellSurface == 6) return 0;
    const bool special = shape.surface == 0xf || cellSurface == 0xf || shape.surface == 1 || cellSurface == 1;
    if (special && (floatBits(std::fabs(contact.normal[1])) < 0x3f350481u)) return 4;
    return 0;
}

void floorContactImpulse(VehicleRigidBody& body, const FloorContact& contact, std::uint8_t surface, float surfaceFriction, float upY) {
    const Vector3 normal = surface == kFloorSurfaceSoft ? Vector3{0.0f, 1.0f, 0.0f} : contact.normal;
    Vector3 arm = sub(contact.point, body.position);
    const Vector3 velocity = contactVelocity(body, arm);
    const float approach = dot(normal, velocity) - kVehicleContactSlop;
    if (approach >= 0.0f) return;
    const float speed = std::min(approach * -kVehicleBounceSpeedScale, 1.0f);
    const float bounce = std::pow(1.0f - speed, kVehicleBouncePower);
    const Vector3 turn = cross(worldInverseInertia(body, cross(arm, normal)), arm);
    const float push = -((bounce * kVehicleBounceScale + kVehicleBounceBase) * approach) / (body.inverseMass + dot(normal, turn));
    Vector3 impulse = scaled(normal, push);
    const Vector3 slide = sub(velocity, scaled(normal, approach));
    const float slideSpeed = std::sqrt(dot(slide, slide));
    if (slideSpeed != 0.0f) {
        float limit = surfaceFriction * push * kVehicleContactFriction;
        if (std::int32_t(floatBits(upY)) > std::int32_t(floatBits(kVehicleUprightLimit))) limit *= kVehicleUprightFriction;
        Vector3 friction;
        if (slideSpeed <= body.inverseMass * limit) {
            const Vector3 back = cross(worldInverseInertia(body, cross(arm, scaled(slide, -1.0f))), arm);
            friction = scaled(slide, -slideSpeed / (body.inverseMass - dot(slide, back)));
        } else {
            friction = scaled(scaled(slide, 1.0f / slideSpeed), -limit);
        }
        for (std::size_t k = 0; k < 3; ++k) impulse[k] += friction[k];
    }
    if (contact.flags & 4) {
        impulse[1] = 0.0f;
        arm[1] = 0.0f;
    }
    applyImpulse(body, impulse, arm);
}

void floorPushImpulse(VehicleRigidBody& body, const FloorContact& contact, std::uint8_t surface, float surfaceFriction, float massTimesTwo) {
    if (surface == kFloorSurfaceSoft || contact.depth == 0.0f) return;
    const Vector3 arm = sub(contact.point, body.position);
    const Vector3 velocity = contactVelocity(body, arm);
    const float approach = dot(contact.normal, velocity) - kVehicleContactSlop;
    Vector3 impulse = scaled(contact.normal, massTimesTwo * contact.depth);
    const Vector3 slide = sub(velocity, scaled(contact.normal, approach));
    const float slideSpeed = std::sqrt(dot(slide, slide));
    if (slideSpeed != 0.0f) {
        const float limit = surfaceFriction * contact.depth * kVehiclePushFriction;
        Vector3 friction;
        if (slideSpeed <= body.inverseMass * limit) {
            const Vector3 back = cross(worldInverseInertia(body, cross(arm, scaled(slide, -1.0f))), arm);
            friction = scaled(slide, -slideSpeed / (body.inverseMass - dot(slide, back)));
        } else {
            friction = scaled(scaled(slide, 1.0f / slideSpeed), -limit);
        }
        for (std::size_t k = 0; k < 3; ++k) impulse[k] += friction[k];
    }
    applyImpulse(body, impulse, arm);
}

bool insideFloorCell(const FloorMesh& mesh, int cell, const Vector3& p) {
    const FloorPoint v0 = mesh.vertex(cell, 0), v1 = mesh.vertex(cell, 1);
    const float dz = p[2] - v0[2], dx = p[0] - v0[0];
    if ((v1[0] - v0[0]) * dz - (v1[2] - v0[2]) * dx >= 0.0f) return false;
    const FloorPoint v2 = mesh.vertex(cell, 2);
    if ((v0[0] - v2[0]) * dz - (v0[2] - v2[2]) * dx >= 0.0f) return false;
    return (v2[0] - v1[0]) * (p[2] - v1[2]) - (v2[2] - v1[2]) * (p[0] - v1[0]) < 0.0f;
}

int locateFloorCell(const FloorMesh& mesh, const Vector3& point) {
    const std::uint32_t count = std::uint32_t(mesh.cellCount());
    if (count == 0) return -1;
    const auto distance = [&](std::uint32_t cell) {
        const FloorPoint v = mesh.vertex(int(cell), 0);
        const float x = v[0] - point[0], y = v[1] - point[1], z = v[2] - point[2];
        return std::sqrt(x * x + y * y + z * z);
    };
    std::uint32_t low = 0, high = count - 1;
    do {
        if (distance(low) < distance(high)) high = low + ((high - low) >> 1);
        else low = low + ((high - low) >> 1);
    } while (low < high - 1);
    while (!(count <= high ? low == 0 : count == high)) {
        if (high < count) {
            if (insideFloorCell(mesh, int(high), point)) return int(high);
            ++high;
        }
        if (low != 0) {
            if (insideFloorCell(mesh, int(low), point)) return int(low);
            --low;
        }
    }
    return -1;
}

void trackFloorCell(const FloorMesh& mesh, FloorTracker& tracker, const Vector3& position) {
    if (tracker.cell < 0) {
        tracker.last = position;
        tracker.cell = locateFloorCell(mesh, position);
        return;
    }
    FloorPoint to = position;
    tracker.cell = walkFloor(mesh, tracker.cell, tracker.last, to, FloorWalkEdge::Fail);
    tracker.last = to;
}

bool collideVehicleWithFloor(VehicleRigidBody& body, const FloorMesh& mesh, const VehicleCollisionBody& shape, FloorTracker& tracker,
                             float surfaceFriction, float linearDamping, float angularDamping, float dt) {
    if (tracker.cell == -1 || dt == 0.0f) return false;
    bool answered = false;
    std::array<int, 8> cells{};
    std::array<Vector3, 8> points{};
    for (int passes = kVehicleCollisionPasses;;) {
        if (tracker.cell == -1) return answered;
        std::array<Vector3, 8> velocities{};
        for (std::size_t i = 0; i < 8; ++i) {
            const Vector3 arm = bodyArm(body, shape.points[i]);
            velocities[i] = contactVelocity(body, arm);
            points[i] = {arm[0] + body.position[0], arm[1] + body.position[1], arm[2] + body.position[2]};
            cells[i] = walkFloor(mesh, tracker.cell, body.position, points[i], FloorWalkEdge::Clip);
        }
        FloorContacts contacts;
        for (std::size_t i = 0; i < 8; ++i)
            if (cells[i] >= 0) sweepFloorPoint(mesh, dt, cells[i], points[i], velocities[i], contacts, shape.percentWalls);
        if (contacts.items.empty()) {
            integrateRigidBody(body, linearDamping, angularDamping, dt);
            dt = 0.0f;
            break;
        }
        const float time = std::max(contacts.earliest * dt * kVehicleContactTimeScale, 0.0f);
        if (time > 0.0f) {
            integrateRigidBody(body, linearDamping, angularDamping, time);
            trackFloorCell(mesh, tracker, body.position);
        }
        const float upY = bodyAxes(body)[1][1];
        for (FloorContact& contact : contacts.items) {
            const std::uint8_t surface = mesh.triangle(contact.cell).surface;
            contact.flags = floorContactFlags(shape, surface, contact);
            floorContactImpulse(body, contact, surface, surfaceFriction, upY);
        }
        dt -= time;
        answered = true;
        if (--passes == 0 || dt == 0.0f || dt < 0.0f) break;
    }
    for (std::size_t i = 0; i < 8; ++i) {
        points[i] = bodyToWorld(body, shape.points[i]);
        cells[i] = walkFloor(mesh, tracker.cell, body.position, points[i], FloorWalkEdge::Clip);
    }
    for (std::size_t i = 0; i < 8; ++i) {
        if (cells[i] < 0) continue;
        const assets::CollisionTriangle& triangle = mesh.triangle(cells[i]);
        const float depth = planeDistance(triangle, points[i]);
        if (depth >= 0.0f) continue;
        FloorContact contact;
        contact.point = points[i];
        contact.normal = normalOf(triangle);
        contact.cell = cells[i];
        contact.depth = -depth;
        floorPushImpulse(body, contact, triangle.surface, surfaceFriction, shape.massTimesTwo);
    }
    if (0.0f < dt) body.angularVelocity = {0.0f, 0.0f, 0.0f};
    return answered;
}

}

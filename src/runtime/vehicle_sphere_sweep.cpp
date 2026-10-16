#include "runtime/vehicle_sphere_sweep.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {

namespace {

constexpr float kSphereReachLimit = 5.0f;
constexpr float kWallEdgeHeight = 5.0f;
constexpr float kSphereDepthLimit = 0.1f;
constexpr std::uint32_t kSphereDepthLimitBits = 0x3dcccccd;

std::int32_t signedBits(float value) {
    std::int32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

Vector3 vectorOf(const FloorPoint& point) { return {point[0], point[1], point[2]}; }

struct SphereTriangle {
    Vector3 a, b, c;
    Vector3 normal;
    float d = 0;
};

float lengthSquared(const Vector3& from, const Vector3& to) {
    const float x = from[0] - to[0];
    const float y = from[1] - to[1];
    const float z = from[2] - to[2];
    float sum = x * x;
    sum += y * y;
    sum += z * z;
    return sum;
}

bool segmentWithinSphere(const Vector3& a, const Vector3& b, const Vector3& point, float radiusSquared) {
    const float ex = b[0] - a[0];
    const float ey = b[1] - a[1];
    const float ez = b[2] - a[2];
    const float wx = point[0] - a[0];
    const float wy = point[1] - a[1];
    const float wz = point[2] - a[2];
    float along = ex * wx;
    along += ey * wy;
    along += ez * wz;
    if (along < 0.0f) return false;
    float edgeSquared = ex * ex;
    edgeSquared += ey * ey;
    edgeSquared += ez * ez;
    const float projected = (along * along) / edgeSquared;
    if (!(edgeSquared > projected)) return false;
    float pointSquared = wx * wx;
    pointSquared += wy * wy;
    pointSquared += wz * wz;
    return pointSquared - projected <= radiusSquared;
}

bool projectedInside(const SphereTriangle& t, const Vector3& point, std::array<int, 2>& axes) {
    axes = floorProjectionAxes({t.normal[0], t.normal[1], t.normal[2]});
    return floorProjectedContains({t.a[0], t.a[1], t.a[2]}, {t.b[0], t.b[1], t.b[2]}, {t.c[0], t.c[1], t.c[2]}, {point[0], point[1], point[2]}, axes);
}

bool sphereTouchesTriangle(float radius, const Vector3& centre, const SphereTriangle& t) {
    const float radiusSquared = radius * radius;
    if (lengthSquared(t.a, centre) <= radiusSquared) return true;
    if (lengthSquared(t.b, centre) <= radiusSquared) return true;
    if (lengthSquared(t.c, centre) <= radiusSquared) return true;
    if (segmentWithinSphere(t.a, t.b, centre, radiusSquared)) return true;
    if (segmentWithinSphere(t.b, t.c, centre, radiusSquared)) return true;
    if (segmentWithinSphere(t.c, t.a, centre, radiusSquared)) return true;
    float distance = centre[1] * t.normal[1];
    distance += centre[2] * t.normal[2];
    float offset = -t.d + centre[0] * t.normal[0];
    distance += offset;
    if (std::fabs(distance) > radius) return false;
    const Vector3 projection = {centre[0] - t.normal[0] * distance, centre[1] - t.normal[1] * distance, centre[2] - t.normal[2] * distance};
    std::array<int, 2> axes;
    return projectedInside(t, projection, axes);
}

struct SegmentPoint {
    Vector3 point;
    float distanceSquared;
};

SegmentPoint closestOnSegment(const Vector3& a, const Vector3& b, const Vector3& p) {
    const float dx = b[0] - a[0];
    const float dy = b[1] - a[1];
    const float dz = b[2] - a[2];
    const float wx = p[0] - a[0];
    const float wy = p[1] - a[1];
    const float wz = p[2] - a[2];
    float along = dx * wx;
    along += dy * wy;
    along += dz * wz;
    if (along <= 0.0f) {
        float d = wx * wx;
        d += wy * wy;
        d += wz * wz;
        return {a, d};
    }
    float lengthSquared2 = dx * dx;
    lengthSquared2 += dy * dy;
    lengthSquared2 += dz * dz;
    const float length = std::sqrt(lengthSquared2);
    const float scaled = along / length;
    if (!(scaled < length)) {
        float d = (p[0] - b[0]) * (p[0] - b[0]);
        d += (p[1] - b[1]) * (p[1] - b[1]);
        d += (p[2] - b[2]) * (p[2] - b[2]);
        return {b, d};
    }
    const float t = scaled / length;
    const Vector3 point = {a[0] + dx * t, a[1] + dy * t, a[2] + dz * t};
    float d = (p[0] - point[0]) * (p[0] - point[0]);
    d += (p[1] - point[1]) * (p[1] - point[1]);
    d += (p[2] - point[2]) * (p[2] - point[2]);
    return {point, d};
}

float edgeSide(const Vector3& from, const Vector3& to, const Vector3& q, const std::array<int, 2>& axes) {
    const int u = axes[0], v = axes[1];
    return (to[u] - from[u]) * (q[v] - from[v]) - (q[u] - from[u]) * (to[v] - from[v]);
}

bool closestOnTriangleEdges(const SphereTriangle& t, const Vector3& q, const std::array<int, 2>& axes, Vector3& closest) {
    SegmentPoint candidates[3];
    int found = 0;
    if (!(edgeSide(t.a, t.b, q, axes) > 0.0f)) candidates[found++] = closestOnSegment(t.a, t.b, q);
    if (!(edgeSide(t.b, t.c, q, axes) > 0.0f)) candidates[found++] = closestOnSegment(t.b, t.c, q);
    if (!(edgeSide(t.c, t.a, q, axes) > 0.0f)) candidates[found++] = closestOnSegment(t.c, t.a, q);
    if (found == 0) return false;
    if (found > 1 && !(candidates[1].distanceSquared > candidates[0].distanceSquared)) closest = candidates[1].point;
    else closest = candidates[0].point;
    return true;
}

bool rayHitsSphere(float radius, float maxDistance, const Vector3& origin, const Vector3& direction, const Vector3& centre, float& fraction) {
    const float ox = origin[0] - centre[0];
    const float oy = origin[1] - centre[1];
    const float oz = origin[2] - centre[2];
    float c = ox * ox;
    c += oy * oy;
    c += oz * oz;
    c -= radius * radius;
    float b = ox * direction[0];
    b += oy * direction[1];
    b += oz * direction[2];
    if (c > 0.0f && b > 0.0f) return false;
    const float discriminant = -c + b * b;
    if (discriminant < 0.0f) return false;
    float t = -b - std::sqrt(discriminant);
    if (!(t <= maxDistance)) return false;
    if (t <= 0.0f) t = 0.0f;
    fraction = t / maxDistance;
    return true;
}

void putContact(FloorContacts& contacts, float time, const Vector3& point, const Vector3& normal, int cell, float depth) {
    FloorContact contact;
    contact.time = time;
    contact.point = point;
    contact.normal = normal;
    contact.cell = cell;
    contact.depth = depth;
    contacts.items.push_back(contact);
    contacts.earliest = contacts.items.front().time;
}

}

VehicleCollisionSpheres buildVehicleSpheres(const Vector3& low, const Vector3& high, const std::array<float, 3>& counts, float radius,
                                            bool extraSpheres) {
    VehicleCollisionSpheres spheres;
    const float extent[3] = {high[0] - low[0], high[1] - low[1], high[2] - low[2]};
    const float firstX = low[0] + radius;
    const float firstY = low[1] + radius;
    const float lastZ = high[2] - radius;
    float step[3] = {0.0f, 0.0f, 0.0f};
    for (int axis = 0; axis < 3; ++axis)
        if (signedBits(counts[std::size_t(axis)]) > 0x3f800000) {
            float spread = extent[axis];
            spread -= radius * 2.0f;
            step[axis] = spread / (counts[std::size_t(axis)] - 1.0f);
        }
    std::size_t count = std::size_t(std::int32_t(counts[0] * counts[1] * counts[2])) + 1;
    if (extraSpheres) count += 2;
    spheres.centres.assign(count, Vector3{0, 0, 0});
    spheres.radii.assign(count, 0.0f);
    for (unsigned j = 0; counts[1] > float(j); ++j)
        for (unsigned k = 0; counts[2] > float(k); ++k)
            for (unsigned i = 0; counts[0] > float(i); ++i) {
                float index = counts[0] * counts[2];
                index = index * float(j);
                index += float(k) * counts[0];
                index = index + float(i);
                const std::size_t at = std::size_t(std::uint32_t(index));
                if (at >= count) continue;
                spheres.centres[at] = {firstX + step[0] * float(i), firstY + step[1] * float(j), lastZ + (-(step[2] * float(k)))};
                spheres.radii[at] = radius;
            }
    spheres.centres[count - 1] = {0.0f, 0.18f, 0.0f};
    spheres.radii[count - 1] = 0.2f;
    if (extraSpheres) {
        spheres.centres[count - 2] = {0.75f, 0.1f, 1.3f};
        spheres.radii[count - 2] = 0.7f;
        spheres.centres[count - 3] = {-0.75f, 0.1f, 1.3f};
        spheres.radii[count - 3] = 0.7f;
    }
    float lowLength = low[0] * low[0];
    lowLength += low[1] * low[1];
    lowLength += low[2] * low[2];
    float highLength = high[0] * high[0];
    highLength += high[1] * high[1];
    highLength += high[2] * high[2];
    lowLength = std::sqrt(lowLength);
    highLength = std::sqrt(highLength);
    spheres.reach = lowLength <= highLength ? highLength : lowLength;
    return spheres;
}

WallEdgeCells findWallEdgeCells(const FloorMesh& mesh, const Vector3& centre, float reach, int startCell, unsigned edgeBase) {
    WallEdgeCells result;
    result.edges.assign(std::size_t(mesh.cellCount()), 0);
    std::vector<std::uint8_t> marked(std::size_t(mesh.cellCount()), 0);
    std::vector<int> queue{startCell};
    marked[std::size_t(startCell)] = 1;
    for (std::size_t at = 0; at < queue.size(); ++at) {
        const int cell = queue[at];
        const assets::FloorCell& record = mesh.cell(cell);
        for (int edge = 0; edge < 3; ++edge) {
            const std::uint16_t raw = record.neighbour[edge];
            const int neighbour = floorNeighbour(raw);
            if (neighbour < 0) continue;
            const unsigned mask = edgeBase << edge;
            const bool wall = (record.flags & mask) != 0;
            if (!wall && marked[std::size_t(raw)]) continue;
            const FloorPoint a = mesh.vertex(cell, edge);
            const FloorPoint b = mesh.vertex(cell, (edge + 1) % 3);
            float dx = b[0] - a[0];
            float dz = b[2] - a[2];
            float lengthSquared2 = dx * dx + 0.0f;
            lengthSquared2 += dz * dz;
            const float length = std::sqrt(lengthSquared2);
            float zero = 0.0f;
            if (0.0f < length) {
                const float inverse = 1.0f / length;
                dx = dx * inverse;
                zero = inverse * 0.0f;
                dz = dz * inverse;
            }
            const float px = centre[0] - a[0];
            const float pz = centre[2] - a[2];
            if (!(std::fabs(px * dz - pz * dx) < reach)) continue;
            float along = px * dx + zero * 0.0f;
            along += pz * dz;
            float outside = -1.0f;
            if (0.0f <= along) {
                if (length < along) {
                    const float fx = centre[0] - b[0];
                    const float fz = centre[2] - b[2];
                    outside = fx * fx + 0.0f;
                    outside += fz * fz;
                }
            } else {
                outside = px * px + 0.0f;
                outside += pz * pz;
            }
            if (outside >= 0.0f && reach * reach < outside) continue;
            if (wall && (signedBits(a[1] - centre[1]) < signedBits(kWallEdgeHeight) || signedBits(b[1] - centre[1]) < signedBits(kWallEdgeHeight))) {
                result.cells.push_back(int(record.index));
                result.edges[std::size_t(cell)] |= std::uint8_t(1u << edge);
            }
            if (!marked[std::size_t(raw)]) {
                marked[std::size_t(raw)] = 1;
                queue.push_back(int(raw));
            }
        }
    }
    return result;
}

bool sweepSphereAgainstCell(const FloorMesh& mesh, int cell, float radius, float dt, const Vector3& centre, const Vector3& velocity,
                            FloorContacts& contacts) {
    const assets::CollisionTriangle& triangle = mesh.triangle(cell);
    SphereTriangle t;
    t.a = vectorOf(mesh.vertex(cell, 0));
    t.b = vectorOf(mesh.vertex(cell, 1));
    t.c = vectorOf(mesh.vertex(cell, 2));
    t.normal = {triangle.normal[0], triangle.normal[1], triangle.normal[2]};
    t.d = triangle.d;
    const float planeOffset = -t.d;

    float facing = t.normal[0] * velocity[0];
    facing += t.normal[1] * velocity[1];
    facing += t.normal[2] * velocity[2];
    if (facing > 0.0f) return false;

    std::array<int, 2> axes{};
    if (sphereTouchesTriangle(radius, centre, t)) {
        float distance = centre[1] * t.normal[1];
        float offset = planeOffset + centre[0] * t.normal[0];
        distance += centre[2] * t.normal[2];
        distance += offset;
        const Vector3 projection = {centre[0] - t.normal[0] * distance, centre[1] - t.normal[1] * distance, centre[2] - t.normal[2] * distance};
        Vector3 closest = projection;
        if (!projectedInside(t, projection, axes)) {
            if (!closestOnTriangleEdges(t, projection, axes, closest)) return false;
        }
        const float dx = centre[0] - closest[0];
        const float dy = centre[1] - closest[1];
        const float dz = centre[2] - closest[2];
        float lengthSquared2 = dx * dx;
        lengthSquared2 += dy * dy;
        lengthSquared2 += dz * dz;
        const float inverse = 1.0f / std::sqrt(lengthSquared2);
        const Vector3 direction = {dx * inverse, dy * inverse, dz * inverse};
        float approach = direction[0] * velocity[0];
        approach += direction[1] * velocity[1];
        approach += direction[2] * velocity[2];
        if (0.0f <= approach) return false;
        if (0.0f < contacts.earliest) contacts.items.clear();
        const float gx = closest[0] - centre[0];
        const float gy = closest[1] - centre[1];
        const float gz = closest[2] - centre[2];
        float gap = gx * gx;
        gap += gy * gy;
        gap += gz * gz;
        float depth = radius - std::sqrt(gap);
        if (signedBits(depth) > std::int32_t(kSphereDepthLimitBits)) depth = kSphereDepthLimit;
        putContact(contacts, 0.0f, closest, direction, cell, depth);
        return true;
    }

    const float displacement[3] = {velocity[0] * dt, velocity[1] * dt, velocity[2] * dt};
    const Vector3 near = {centre[0] - t.normal[0] * radius, centre[1] - t.normal[1] * radius, centre[2] - t.normal[2] * radius};
    float startDistance = t.normal[1] * near[1];
    startDistance += t.normal[2] * near[2];
    float startOffset = planeOffset;
    startOffset += t.normal[0] * near[0];
    startDistance = startDistance + startOffset;
    const float endY = near[1] + displacement[1];
    const float endX = near[0] + displacement[0];
    const float endZ = near[2] + displacement[2];
    float endDistance = t.normal[1] * endY;
    float endOffset = planeOffset;
    endDistance += t.normal[2] * endZ;
    endOffset += t.normal[0] * endX;
    endDistance = endDistance + endOffset;

    Vector3 edgePoint = near;
    if (startDistance > 0.0f) {
        if (endDistance >= 0.0f) return false;
        const float time = startDistance / (startDistance - endDistance);
        const Vector3 hit = {near[0] + displacement[0] * time, near[1] + displacement[1] * time, near[2] + displacement[2] * time};
        const bool inside = projectedInside(t, hit, axes);
        if (inside && time <= contacts.earliest) {
            if (time < contacts.earliest) contacts.items.clear();
            putContact(contacts, time, hit, t.normal, cell, 0.0f);
            return true;
        }
        if (inside) return false;
        edgePoint = hit;
    } else {
        axes = floorProjectionAxes({t.normal[0], t.normal[1], t.normal[2]});
        const float back = radius + startDistance;
        edgePoint = {centre[0] - t.normal[0] * back, centre[1] - t.normal[1] * back, centre[2] - t.normal[2] * back};
    }

    Vector3 closest;
    if (!closestOnTriangleEdges(t, edgePoint, axes, closest)) return false;
    float rx = (closest[0] - displacement[0]) - closest[0];
    float ry = (closest[1] - displacement[1]) - closest[1];
    float rz = (closest[2] - displacement[2]) - closest[2];
    float lengthSquared2 = rx * rx;
    lengthSquared2 += ry * ry;
    lengthSquared2 += rz * rz;
    const float length = std::sqrt(lengthSquared2);
    if (length > 0.0f) {
        const float inverse = 1.0f / length;
        rx *= inverse;
        ry *= inverse;
        rz *= inverse;
    }
    float fraction = 0.0f;
    if (!rayHitsSphere(radius, length, closest, {rx, ry, rz}, centre, fraction)) return false;
    if (!(fraction <= contacts.earliest)) return false;
    if (fraction < contacts.earliest) contacts.items.clear();
    putContact(contacts, fraction, closest, t.normal, cell, 0.0f);
    return true;
}

void sweepVehicleSpheres(const FloorMesh& mesh, const FloorTracker& tracker, const VehicleRigidBody& body, const VehicleCollisionSpheres& spheres,
                         bool driven, float dt, FloorContacts& contacts) {
    const Vector3& position = body.position;
    const float scaled = 1.0f * dt;
    const Vector3 end = {position[0] + body.velocity[0] * scaled, position[1] + body.velocity[1] * scaled, position[2] + body.velocity[2] * scaled};
    const Vector3 centre = {(position[0] + end[0]) * 0.5f, (position[1] + end[1]) * 0.5f, (position[2] + end[2]) * 0.5f};
    const float dx = end[0] - position[0];
    const float dy = end[1] - position[1];
    const float dz = end[2] - position[2];
    float lengthSquared2 = dx * dx;
    lengthSquared2 += dy * dy;
    lengthSquared2 += dz * dz;
    const float travelled = std::sqrt(lengthSquared2);
    if (signedBits(kSphereReachLimit) <= signedBits(travelled)) return;
    const float reach = spheres.reach + travelled * 0.5f;
    FloorPoint swept = {centre[0], centre[1], centre[2]};
    const int startCell = walkFloor(mesh, tracker.cell, {position[0], position[1], position[2]}, swept, FloorWalkEdge::Stay);
    if (startCell < 0) return;
    const WallEdgeCells walls = findWallEdgeCells(mesh, centre, reach, startCell, driven ? 2u : 16u);
    if (walls.cells.empty()) return;
    std::vector<Vector3> positions(spheres.centres.size());
    std::vector<Vector3> velocities(spheres.centres.size());
    for (std::size_t i = 0; i < spheres.centres.size(); ++i) {
        const Vector3 world = bodyToWorld(body, spheres.centres[i]);
        const Vector3 arm = {world[0] - position[0], world[1] - position[1], world[2] - position[2]};
        const Vector3& w = body.angularVelocity;
        const Vector3 turn = {arm[1] * w[2] - arm[2] * w[1], arm[2] * w[0] - arm[0] * w[2], arm[0] * w[1] - arm[1] * w[0]};
        velocities[i] = {body.velocity[0] - turn[0], body.velocity[1] - turn[1], body.velocity[2] - turn[2]};
        positions[i] = {arm[0] + position[0], arm[1] + position[1], arm[2] + position[2]};
    }
    for (const int cell : walls.cells)
        for (std::size_t i = 0; i < spheres.centres.size(); ++i) {
            sweepSphereAgainstCell(mesh, cell, spheres.radii[i], scaled, positions[i], velocities[i], contacts);
            for (int edge = 0; edge < 3; ++edge) {
                if ((walls.edges[std::size_t(cell)] & (1u << edge)) == 0) continue;
                sweepSphereAgainstCell(mesh, floorNeighbour(mesh.cell(cell).neighbour[edge]), spheres.radii[i], scaled, positions[i], velocities[i],
                                       contacts);
            }
        }
}

}

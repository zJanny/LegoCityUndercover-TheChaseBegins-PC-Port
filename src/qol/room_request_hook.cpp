#include "qol/room_request_hook.hpp"

#include "qol/settings.hpp"
#include "qol/view_distance.hpp"

#include <algorithm>
#include <cmath>

namespace qol {

namespace {

using Vector = std::array<double, 3>;

Vector vectorOf(const RoomPoint& point) { return {point[0], point[1], point[2]}; }
Vector add(const Vector& a, const Vector& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
Vector subtract(const Vector& a, const Vector& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vector scaled(const Vector& a, double s) { return {a[0] * s, a[1] * s, a[2] * s}; }
double dot(const Vector& a, const Vector& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vector cross(const Vector& a, const Vector& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }

Vector unit(const Vector& a) {
    const double length = std::sqrt(dot(a, a));
    return length > 0 ? scaled(a, 1.0 / length) : a;
}

template <std::size_t N, std::size_t M>
bool separated(const Vector& axis, const std::array<Vector, N>& a, const std::array<Vector, M>& b) {
    if (dot(axis, axis) < 1e-12) return false;
    double lowA = dot(axis, a[0]), highA = lowA, lowB = dot(axis, b[0]), highB = lowB;
    for (const Vector& point : a) {
        lowA = std::min(lowA, dot(axis, point));
        highA = std::max(highA, dot(axis, point));
    }
    for (const Vector& point : b) {
        lowB = std::min(lowB, dot(axis, point));
        highB = std::max(highB, dot(axis, point));
    }
    return highA < lowB || highB < lowA;
}

}

bool frustumReaches(const RoomBounds& room, const RoomCamera& camera, float aspect) {
    const Vector eye = vectorOf(camera.eye);
    const Vector forward = unit(vectorOf(camera.forward));
    Vector right = cross(forward, {0, 1, 0});
    right = dot(right, right) < 1e-12 ? Vector{1, 0, 0} : unit(right);
    const Vector up = cross(right, forward);
    const double tangent = std::tan(double(camera.verticalFieldOfView) / 2);
    std::array<Vector, 5> pyramid{eye, eye, eye, eye, eye};
    std::size_t corner = 1;
    for (int vertical = -1; vertical <= 1; vertical += 2)
        for (int horizontal = -1; horizontal <= 1; horizontal += 2) {
            const Vector direction = add(forward, add(scaled(right, horizontal * tangent * aspect), scaled(up, vertical * tangent)));
            pyramid[corner++] = add(eye, scaled(direction, camera.farPlane));
        }
    std::array<Vector, 8> box;
    for (std::size_t i = 0; i < 8; ++i)
        box[i] = {i & 1 ? room.high[0] : room.low[0], i & 2 ? room.high[1] : room.low[1], i & 4 ? room.high[2] : room.low[2]};
    std::vector<Vector> edges;
    for (std::size_t i = 1; i < 5; ++i) edges.push_back(subtract(pyramid[i], eye));
    edges.push_back(subtract(pyramid[2], pyramid[1]));
    edges.push_back(subtract(pyramid[3], pyramid[1]));
    std::vector<Vector> axes = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, cross(edges[4], edges[5]), cross(edges[0], edges[1]),
                                cross(edges[0], edges[2]), cross(edges[3], edges[1]), cross(edges[3], edges[2])};
    for (const Vector& edge : edges)
        for (const Vector& boxEdge : {Vector{1, 0, 0}, Vector{0, 1, 0}, Vector{0, 0, 1}}) axes.push_back(cross(boxEdge, edge));
    for (const Vector& axis : axes)
        if (separated(axis, pyramid, box)) return false;
    return true;
}

float nearestDistance(const RoomBounds& room, const RoomPoint& eye) {
    double sum = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const double nearest = std::clamp(double(eye[axis]), double(room.low[axis]), double(room.high[axis]));
        sum += (eye[axis] - nearest) * (eye[axis] - nearest);
    }
    return float(std::sqrt(sum));
}

bool roomRequestsActive(const Settings& settings) {
    return settings.enabled(Feature::WidenedView) || effectiveViewDistanceStep(settings) > 0;
}

std::set<int> viewRooms(const std::vector<RoomBounds>& rooms, const RoomCamera& camera, const Settings& settings) {
    std::set<int> reached;
    if (!roomRequestsActive(settings)) return reached;
    const bool widened = settings.enabled(Feature::WidenedView);
    const bool scaled = effectiveViewDistanceStep(settings) > 0;
    RoomCamera range = camera;
    range.farPlane = camera.farPlane * effectiveViewDistanceFactor(settings);
    for (const RoomBounds& room : rooms) {
        const bool inRange = scaled && nearestDistance(room, camera.eye) <= range.farPlane;
        const bool inStrip = widened && frustumReaches(room, range, widenedViewAspect) && !frustumReaches(room, range, originalViewAspect);
        if (inRange || inStrip) reached.insert(room.room);
    }
    return reached;
}

std::set<int> withViewRooms(std::set<int> requested, const std::vector<RoomBounds>& rooms, const RoomCamera& camera) {
    const std::set<int> reached = viewRooms(rooms, camera, activeSettings());
    requested.insert(reached.begin(), reached.end());
    return requested;
}

}

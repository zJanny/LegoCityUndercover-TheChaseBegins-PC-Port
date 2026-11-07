#pragma once

#include "qol/active_settings.hpp"

#include <array>
#include <set>
#include <utility>
#include <vector>

namespace qol {

using RoomPoint = std::array<float, 3>;

struct RoomBounds {
    int room = 0;
    RoomPoint low{}, high{};
};

struct RoomCamera {
    RoomPoint eye{};
    RoomPoint forward{0, 0, 1};
    float verticalFieldOfView = 0;
    float farPlane = 0;
};

inline constexpr float originalViewAspect = 5.0f / 3.0f;
inline constexpr float widenedViewAspect = 16.0f / 9.0f;

bool frustumReaches(const RoomBounds& room, const RoomCamera& camera, float aspect);
float nearestDistance(const RoomBounds& room, const RoomPoint& eye);

bool roomRequestsActive(const Settings& settings);
std::set<int> viewRooms(const std::vector<RoomBounds>& rooms, const RoomCamera& camera, const Settings& settings);
std::set<int> withViewRooms(std::set<int> requested, const std::vector<RoomBounds>& rooms, const RoomCamera& camera);

template <class Boxes>
std::set<int> withViewRooms(std::set<int> requested, const Boxes& boxes, const float* eye, const RoomPoint& forward, float verticalFieldOfView,
                            float farPlane) {
    if (!roomRequestsActive(activeSettings())) return requested;
    std::vector<RoomBounds> rooms;
    rooms.reserve(boxes.size());
    for (const auto& [room, box] : boxes) rooms.push_back({room, box.low, box.high});
    return withViewRooms(std::move(requested), rooms, {{eye[0], eye[1], eye[2]}, forward, verticalFieldOfView, farPlane});
}

}

// The room request hook (src/qol/room_request_hook.*): rooms the widened view and the View distance slider add to the room request.
// Spec: docs/design/qol-view-distance.md entry 2026-11-02T18:43:07 ("No cap, measure"; room rule "Radial (Recommended)", the user,
// 2026-11-03: above x1.0 every room whose box comes within 260 m x factor of the camera) and docs/design/qol-screen-layout.md entry
// 2026-11-02T15:42:07 ("Yes, load them"; "Strip only (Recommended)", the user, 2026-11-03: rooms the 16:9 frustum reaches and the 5:3
// frustum does not, up to the far plane). The far plane 260 is kFollowFarPlane (src/runtime/clip_planes.hpp). Rooms, boxes and the
// camera are synthetic fixtures; every expected value is computed by hand in the comment beside it. Cases written by 20261103-large-E
// on 2026-11-03.
#include "qol/active_settings.hpp"
#include "qol/room_request_hook.hpp"
#include "qol/settings.hpp"
#include "runtime/clip_planes.hpp"
#include "runtime/room_streamer.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

namespace rt = chase::runtime;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

constexpr int stepX2 = 4;
constexpr int stepX20 = 28;
const float verticalFieldOfView = 2 * std::atan(0.25f);

qol::Settings original() {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, true);
    settings.setViewDistanceStep(stepX20);
    settings.setMaster(qol::Master::Original);
    return settings;
}

qol::Settings custom(bool widened, int step) {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, widened);
    settings.setViewDistanceStep(step);
    settings.setMaster(qol::Master::Custom);
    return settings;
}

qol::Settings improved(int step) {
    qol::Settings settings;
    settings.setViewDistanceStep(step);
    settings.setMaster(qol::Master::Improved);
    return settings;
}

qol::RoomBounds box(int room, float x, float z, float half) { return {room, {x - half, 0, z - half}, {x + half, 2, z + half}}; }

qol::RoomCamera camera(float farPlane) { return {{0, 1, 0}, {0, 0, 1}, verticalFieldOfView, farPlane}; }

// tan(vertical / 2) = 0.25: at depth 100 the 5:3 view reaches x = 41.67 and the 16:9 view x = 44.44.
std::vector<qol::RoomBounds> stripFixture() {
    return {box(1, 0, 100, 0.4f), box(2, 43, 100, 0.4f), box(3, -43, 100, 0.4f), box(4, 50, 100, 0.4f), box(5, 0, 300, 0.4f),
            box(6, 0, -100, 0.4f)};
}

void geometry() {
    const qol::RoomCamera view = camera(rt::kFollowFarPlane);
    require(qol::frustumReaches(box(1, 0, 100, 0.4f), view, qol::originalViewAspect), "a box in the middle is in the 5:3 view");
    require(!qol::frustumReaches(box(2, 43, 100, 0.4f), view, qol::originalViewAspect), "x 42.6 at depth 100 is beyond the 5:3 edge 41.67");
    require(qol::frustumReaches(box(2, 43, 100, 0.4f), view, qol::widenedViewAspect), "x 42.6 at depth 100 is inside the 16:9 edge 44.44");
    require(!qol::frustumReaches(box(4, 50, 100, 0.4f), view, qol::widenedViewAspect), "x 49.6 at depth 100 is beyond the 16:9 edge");
    require(!qol::frustumReaches(box(5, 0, 300, 0.4f), view, qol::widenedViewAspect), "depth 299.6 is beyond the far plane 260");
    require(!qol::frustumReaches(box(6, 0, -100, 0.4f), view, qol::widenedViewAspect), "a box behind the camera is not in view");
    require(std::abs(qol::nearestDistance(box(6, 0, -100, 0.4f), {0, 1, 0}) - 99.6f) < 1e-3f, "nearest box point 99.6 behind the camera");
    require(qol::nearestDistance(box(1, 0, 0, 0.4f), {0, 1, 0}) == 0, "an eye inside the box is at distance 0");
}

void originalModeFirst() {
    const std::vector<qol::RoomBounds> rooms = stripFixture();
    for (const qol::Settings& settings : {original(), custom(false, 0), qol::Settings{}}) {
        require(!qol::roomRequestsActive(settings), "Original and Custom without the toggles add no rooms");
        require(qol::viewRooms(rooms, camera(rt::kFollowFarPlane), settings).empty(), "no rooms in Original or with both toggles off");
        qol::setActiveSettings(settings);
        require(qol::withViewRooms({7, 9}, rooms, camera(rt::kFollowFarPlane)) == std::set<int>{7, 9}, "the scripted request passes unchanged");
    }
}

void stripRooms() {
    const std::vector<qol::RoomBounds> rooms = stripFixture();
    require(qol::viewRooms(rooms, camera(rt::kFollowFarPlane), custom(true, 0)) == std::set<int>{2, 3},
            "widened view at x1.0: only the two strip rooms (not the middle, outside or far rooms)");
    require(qol::viewRooms(rooms, camera(rt::kFollowFarPlane), improved(0)) == std::set<int>{2, 3}, "Improved at x1.0: the strip rooms");
    const std::vector<qol::RoomBounds> far = {box(1, 43 * 4, 400, 0.4f)};
    require(qol::viewRooms(far, camera(rt::kFollowFarPlane), custom(true, 0)).empty(), "a strip room at depth 400 is beyond the far plane 260");
    require(qol::viewRooms(far, camera(rt::kFollowFarPlane), custom(true, stepX2)) == std::set<int>{1},
            "the strip reaches to the scaled far plane 520 (and the radial rule covers it as well)");
}

void radialRooms() {
    const std::vector<qol::RoomBounds> rooms = stripFixture();
    require(qol::viewRooms(rooms, camera(rt::kFollowFarPlane), custom(false, stepX2)) == std::set<int>{1, 2, 3, 4, 5, 6},
            "x2.0 (520 m): every fixture room, behind and beside the camera too");
    const std::vector<qol::RoomBounds> ring = {box(1, 0, -519, 0.5f), box(2, 0, -521, 0.5f), box(3, 0, -5199, 0.5f), box(4, 0, -5201, 0.5f)};
    require(qol::viewRooms(ring, camera(rt::kFollowFarPlane), custom(false, stepX2)) == std::set<int>{1},
            "x2.0 (520 m): nearest box points 518.5 in, 520.5 out");
    require(qol::viewRooms(ring, camera(rt::kFollowFarPlane), improved(stepX20)) == std::set<int>{1, 2, 3},
            "x20 (5200 m): nearest points 5198.5 in, 5200.5 out");
}

struct Street {
    rt::StreamRooms rooms;
    std::map<int, rt::RoomBox> boxes;
};

// Ten 100 m tiles along z, room n from z = (n - 1) * 100 to n * 100, each the neighbour of the next.
Street street() {
    Street result;
    for (int n = 1; n <= 10; ++n) {
        rt::RoomBox room;
        room.low = {-50, 0, float(n - 1) * 100};
        room.high = {50, 10, float(n) * 100};
        result.boxes[n] = room;
        rt::StreamRoom stream;
        stream.position = room.centre();
        if (n > 1) stream.neighbours.push_back(n - 1);
        if (n < 10) stream.neighbours.push_back(n + 1);
        result.rooms[n] = stream;
    }
    return result;
}

std::set<int> afterLoad(const qol::Settings& settings, std::vector<std::set<int>>* history = nullptr) {
    qol::setActiveSettings(settings);
    const Street fixture = street();
    rt::RoomStreamer streamer(fixture.rooms, fixture.boxes);
    const rt::Vec3 player = {0, 1, 50}, forward = {0, 0, 1};
    const float eye[3] = {0, 3, 40};
    streamer.settle(1, player, forward);
    for (unsigned update = 0; update <= rt::kRoomLoadUpdates; ++update) {
        streamer.update(player, forward, qol::withViewRooms(std::set<int>{}, streamer.boxes(), eye, forward, verticalFieldOfView, rt::kFollowFarPlane));
        if (history) history->push_back(streamer.loadedRooms());
    }
    return streamer.loadedRooms();
}

void streamerRequests() {
    std::vector<std::set<int>> plain;
    {
        const Street fixture = street();
        rt::RoomStreamer streamer(fixture.rooms, fixture.boxes);
        streamer.settle(1, {0, 1, 50}, {0, 0, 1});
        for (unsigned update = 0; update <= rt::kRoomLoadUpdates; ++update) {
            streamer.update({0, 1, 50}, {0, 0, 1});
            plain.push_back(streamer.loadedRooms());
        }
    }
    require(plain.back() == std::set<int>{1, 2, 3}, "the original request: current room 1, neighbour 2 and the preload 3");
    std::vector<std::set<int>> hooked;
    afterLoad(original(), &hooked);
    require(hooked == plain, "Original: the streamer loads exactly the original rooms at every update");
    require(afterLoad(custom(false, 0)) == plain.back(), "Custom with both toggles off: the original rooms");
    require(afterLoad(custom(false, stepX2)) == std::set<int>{1, 2, 3, 4, 5, 6}, "x2.0 from z 40: tiles starting at most 520 m away (1 to 6)");
    require(afterLoad(improved(stepX20)) == std::set<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, "x20: every tile, no cap");
}

}

int main() {
    geometry();
    originalModeFirst();
    stripRooms();
    radialRooms();
    streamerRequests();
    qol::setActiveSettings(qol::Settings{});
    if (failures) return 1;
    std::cout << "room request hook: ok\n";
    return 0;
}

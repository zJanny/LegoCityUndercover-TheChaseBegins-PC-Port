// Research of the widened 16:9 view against the level data: the largest camera record FOV (QOLFS-16) and the rooms that only the
// 16:9 frustum reaches (QOLFS-17). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
// Method and results: docs/re/view-edges.md. Measured by 20261102-qolctl2-E on 2026-11-02 over the shipped level data.
#include "assets/assets.hpp"
#include "runtime/camera_records.hpp"
#include "runtime/room_streamer.hpp"
#include "runtime/scene_change.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase;
using namespace chase::runtime;

namespace {
using V3 = std::array<double, 3>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

V3 add(const V3& a, const V3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 sub(const V3& a, const V3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3 scale(const V3& a, double s) { return {a[0] * s, a[1] * s, a[2] * s}; }
double dot(const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 cross(const V3& a, const V3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
V3 unit(const V3& a) { return scale(a, 1.0 / std::sqrt(dot(a, a))); }

constexpr double kPi = 3.14159265358979323846;
constexpr double kVerticalTangentPerFovTangent = 0.75;
constexpr double kSpawnConeCosine = 0.8;
constexpr double kFarPlane = 260.0;
constexpr double kAspect53 = 5.0 / 3.0;
constexpr double kAspect169 = 16.0 / 9.0;

double halfTangent(double poseFovDegrees) { return kVerticalTangentPerFovTangent * std::tan(poseFovDegrees * kPi / 360.0); }

double largestFovInsideSpawnCone(double aspect) {
    const double coneTangent = std::sqrt(1.0 - kSpawnConeCosine * kSpawnConeCosine) / kSpawnConeCosine;
    const double vertical = coneTangent / std::sqrt(1.0 + aspect * aspect);
    return 2.0 * std::atan(vertical / kVerticalTangentPerFovTangent) * 180.0 / kPi;
}

struct Frustum {
    std::array<V3, 5> corners;
};

Frustum makeFrustum(const V3& eye, double yaw, double pitch, double tangent, double aspect) {
    const V3 forward = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
    const V3 right = unit(cross(forward, {0, 1, 0}));
    const V3 up = cross(right, forward);
    Frustum frustum;
    frustum.corners[0] = eye;
    int index = 1;
    for (int sy = -1; sy <= 1; sy += 2)
        for (int sx = -1; sx <= 1; sx += 2) {
            const V3 direction = add(forward, add(scale(right, sx * tangent * aspect), scale(up, sy * tangent)));
            frustum.corners[std::size_t(index++)] = add(eye, scale(direction, kFarPlane));
        }
    return frustum;
}

bool separatedOn(const V3& axis, const std::array<V3, 5>& frustum, const std::array<V3, 8>& box) {
    if (dot(axis, axis) < 1e-12) return false;
    double lowA = 1e300, highA = -1e300, lowB = 1e300, highB = -1e300;
    for (const V3& p : frustum) {
        lowA = std::min(lowA, dot(axis, p));
        highA = std::max(highA, dot(axis, p));
    }
    for (const V3& p : box) {
        lowB = std::min(lowB, dot(axis, p));
        highB = std::max(highB, dot(axis, p));
    }
    return highA < lowB || highB < lowA;
}

bool frustumReaches(const Frustum& frustum, const RoomBox& room) {
    std::array<V3, 8> box;
    for (int i = 0; i < 8; ++i)
        box[std::size_t(i)] = {double(i & 1 ? room.high[0] : room.low[0]), double(i & 2 ? room.high[1] : room.low[1]),
                               double(i & 4 ? room.high[2] : room.low[2])};
    std::vector<V3> axes = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    std::vector<V3> edges;
    for (std::size_t i = 1; i < 5; ++i) edges.push_back(sub(frustum.corners[i], frustum.corners[0]));
    edges.push_back(sub(frustum.corners[2], frustum.corners[1]));
    edges.push_back(sub(frustum.corners[3], frustum.corners[1]));
    const V3 farNormal = cross(edges[4], edges[5]);
    axes.push_back(farNormal);
    axes.push_back(cross(edges[0], edges[1]));
    axes.push_back(cross(edges[0], edges[2]));
    axes.push_back(cross(edges[3], edges[1]));
    axes.push_back(cross(edges[3], edges[2]));
    for (const V3& edge : edges)
        for (int axis = 0; axis < 3; ++axis) {
            V3 unitAxis = {0, 0, 0};
            unitAxis[std::size_t(axis)] = 1;
            axes.push_back(cross(unitAxis, edge));
        }
    for (const V3& axis : axes)
        if (separatedOn(axis, frustum.corners, box)) return false;
    return true;
}

struct CameraFovScan {
    std::size_t levels = 0, levelsWithCam = 0, records = 0;
    int maxRecordFov = 0;
    std::string maxRecordLevel, maxRecordName;
    std::size_t recordsAboveCone = 0;
    std::map<std::string, std::map<int, std::size_t>> setupFovByKind;
    std::vector<std::string> setupsNotFortyThree;
};

CameraFovScan scanCameraFov(assets::FuseArchive& archive) {
    CameraFovScan result;
    for (const std::string& name : archive.names()) {
        if (name.rfind("levels/levels/", 0) != 0 || name.size() < 6 || name.compare(name.size() - 5, 5, ".blvl") != 0) continue;
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(name)));
        std::string text;
        if (const assets::FuseEntry* cam = archive.find(name.substr(0, name.size() - 5) + ".cam")) {
            const assets::Bytes bytes = archive.read(*cam);
            text.assign(bytes.begin(), bytes.end());
            ++result.levelsWithCam;
            for (const assets::CameraSetup& setup : assets::parseCameraFollow(text).setups) {
                ++result.setupFovByKind[setup.kind][int(setup.fov)];
                if (int(setup.fov) != 43) result.setupsNotFortyThree.push_back(setup.kind + " " + setup.name + " FOV " + std::to_string(int(setup.fov)));
            }
        }
        const CameraRecords records(level, text);
        ++result.levels;
        result.records += records.records().size();
        for (const CameraRecord& record : records.records()) {
            if (record.fov > result.maxRecordFov) {
                result.maxRecordFov = record.fov;
                result.maxRecordLevel = name;
                result.maxRecordName = record.name;
            }
            if (record.fov > largestFovInsideSpawnCone(kAspect169)) ++result.recordsAboveCone;
        }
    }
    return result;
}

constexpr double kFollowDistance = 10.0;
constexpr double kFollowEyeHeight = 3.65;
constexpr double kFollowLookHeight = 1.95;
constexpr double kFollowPoseFov = 40.0;

struct RoomParts {
    RoomBox box;
    std::vector<RoomBox> parts;
};

std::map<int, RoomParts> loadRoomParts(assets::FuseArchive& archive, const std::string& folder) {
    std::map<int, RoomParts> rooms;
    for (int number = 1; number <= 200; ++number) {
        const assets::FuseEntry* entry = archive.find(folder + "/room" + std::to_string(number) + ".bfnmdl");
        if (!entry) continue;
        const assets::Model model = assets::parseModel(archive.read(*entry));
        RoomParts room;
        for (std::size_t mesh = 1; mesh < model.meshes.size(); ++mesh)
            for (const assets::ModelPart& part : model.meshes[mesh].parts) {
                if (part.vertices.empty()) continue;
                RoomBox box;
                for (int axis = 0; axis < 3; ++axis) {
                    box.low[std::size_t(axis)] = part.centre[axis] - part.half[axis];
                    box.high[std::size_t(axis)] = part.centre[axis] + part.half[axis];
                }
                room.parts.push_back(box);
            }
        if (const std::optional<RoomBox> whole = roomBoxOf(model)) {
            room.box = *whole;
            rooms[number] = std::move(room);
        }
    }
    return rooms;
}

struct Reach {
    bool reached = false;
    double depth = 0;
};

Frustum shortened(const Frustum& frustum, double depth) {
    Frustum result = frustum;
    for (std::size_t i = 1; i < 5; ++i) result.corners[i] = add(frustum.corners[0], scale(sub(frustum.corners[i], frustum.corners[0]), depth / kFarPlane));
    return result;
}

Reach reach(const V3& eye, double yaw, double pitch, double tangent, double aspect, const RoomParts& room) {
    const Frustum frustum = makeFrustum(eye, yaw, pitch, tangent, aspect);
    if (!frustumReaches(frustum, room.box)) return {false, 0};
    double nearest = 1e30;
    for (const RoomBox& part : room.parts) {
        if (!frustumReaches(frustum, part)) continue;
        double low = 0, high = kFarPlane;
        for (int step = 0; step < 14; ++step) {
            const double middle = (low + high) * 0.5;
            if (frustumReaches(shortened(frustum, middle), part)) high = middle;
            else low = middle;
        }
        nearest = std::min(nearest, high);
    }
    if (nearest > 1e29) return {false, 0};
    return {true, nearest};
}

struct LevelEdges {
    std::string instance;
    std::size_t positions = 0, samples = 0;
    std::size_t samplesStripOnly = 0, samplesStripOnlyUnloaded = 0, samplesVisible53Unloaded = 0;
    std::set<int> stripOnlyUnloadedRooms;
    std::set<std::pair<int, int>> pairs;
    double nearestStrip = 1e30, nearest53 = 1e30;
    int nearestCurrent = 0, nearestRoom = 0;
    std::array<std::size_t, 5> depthBins{};
};

LevelEdges scanLevelEdges(const assets::Level& master, assets::FuseArchive& archive, const WorldLevelEntry& world, double step, int yaws,
                          double lift) {
    LevelEdges result;
    result.instance = world.instance;
    std::string folder = world.levelFile.substr(std::string("levels/levels/").size());
    folder = "models/" + folder.substr(0, folder.find('/'));
    const std::map<int, RoomParts> parts = loadRoomParts(archive, folder);
    std::map<int, RoomBox> boxes;
    for (const auto& [number, room] : parts) boxes[number] = room.box;
    StreamRooms rooms = streamRoomsOf(master, world.instance, boxes);
    if (rooms.empty()) return result;
    for (const auto& [number, room] : rooms) {
        if (!boxes.count(number)) return result;
        for (int neighbour : room.neighbours)
            if (!rooms.count(neighbour)) return result;
    }
    RoomStreamer streamer(std::move(rooms), boxes);
    double minX = 1e30, maxX = -1e30, minZ = 1e30, maxZ = -1e30;
    for (const auto& [number, box] : boxes) {
        minX = std::min<double>(minX, box.low[0]);
        maxX = std::max<double>(maxX, box.high[0]);
        minZ = std::min<double>(minZ, box.low[2]);
        maxZ = std::max<double>(maxZ, box.high[2]);
    }
    const double tangent = halfTangent(kFollowPoseFov);
    const double pitch = -std::atan((kFollowEyeHeight - kFollowLookHeight) / kFollowDistance);
    for (double x = minX + step * 0.5; x < maxX; x += step)
        for (double z = minZ + step * 0.5; z < maxZ; z += step) {
            const int roomNumber = streamer.roomAt({float(x), 0, float(z)});
            const auto found = boxes.find(roomNumber);
            if (found == boxes.end()) continue;
            const RoomBox& own = found->second;
            if (x < own.low[0] || x > own.high[0] || z < own.low[2] || z > own.high[2]) continue;
            ++result.positions;
            const V3 player = {x, double(own.low[1]) + lift, z};
            for (int k = 0; k < yaws; ++k) {
                const double yaw = 2.0 * kPi * k / yaws;
                const V3 forward = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
                streamer.settle(roomNumber, {float(player[0]), float(player[1]), float(player[2])},
                                {float(forward[0]), float(forward[1]), float(forward[2])});
                const V3 eye = {player[0] - std::sin(yaw) * kFollowDistance, player[1] + kFollowEyeHeight - kFollowLookHeight,
                                player[2] - std::cos(yaw) * kFollowDistance};
                ++result.samples;
                bool stripOnly = false, stripOnlyUnloaded = false, visible53Unloaded = false;
                for (const auto& [number, room] : parts) {
                    const bool loaded = streamer.loaded(number);
                    const Reach narrow = reach(eye, yaw, pitch, tangent, kAspect53, room);
                    if (narrow.reached) {
                        if (!loaded) {
                            visible53Unloaded = true;
                            result.nearest53 = std::min(result.nearest53, narrow.depth);
                        }
                        continue;
                    }
                    const Reach wide = reach(eye, yaw, pitch, tangent, kAspect169, room);
                    if (!wide.reached) continue;
                    stripOnly = true;
                    if (loaded) continue;
                    stripOnlyUnloaded = true;
                    result.stripOnlyUnloadedRooms.insert(number);
                    result.pairs.insert({roomNumber, number});
                    ++result.depthBins[std::min<std::size_t>(4, std::size_t(wide.depth / 52.0))];
                    if (wide.depth < result.nearestStrip) {
                        result.nearestStrip = wide.depth;
                        result.nearestCurrent = roomNumber;
                        result.nearestRoom = number;
                    }
                }
                result.samplesStripOnly += stripOnly;
                result.samplesStripOnlyUnloaded += stripOnlyUnloaded;
                result.samplesVisible53Unloaded += visible53Unloaded;
            }
        }
    return result;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-view-edges-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-view-edges-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);

        require(std::fabs(largestFovInsideSpawnCone(kAspect169) - 52.23) < 0.01, "16:9 spawn cone limit");
        require(std::fabs(largestFovInsideSpawnCone(kAspect53) - 54.45) < 0.01, "5:3 spawn cone limit");

        // QOLFS-16. Provenance: 20261102-qolctl2-E, 2026-11-02, scan of every levels/levels/**/*.blvl and its .cam file of the shipped RomFS
        // with CameraRecords (FOV byte +0x49 of each record, default 43 for a bound without a setup).
        const CameraFovScan fov = scanCameraFov(archive);
        require(fov.levels == 30 && fov.levelsWithCam == 10, "30 level files, 10 with a .cam file");
        require(fov.maxRecordFov == 43, "the largest record FOV of all levels is 43, found " + std::to_string(fov.maxRecordFov));
        require(fov.recordsAboveCone == 0, "no record FOV above the 16:9 spawn cone limit");
        const std::map<int, std::size_t> bound = {{43, 1822}};
        const std::map<int, std::size_t> roomSetups = {{0, 2}, {43, 251}};
        require(fov.setupFovByKind.size() == 2 && fov.setupFovByKind.at("Bound") == bound && fov.setupFovByKind.at("Room") == roomSetups,
                "the .cam setups: 1822 Bound blocks of FOV 43, 253 Room blocks (251 of FOV 43, 2 of FOV 0)");
        require(fov.setupsNotFortyThree == std::vector<std::string>({"Room room13 FOV 0", "Room room38 FOV 0"}), "the two Room blocks of FOV 0");

        // QOLFS-17. Provenance: 20261102-qolctl2-E, 2026-11-02, the follow camera at pose FOV 40, 10 m behind the player and 1.7 m above
        // the look point (docs/re/camera-follow.md), pitch from that offset, far plane 260 (docs/re/rendering-fog.md); players on a 50 m grid
        // of every room box (smallest box containing the point) 5 m above the box floor, 12 yaws; the loaded set is the settled
        // RoomStreamer set for that view; a room counts as reached when a part box of its model meets the frustum.
        const struct {
            const char* instance;
            std::size_t samples, stripOnlyUnloaded;
            double nearestStrip, nearest53;
        } expected[] = {
            {"WorldLevel_00_PoliceStation", 1572, 10, 144.489, 0.016},
            {"WorldLevel_01_CherryTreeHills", 6972, 295, 108.164, 54.082},
            {"WorldLevel_02_Albatross Island", 864, 20, 115.305, 22.645},
            {"WorldLevel_03_Auburn", 3996, 470, 117.035, 0.016},
            {"WorldLevel_04_FortMeadows", 6396, 202, 47.766, 44.941},
            {"WorldLevel_05_BlueBellWood", 3276, 106, 120.542, 100.055},
            {"WorldLevel_06_ApolloIsland", 2496, 227, 111.084, 30.024},
            {"WorldLevel_07_Downtown", 3156, 427, 44.815, 32.5},
        };
        const assets::Level city = assets::parseLevel(archive.read(*archive.find(kMasterLevelFile)));
        const std::vector<WorldLevelEntry> worlds = worldLevels(city);
        require(worlds.size() == std::size(expected), "eight world levels");
        for (std::size_t i = 0; i < worlds.size(); ++i) {
            require(worlds[i].instance == expected[i].instance, "world level order: " + worlds[i].instance);
            const LevelEdges edges = scanLevelEdges(city, archive, worlds[i], 50.0, 12, 5.0);
            std::cout << "edges " << edges.instance << " samples " << edges.samples << " stripOnly " << edges.samplesStripOnly
                      << " stripOnlyUnloaded " << edges.samplesStripOnlyUnloaded << " visible53Unloaded " << edges.samplesVisible53Unloaded
                      << " rooms " << edges.stripOnlyUnloadedRooms.size() << " pairs " << edges.pairs.size() << " nearestStrip "
                      << edges.nearestStrip << " (room " << edges.nearestCurrent << " to " << edges.nearestRoom << ") nearest53 " << edges.nearest53
                      << " depth bins of 52 m:";
            for (std::size_t bin : edges.depthBins) std::cout << ' ' << bin;
            std::cout << "\n";
            require(edges.samples == expected[i].samples, edges.instance + ": samples " + std::to_string(edges.samples));
            require(edges.samplesStripOnlyUnloaded == expected[i].stripOnlyUnloaded,
                    edges.instance + ": samples with a strip-only unloaded room " + std::to_string(edges.samplesStripOnlyUnloaded));
            require(std::fabs(edges.nearestStrip - expected[i].nearestStrip) < 0.5, edges.instance + ": nearest strip-only depth " + std::to_string(edges.nearestStrip));
            require(std::fabs(edges.nearest53 - expected[i].nearest53) < 0.5, edges.instance + ": nearest unloaded depth at 5:3 " + std::to_string(edges.nearest53));
            require(edges.nearest53 < edges.nearestStrip,
                    edges.instance + ": the 5:3 view reaches an unloaded room nearer than any strip-only room");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

// The nav path finder and follower on the real nav grids of lvl_01_cherrytreehills (src/runtime/nav_path.cpp,
// nav_follow.cpp). The police station entry (docs/re/scripts.md: the player walks from (904.652, 16.577, 1113.197) to
// Enter_Police_Station_Position_End) is a hand-checked case; the rest are invariants of the transcription over many pairs of
// positions: no crash or hang, the chain of the cell nodes leads to the exit cell, a same-grid path is followed to its end.
// No value is compared with a run of the original. Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/ai.hpp"
#include "runtime/nav_follow.hpp"
#include "runtime/nav_grid.hpp"
#include "runtime/nav_load.hpp"
#include "runtime/nav_path.hpp"

#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

NavPathRecord plainAiMove() {
    NavPathRecord record;
    record.avoidMask = record.gateMask = kAiNavGateMask;
    record.heightTolerance = 0.5f;
    return record;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: nav-path-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "nav-path-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const std::string levelPath = kLevelFile;
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(levelPath)));
        std::vector<NavWorld> worlds = loadNavWorlds(archive, levelPath, level);
        for (NavWorld& world : worlds) world.now = kAiNavClockStart;
        require(worlds.size() == 1 && worlds[0].rooms.size() == 43 && navTotalGrids(worlds[0]) == 1588, "one section of 43 rooms and 1588 grids");

        // Police station entry. Grid 26: x 2527, z 1522, width 12, depth 13, flags 0x4000 (a restricted grid, open because
        // the mask has bit 0). Corner x = -967.413 + 2527 * 0.739895 = 902.3, z = -15.1678 + 1522 * 0.739895 = 1110.8; the
        // start cell is (3, 3) = 3 * 12 + 3 = 39, the goal (11, 11) = 143.
        const auto goalPlacement = assets::objectStart(level, "enter_police_station_position_end");
        require(goalPlacement.has_value(), "the goal object");
        NavPathRecord record = plainAiMove();
        const float start[3] = {904.652f, 16.577f, 1113.197f};
        const float goal[3] = {goalPlacement->position[0], goalPlacement->position[1], goalPlacement->position[2]};
        require(navFindPath(worlds[0], record, start, goal) == 3 && record.phase == 2, "police entry: a path within the grid");
        require(record.startGrid == 26 && record.goalGrid == 26 && record.startCell == 39 && record.goalCell == 143 && record.exitCell == 143,
                "grid 26, cells 39 and 143 (hand-computed above)");
        // Oracle (Azahar, save state at the police station entrance with its question box open, A pressed, GDB reads of the path record 0x58e330 and its cell nodes at
        // update 4 after the finder; docs/re/ai-navigation.md): phase 2, 37 open cells, the chain 39 -> 52 -> 65 -> ... -> 143
        // with costs 24, 21, 18, ..., 0 and mark 143, cell 40 cost 23 parent 52, cell 155 cost 2 parent 143.
        require(record.cellOpen.size() == 37, "37 open cells as in the original");
        const auto& cn = record.cellNodes;
        require(cn[39].cost == 24 && cn[39].parent == 52 && cn[39].mark == 143 && cn[52].cost == 21 && cn[65].parent == 78 &&
                    cn[130].cost == 3 && cn[130].parent == 143 && cn[143].cost == 0 && cn[143].parent == 143,
                "the chain and costs of the original");
        require(cn[40].cost == 23 && cn[40].parent == 52 && cn[155].cost == 2 && cn[155].parent == 143 && cn[26].cost == 27 && cn[26].parent == 39,
                "neighbour costs of the original");
        const NavFollowResult first = navFollow(worlds[0], record, start);
        require(first.code == 4 && std::fabs(first.target[0] - 910.81f) < 0.01f && std::fabs(first.target[2] - 1119.46f) < 0.01f,
                "the first waypoint is the centre of cell 143: 902.3 + 11.5 * 0.739895, 1110.8 + 11.5 * 0.739895");

        std::uint32_t seed = 12345;
        const auto next = [&] {
            seed = seed * 1664525u + 1013904223u;
            return seed >> 8;
        };
        std::vector<std::array<float, 3>> points;
        for (const auto& object : level.objects)
            if (const auto placement = assets::objectStart(level, object.instance))
                points.push_back({placement->position[0], placement->position[1], placement->position[2]});
        require(points.size() > 1000, "object positions");
        std::size_t paths = 0, followed = 0, arrived = 0, crossFollowed = 0, crossArrived = 0;
        // Standing height above the grid base: the player at 16.577 over the first waypoint 16.48 (oracle read, police entry).
        constexpr float kStandingHeight = 0.097f;
        for (int n = 0; n < 3000; ++n) {
            const auto& a = points[next() % points.size()];
            std::array<float, 3> b = a;
            b[0] += static_cast<float>(static_cast<int>(next() % 4001) - 2000) / 100.0f;
            b[2] += static_cast<float>(static_cast<int>(next() % 4001) - 2000) / 100.0f;
            worlds[0].now += 5.0f;
            NavPathRecord pair = plainAiMove();
            const int code = navFindPath(worlds[0], pair, a.data(), b.data());
            require(code >= 0 && code <= 3, "result code");
            if (code == 0) require(pair.startGrid == pair.goalGrid && pair.startCell == pair.goalCell && pair.phase == 0, "code 0 is the same cell");
            if (code != 3 || pair.phase != 2) continue;
            ++paths;
            require(pair.exitCell < kNavMaxCellNodes && pair.cellNodes[pair.startCell].state == 3, "a closed start cell node and an exit cell");
            int cell = pair.startCell;
            int steps = 0;
            while (cell != pair.exitCell && steps++ < 300) cell = pair.cellNodes[std::size_t(cell)].parent;
            require(cell == pair.exitCell, "the parent chain of the start cell reaches the exit cell");
            const bool across = pair.startGrid != pair.goalGrid;
            ++(across ? crossFollowed : followed);
            float position[3] = {a[0], a[1], a[2]};
            bool done = false;
            int holdSteps = 0;
            for (int step = 0; step < 6000 && !done; ++step) {
                const NavFollowResult result = navFollow(worlds[0], pair, position);
                if (result.code == 0) {
                    ++(across ? crossArrived : arrived);
                    done = true;
                } else if (result.code != 4 && result.code != 5) {
                    if (result.code != 3 || ++holdSteps > 60) done = true;
                } else {
                    holdSteps = 0;
                    position[1] = result.target[1] + kStandingHeight;
                    const float dx = result.target[0] - position[0], dz = result.target[2] - position[2];
                    const float distance = std::sqrt(dx * dx + dz * dz);
                    const float stepLength = std::min(distance, 0.11f);
                    if (distance > 0.0f) {
                        position[0] += dx / distance * stepLength;
                        position[2] += dz / distance * stepLength;
                    }
                }
            }
            require(done, "the follower ends within 6000 steps");
        }
        // Port run with seed 12345 (no comparison with the original): 26 same-grid paths and 228 paths across grids, 201 of
        // them end on the goal cell. The others end blocked: a grid whose cells in the middle are walled off from the exit cells
        // (grid 660 of lvl_01: the follower marks one exit cell after the other, 0x27e12c, and then turns back) or a refused gate.
        require(paths > 100 && followed > 10 && crossFollowed > 100, "enough paths were found");
        require(arrived == followed, "every same-grid path is followed to its end");
        require(crossArrived * 10 >= crossFollowed * 8, "at least 8 of 10 paths across grids reach the goal cell");
    } catch (const std::exception& error) {
        std::cerr << "nav-path-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "nav-path-real-test: ok\n";
    return 0;
}

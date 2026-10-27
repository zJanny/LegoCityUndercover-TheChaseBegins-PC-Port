// The crane platform of mission 1-3 in the real lvl_01_cherrytreehills (docs/re/props-rocking.md): its record from the level data, the
// attachments the level's "startup" makes with AttachObject, and the descent in `1-3_Generator_Script` that carries the girder and the girder
// guy. Needs the extracted RomFS; exits with 77 (skipped) when it is missing. Level values printed with the C++ level loader by
// 20261026-crane-A on 2026-10-26; the oracle values are RPC 20261025-garage-C-1 (recorded from a save state on the generator trigger spot of
// mission 1-3 after the generator was built, the game's dt 0.033).
#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr float kTraceDt = 0.033f;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: platform-node-move-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "platform-node-move-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
        assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner runner(level, state);
        runner.loadObjectKinds(classOf);
        runner.setObjectPositionSource([&](const std::string& name, float out[3]) {
            const auto start = assets::objectStart(level, name);
            if (!start) return false;
            const runtime::PlatformVector carried = runner.platforms().carriedOffset(name);
            for (int axis = 0; axis < 3; ++axis) out[axis] = start->position[axis] + carried[axis];
            return true;
        });

        // 1-3_Crane_Platform_Dummy (props/prop_platform, room7): type 1.0, Speed 0.01, ObjectTo 1-3_Crane_Platform_Target (dummy at y 5.997).
        const runtime::PlatformRecord* crane = runner.platforms().find("1-3_crane_platform_dummy");
        require(crane && crane->motion.type == 1 && crane->speed == 0.01f && crane->state == 0, "the crane platform: type 1, speed 0.01, state 0");
        require(near(crane->motion.origin[1], 16.6251f, 1e-4f) && near(crane->motion.target[1], 5.9968f, 1e-3f) &&
                    crane->motion.target[0] == crane->motion.origin[0] && crane->motion.target[2] == crane->motion.origin[2],
                "from its placement down to 1-3_Crane_Platform_Target (oracle node y 16.6251 to 5.9968)");
        const runtime::PlatformRecord* ledge = runner.platforms().find("1-3_ledge_platform");
        require(ledge && ledge->motion.type == 1 && near(ledge->motion.target[1], 30.08f, 1e-2f), "1-3_Ledge_Platform rises to 1-3_Ledge_Platform_Top_Target");
        const runtime::PlatformRecord* mower = runner.platforms().find("1-3_lawnmower_01");
        require(mower && mower->motion.type == 3 && mower->motion.path && mower->motion.looped && mower->motion.constantSpeed && mower->speed == 0.007f &&
                    mower->motion.track && mower->motion.track->path.points.size() == 10 && mower->motion.track->path.closed,
                "1-3_Lawnmower_01: type 3, Looped, ConstantSpeed, speed 0.007 on the closed 1-3_Lawnmower_Path_01 (10 points)");
        const runtime::PlatformRecord* rescueMower = runner.platforms().find("1-3_lawnmower_02");
        require(rescueMower && rescueMower->motion.type == 3 && rescueMower->motion.looped && rescueMower->motion.constantSpeed &&
                    rescueMower->motion.orient == 1 && rescueMower->speed == 0.0018f && rescueMower->motion.track &&
                    rescueMower->motion.track->path.points.size() == 35 && rescueMower->motion.track->path.orientations.size() == 39 &&
                    rescueMower->motion.track->path.orientationControls.size() == 39,
                "1-3_Lawnmower_02: orient 1, speed 0.0018 on 1-3_Lawnmower_Path_02 (35 points, 39 quaternions and 39 controls)");

        runtime::runLevelStartup(runner, runtime::kStartupStepDt, runtime::kStartupMaxUpdates);
        const auto parentIs = [&](const char* child, const char* parent) {
            const std::string* found = runner.platforms().nodeParent(child);
            return found && *found == parent;
        };
        require(parentIs("1-3_crane_girder", "1-3_crane_platform_dummy") && parentIs("1-3_ai_girder_guy", "1-3_crane_girder") &&
                    parentIs("1-3_crow", "1-3_crane_girder") && parentIs("rope_01", "1-3_crane_girder") &&
                    parentIs("1-3_ledge_platform_switch", "1-3_ledge_platform"),
                "the startup's AttachObject calls put the girder under the platform and the guy, the crow and the rope under the girder");
        require(runner.platforms().attachedTo("1-3_crane_platform_dummy") == std::vector<std::string>{"1-3_crane_girder"} &&
                    runner.platforms().attachedTo("1-3_crane_girder").empty(),
                "message 43 lists the girder at the crane platform (the girder is not a platform)");
        require(runner.platforms().carriedOffset("1-3_crane_girder")[1] == 0.0f, "nothing moved before the scene");

        require(runner.start("1-3_Generator_Script"), "1-3_Generator_Script");
        int update = 0, triggeredAt = -1, arrivedAt = -1;
        std::uint32_t moves = 0;
        while (update < 400 && arrivedAt < 0) {
            runner.update(kTraceDt);
            if (triggeredAt < 0 && crane->state == runtime::kPlatformMovingState) triggeredAt = update;
            for (const runtime::PlatformArrival& arrival : runner.platforms().takeArrivals())
                if (arrival.instance == "1-3_crane_platform_dummy") {
                    arrivedAt = update;
                    moves = arrival.movedUpdates;
                    require(arrival.state == runtime::kPlatformArrivedState && near(arrival.node[1], 5.9968f, 1e-3f), "the travel ends in state 5 on the target");
                }
            ++update;
        }
        require(triggeredAt >= 0 && arrivedAt - triggeredAt == 102 && moves == 102,
                "trigger at u90, state 5 at u192, the node moved u91..u192 (oracle), got " + std::to_string(arrivedAt - triggeredAt) + " and " + std::to_string(moves));
        const float descent = 5.9968f - 16.6251f;
        for (const char* name : {"1-3_crane_platform_dummy", "1-3_crane_girder", "1-3_ai_girder_guy", "rope_01"})
            require(near(runner.platforms().carriedOffset(name)[1], descent, 2e-3f), std::string(name) + " went down 10.63 (oracle girder y 27.481 to 16.852)");
        runner.update(kTraceDt);
        runner.update(kTraceDt);
        const runtime::AiCharacter* guy = runner.ai().character("1-3_ai_girder_guy");
        require(guy && guy->superJump, "AISuperJump at u193");
        // The oracle's guy stands at y 17.617 at u193, 0.014 below the carried placement: the girder rocks under him, a rotation that the carried offset does not hold.
        require(near(guy->superJump->start[1], 28.259f + descent, 2e-3f) && near(guy->superJump->start[1], 17.617f, 0.02f),
                "the jump starts from the lowered girder (oracle y 17.617)");

        // The rescue mower: TriggerObject(1-3_Lawnmower_02, 1) of 1-3_Start_MowerMan_rescue, sent here directly. The gardener that the startup
        // attaches to it keeps its place on the mower while the mower goes round and turns.
        require(parentIs("1-3_gardner_prop_02", "1-3_lawnmower_02"), "the startup attaches 1-3_Gardner_Prop_02 to 1-3_Lawnmower_02");
        const auto worldOf = [&](const char* name) {
            const std::optional<assets::Placement> start = assets::objectStart(level, name);
            const runtime::PlatformVector carried = runner.platforms().carriedOffset(name);
            return runtime::PlatformVector{start->position[0] + carried[0], start->position[1] + carried[1], start->position[2] + carried[2]};
        };
        const auto riderInMowerFrame = [&]() {
            const runtime::PlatformVector mowerAt = worldOf("1-3_lawnmower_02"), riderAt = worldOf("1-3_gardner_prop_02");
            const runtime::PathRotation rows =
                runtime::multiplyPathRotation(rescueMower->motion.placementRotation, runner.platforms().carriedRotation("1-3_lawnmower_02"));
            runtime::PlatformVector local{};
            for (std::size_t axis = 0; axis < 3; ++axis)
                for (std::size_t k = 0; k < 3; ++k) local[axis] += (riderAt[k] - mowerAt[k]) * rows[axis * 3 + k];
            return local;
        };
        const runtime::PlatformVector seated = riderInMowerFrame();
        runner.platforms().message("1-3_lawnmower_02", 0xff);
        runner.update(kTraceDt);
        require(rescueMower->state == runtime::kPlatformMovingState && runner.platforms().carriedOffset("1-3_lawnmower_02")[0] == 0.0f,
                "the trigger's update takes state 2 without a node move");
        runner.update(kTraceDt);
        const runtime::PlatformVector first = worldOf("1-3_lawnmower_02");
        const assets::LevelPath& rescuePath = rescueMower->motion.track->path;
        require(near(first[0], rescuePath.points[0][0], 0.5f) && near(first[1], 16.8037f, 1e-3f) && near(first[2], rescuePath.points[0][2], 0.1f),
                "the first node move puts the mower on its path next to point 0 (974.57, 16.80, 1047.54), away from its placement (966.26, 16.80, 1058.28)");
        int lapAt = -1;
        float worstSeat = 0.0f;
        for (int step = 1; step < 700 && lapAt < 0; ++step) {
            runner.update(kTraceDt);
            const runtime::PlatformVector seat = riderInMowerFrame();
            for (std::size_t axis = 0; axis < 3; ++axis) worstSeat = std::max(worstSeat, std::fabs(seat[axis] - seated[axis]));
            for (const runtime::PlatformArrival& lap : runner.platforms().takeLaps())
                if (lap.instance == "1-3_lawnmower_02") lapAt = step;
        }
        require(lapAt > 0 && worstSeat < 1e-3f, "the gardener stays in his seat for a whole lap, off by " + std::to_string(worstSeat));
    } catch (const std::exception& error) {
        std::cerr << "platform-node-move-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "platform-node-move-real-test: ok\n";
    return 0;
}

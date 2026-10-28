// The ledge lift of mission 1-3 in the real lvl_01_cherrytreehills (docs/re/props-rocking-platforms.md): 1-3_Ledge_Platform_Switch_Bottom_Script sets the speed
// with SetPlatformSpeed, the platform rises to 1-3_Ledge_Platform_Top_Target carrying the attached pull switch, and after WaitForPlatform the script sets the origin
// and target by Lift_Up and resets the platform (message 8). The platform registers collision by its link 0x19a3e4. Needs the extracted RomFS; exits with 77
// (skipped) when it is missing. Level values printed with the C++ level loader by 20261027-mission13-B on 2026-10-28; the code was read with REA (Ghidra).
#include "assets/object_classes.hpp"
#include "assets/object_collision.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr float kDt = 0.033f;  // the update step of chase-game and of the oracle traces (platform-node-move-real-data)
constexpr float kBottom = 17.2241f;  // 1-3_Ledge_Platform and 1-3_Ledge_Platform_Bottom_Target, initialposition y
constexpr float kTop = 30.0791f;     // 1-3_Ledge_Platform_Top_Target, initialposition y

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

struct Run {
    int updates = 0;
    std::uint32_t moves = 0;
    float switchRise = 0;
    std::vector<float> nodeY;
    std::vector<float> value;
};

Run runLift(chase::runtime::ScriptRunner& runner) {
    using namespace chase;
    require(runner.start("1-3_Ledge_Platform_Switch_Bottom_Script"), "1-3_Ledge_Platform_Switch_Bottom_Script");
    Run run;
    bool arrived = false;
    while (run.updates < 200 && !arrived) {
        runner.update(kDt);
        run.nodeY.push_back(runner.platforms().nodeTranslation("1-3_ledge_platform")->at(1));
        run.value.push_back(runner.platforms().find("1-3_ledge_platform")->value);
        for (const runtime::PlatformArrival& arrival : runner.platforms().takeArrivals())
            if (arrival.instance == "1-3_ledge_platform") {
                arrived = true;
                run.moves = arrival.movedUpdates;
                run.switchRise = runner.platforms().carriedOffset("1-3_ledge_platform_switch")[1];
                require(arrival.state == runtime::kPlatformArrivedState && near(arrival.node[1], kTop, 1e-3f), "the rise ends in state 5 at the top target");
            }
        ++run.updates;
    }
    for (int i = 0; i < 5; ++i) runner.update(kDt);
    return run;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: platform-riding-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "platform-riding-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));

        const std::vector<assets::ObjectCollision> candidates = assets::objectCollisionCandidates(archive, level);
        require(std::any_of(candidates.begin(), candidates.end(), [](const assets::ObjectCollision& entry) { return entry.instance == "1-3_Ledge_Platform"; }),
                "the platform's link 0x19a3e4 registers its collision (0x23aff8 and HasCollisionMesh, as kind 67)");

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
        runtime::runLevelStartup(runner, runtime::kStartupStepDt, runtime::kStartupMaxUpdates);
        const runtime::PlatformRecord* ledge = runner.platforms().find("1-3_ledge_platform");
        require(ledge && ledge->motion.type == 1 && ledge->speed == 0.02f && near(ledge->motion.origin[1], kBottom, 1e-4f), "the ledge platform: type 1, speed 0.02");

        // Lift_Up is 0 (the player never entered 1-3_Lift_Up_Attribute): the script sets origin bottom, target top, and ResetObject puts the node back down.
        const Run first = runLift(runner);
        require(ledge->speed == 0.013f, "SetPlatformSpeed 0.013");
        require(first.moves == 78 && first.updates == 79, "the trigger update, then a value step of 0.033 x 30 x 0.013 reaches 1.0 in 78 node moves, got " + std::to_string(first.moves));
        require(near(first.switchRise, kTop - kBottom, 1e-3f), "the attached pull switch rises with the platform");
        // RPC 20261027-mission13-B-1 (20261027-mission13-servant, 2026-10-28, New 3DS, OpenGL, recorded from a save state on the lift trigger of mission 1-3):
        // after the trigger the record's value reads 0.0129, 0.0257, ... 0.991, then 1.0 with state 5, and the node y 17.3895, 17.555, 17.7204, ... 29.9633, 30.0791.
        const float oracleNode[] = {17.3895f, 17.555f, 17.7204f};
        for (int i = 0; i < 3; ++i)
            require(near(first.nodeY[std::size_t(i + 1)], oracleNode[i], 2e-4f) && near(first.value[std::size_t(i + 1)], 0.01287f * float(i + 1), 1e-4f),
                    "node move " + std::to_string(i + 1) + " as in the oracle trace");
        require(near(first.nodeY[77], 29.9633f, 2e-4f) && near(first.value[77], 0.991f, 1e-3f) && near(first.nodeY[78], kTop, 1e-4f),
                "the 77th move and the end at the top as in the oracle trace");
        require(near(ledge->motion.origin[1], kBottom, 1e-4f) && near(ledge->motion.target[1], kTop, 1e-3f), "Lift_Up 0: origin bottom, target top");
        require(ledge->value == 0.0f && ledge->state == runtime::kPlatformStoppedState && near(runner.platforms().nodeTranslation("1-3_ledge_platform")->at(1), kBottom, 1e-3f),
                "ResetObject: value 0, state 0 and the node back at the origin");

        // Lift_Up 1 (1-3_Lift_Up_Attribute ran while the player rode up): the script swaps origin and target, and the reset leaves the node at the top.
        runner.setVariable(assets::nameHash("Lift_Up"), 1.0f);
        runLift(runner);
        require(near(ledge->motion.origin[1], kTop, 1e-3f) && near(ledge->motion.target[1], kBottom, 1e-4f), "Lift_Up 1: origin top, target bottom");
        require(near(runner.platforms().nodeTranslation("1-3_ledge_platform")->at(1), kTop, 1e-3f), "ResetObject keeps the node at the top origin");
    } catch (const std::exception& error) {
        std::cerr << "platform-riding-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "platform-riding-real-test: ok\n";
    return 0;
}

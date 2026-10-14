// AIMove on real data: the police station entry script Police_ExtToInt of lvl_01_cherrytreehills walks Player1 to
// Enter_Police_Station_Position_End with AIMove, waits and changes the scene. Provenance: oracle state street-after-1-1
// (docs/re/scripts.md): after A the player node stands at (904.652, 16.577, 1113.197) and then moves 0.11 per update
// towards Enter_Police_Station_Position_End. The AI path finder is not ported (STAND-IN SCRIPT-S31); the straight walk
// reproduces that speed. Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: ai-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "ai-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner runner(level, state);
        runner.setObjectPositionSource([&](const std::string& instance, float out[3]) {
            const auto placement = assets::objectStart(level, instance);
            if (placement) std::copy(placement->position, placement->position + 3, out);
            return placement.has_value();
        });
        require(runner.start("Police_ExtToInt"), "script");
        runner.answerDialog(runtime::kDialogConfirm);
        runner.update(1.0f / 30.0f);
        const auto placements = runner.takePlacements();
        require(placements.size() == 1, "PositionObject");
        const auto lower = assets::objectStart(level, placements[0].target);
        const auto end = assets::objectStart(level, "enter_police_station_position_end");
        require(lower && end, "placements");
        require(runner.ai().moving("player1"), "AIMove Player1 follows PositionObject in the same pass and starts a move at once (the script is not blocked by it)");
        const auto start = *runner.ai().character("player1")->position;
        require(std::fabs(start[0] - lower->position[0]) < 1e-4f && std::fabs(start[2] - lower->position[2]) < 1e-4f, "the move starts where PositionObject put the player");
        const std::array<float, 3> before = *runner.ai().character("player1")->position;
        runner.update(1.0f / 30.0f);
        const std::array<float, 3> after = *runner.ai().character("player1")->position;
        const float step = std::hypot(after[0] - before[0], after[2] - before[2]);
        require(std::fabs(step - 0.11f) < 0.002f, "0.11 per update (oracle)");
        const float total = std::hypot(end->position[0] - lower->position[0], end->position[2] - lower->position[2]);
        const int expectedUpdates = int(std::ceil(total / 0.11f));
        int updates = 1;
        while (runner.ai().moving("player1") && updates < 1000) {
            runner.update(1.0f / 30.0f);
            ++updates;
        }
        require(std::abs(updates - expectedUpdates) <= 2, "the walk takes distance / 0.11 updates");
        const auto position = *runner.ai().character("player1")->position;
        require(std::fabs(position[0] - end->position[0]) < 1e-3f && std::fabs(position[2] - end->position[2]) < 1e-3f,
                "the player ends on Enter_Police_Station_Position_End");
        for (int i = 0; i < 600 && runner.threadCount() > 0; ++i) {
            runner.director().update(1.0f / 30.0f, [&](const std::string& name, float out[3]) {
                const auto placed = assets::objectStart(level, name);
                if (placed) std::copy(placed->position, placed->position + 3, out);
                return placed.has_value();
            });
            runner.update(1.0f / 30.0f);
        }
        const std::vector<std::string> changes = runner.takeSceneChanges();
        require(changes.size() == 1, "the script reaches SceneChange after its Wait");
    } catch (const std::exception& error) {
        std::cerr << "ai-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-real-test: ok\n";
    return 0;
}

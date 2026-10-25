// The police entry script Police_ExtToInt of lvl_01_cherrytreehills run by ScriptRunner: WarningDialogBox and
// PositionObject (docs/re/scripts.md, the entry on the script natives of the police entry). Needs the extracted RomFS;
// exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "frontend/resources.hpp"
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
        std::cerr << "usage: script-dialog-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "script-dialog-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        // Provenance of the texts: oracle save state outside the police station after mission 1-1 (screenshot of the dialog, 2026-10-14) and the key
        // lookup with frontend::Resources: gametext fd849176, systemtext d12f028b (OK) and 994b0cc9 (Cancel).
        frontend::Resources resources(archive);
        const runtime::ScriptDialog keys;
        require(resources.textForKey("localisation/gametext_uk.loc", keys.bodyKey) ==
                    "If you are on a mission, any progress will be lost.\nThe mission will reset when you return.",
                "body text");
        require(resources.textForKey("localisation/systemtext_uk.loc", keys.confirmKey) == "OK", "confirm text");
        require(resources.textForKey("localisation/systemtext_uk.loc", keys.cancelKey) == "Cancel", "cancel text");

        for (const int answer : {runtime::kDialogConfirm, runtime::kDialogCancel}) {
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("Police_ExtToInt"), "script Police_ExtToInt");
            require(runner.dialogOpen() && runner.threadCount() == 1, "the script opens the dialog at once");
            require(runner.takePlacements().empty(), "nothing is positioned before the answer");
            for (int i = 0; i < 5; ++i) runner.update(1.0f / 30.0f);
            require(runner.dialogOpen() && runner.variable("Entered_Police_Station") == 0, "blocked until answered");
            runner.answerDialog(answer);
            runner.update(1.0f / 30.0f);
            require(runner.variable("DIALOG_RETURNCODE") == float(answer), "DIALOG_RETURNCODE");

            const auto placements = runner.takePlacements();
            if (answer == runtime::kDialogConfirm) {
                // Oracle (the save state outside the police station after mission 1-1, A pressed): the frame after the answer the player node was at
                // (904.652, 16.577, 1113.197); the placement of the target is (904.652, 16.5773, 1113.2), yaw 45.4542.
                require(runner.variable("Entered_Police_Station") == 1, "Entered_Police_Station");
                require(placements.size() == 1 && placements[0].object == "player1" &&
                            placements[0].target == "enter_police_station_position_lower",
                        "PositionObject Player1 Enter_Police_Station_Position_Lower");
                const auto target = assets::objectStart(level, placements[0].target);
                require(target.has_value(), "target placement");
                require(std::fabs(target->position[0] - 904.652f) < 0.01f && std::fabs(target->position[1] - 16.577f) < 0.01f &&
                            std::fabs(target->position[2] - 1113.197f) < 0.01f && std::fabs(target->orientation[1] - 45.4542f) < 0.01f,
                        "target position and yaw");
                require(runner.threadCount() == 1, "the script waits (Wait 3) after the placement");
            } else {
                require(placements.empty() && runner.variable("Entered_Police_Station") == 0, "cancel positions nothing");
                require(runner.threadCount() == 0, "cancel ends the script");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "script-dialog-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-dialog-real-test: ok\n";
    return 0;
}

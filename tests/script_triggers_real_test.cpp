// Trigger pass of lvl_01_cherrytreehills on real data: the police entrance walk. Provenance: Azahar oracle, state
// street-at-entrance (New 3DS, docs/re/script-triggers.md): the PlayerOnFootBoundEntered trigger 204 (bound
// Police_Station_Transition_Lower) stored "inside" at the check of the update the player entered, the dialog opened, and the
// BoundLeft trigger 203 (same bound, script Police_ExtToInt_SwitchOn) fired at the check after the answer moved the player out.
// Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr std::size_t kLowerBound = 87;
constexpr std::size_t kOnFootTrigger = 204;
constexpr std::size_t kLeftTrigger = 203;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: script-triggers-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "script-triggers-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        // Provenance: trigger census of the level file (scratch tool over Level::triggers, 2026-10-14).
        require(level.triggers.size() == 241, "trigger count");
        require(level.triggers[kOnFootTrigger].event == "PlayerOnFootBoundEntered" &&
                    level.triggers[kOnFootTrigger].firstArgument == "Police_Station_Transition_Lower" &&
                    level.triggers[kOnFootTrigger].scriptName == "Police_ExtToInt",
                "trigger 204");
        require(level.triggers[kLeftTrigger].event == "BoundLeft" &&
                    level.triggers[kLeftTrigger].scriptName == "Police_ExtToInt_SwitchOn",
                "trigger 203");
        require(level.bounds[kLowerBound].name == "Police_Station_Transition_Lower", "bound 87");

        assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner runner(level, state);
        runner.setAllRoomsLoaded();
        require(runner.start("startup"), "startup script");
        for (int i = 0; i < 10; ++i) runner.update(0.034f);
        require(runner.boundsEnabled()[kLowerBound] == 0, "oracle gameplay-start: the bound is off until mission 1-1 is done");
        require(!runner.triggers()[kLeftTrigger].enabled, "the startup switches Police_ExtToInt_SwitchOn off");
        runner.setTriggerEnabled(kLeftTrigger, true);
        runner.setBoundEnabled(kLowerBound, true);

        // Provenance: street-at-entrance (after mission 1-1): bound 87 and trigger 203 are enabled (oracle traps tt1a..tt1f).
        // Bound 87: cylinder centre (907.242, 18.2292, 1115.83), radius 2.0466, half height 2 (level section +0x24).
        // The path x = 906.0 enters the cylinder where dz < sqrt(2.0466^2 - 1.242^2) = 1.627, z > 1114.203: step 9 of 0.25 from 1112.
        int enteredAt = -1;
        float z = 1112.0f;
        for (int step = 0; step < 16 && enteredAt < 0; ++step, z += 0.25f) {
            const float player[3] = {906.0f, 16.57f, z};
            runner.checkTriggers(player);
            runner.update(0.034f);
            if (runner.dialogOpen()) enteredAt = step;
        }
        require(enteredAt == 9, "the entrance dialog opens in the update after the check that saw the player inside: " + std::to_string(enteredAt));
        require(runner.triggers()[kOnFootTrigger].inside && !runner.triggers()[kOnFootTrigger].skip, "trigger 204 stored inside");
        require(runner.triggers()[kLeftTrigger].inside, "BoundLeft trigger 203 stored inside at the same check");

        // Oracle: the check after the answer (the player was placed outside) fires BoundLeft; it needs the inside state stored before.
        runner.answerDialog(runtime::kDialogConfirm);
        const float placed[3] = {904.652f, 16.577f, 1113.197f};
        runner.checkTriggers(placed);
        require(!runner.triggers()[kLeftTrigger].inside, "trigger 203 stored outside");
        require(runner.threadCount() >= 1, "BoundLeft thread created");
        std::cout << "script-triggers-real: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "script-triggers-real: FAIL: " << error.what() << '\n';
        return 1;
    }
}

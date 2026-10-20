#pragma once
// The save slot of a chase-game run (docs/re/save-format-state.md): the live slot of a new game or of --save FILE --slot N,
// the stud and super brick counters, the play-time tick and the save requests written to the port's save file.
#include "assets/assets.hpp"
#include "runtime/natives_progress.hpp"
#include "runtime/script_runner.hpp"
#include "game_options.hpp"
#include "save/level_state.hpp"
#include "save/new_game.hpp"
#include "save/save_writer.hpp"
#include "save/slot_progress.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace game_save {

// The state the original keeps in globals for the whole run: the live slot 0x51b6b0, the counters 0x593418 / 0x593430 and
// the file image of the save writer. It outlives the level sessions of a run (scene changes).
struct Progress {
    bool started = false;
    chase::save::Bytes slot;
    std::uint64_t studs = 0;
    std::uint32_t superBricks = 0;
    chase::save::SaveRequestWriter writer;
};

std::string sceneNameOf(const std::string& levelPath);

// --save-out OUT, else the port's save file (chase::save::defaultSavePath). STAND-IN SAVE-S10: a run started with
// --save FILE writes there too, not into FILE (unless FILE is that path).
std::string saveTarget(const game_options::Options& options);

void chooseContinueLevel(chase::assets::FuseArchive& archive, game_options::Options& options);

// The first level of a run: the slot of --save FILE --slot N or a new game (0x228274 and 0x1a25cc), then the level entry
// 0x20a4d4 for the level being started; sets up the writer for the save target.
void start(Progress& progress, const game_options::Options& options, chase::assets::FuseArchive& archive, const std::string& levelPath);

// A level entered through a scene change: the enter 0x2e1324 stores the TablesLevels row of the level at slot +6 (rows
// 1..8 here; the original compares all 28 rows).
void sceneEntered(Progress& progress, chase::assets::FuseArchive& archive, const std::string& levelPath);

// The progress of the live slot for the level being started (level unlock bits, game complete, saved level variables).
chase::save::SlotStart slotStartFor(const Progress& progress, chase::assets::FuseArchive& archive, const chase::assets::Level& level,
                                    const std::string& levelPath);
void apply(const chase::save::SlotStart& start, chase::runtime::ScriptRunner& runner);

// What the level state refresh needs of a level session: the records and end bit set of the slot's saved state of the
// level and the live level variables of `runner`.
chase::save::LevelStateInput levelStateInput(const chase::save::SlotStart& start, const chase::runtime::ScriptRunner& runner);
// 0x192f60 on the live slot for level `levelId` (only 1..8, as 0x21cf38 checks); prints when the data does not fit.
void refreshLevelState(chase::save::Bytes& slot, const chase::assets::Level& level, unsigned levelId,
                       const chase::save::LevelStateInput& input);

// The pause mode's update 0x300378 adds 1 to the play time at slot +0 (0x300444) once per update, as the level update does.
void playTick(chase::runtime::ProgressNativeState& natives);

// One update: the play-time tick of the update loops (STAND-IN SAVE-S11: every update of a level session ticks) and,
// when the progress natives made new save requests since `seenRequests`, one request 0x21cf38(0): the level state
// (`refreshLevelState` on the live slot) and the summary with the live counters, then the commit.
void update(Progress& progress, chase::runtime::ProgressNativeState& natives, unsigned& seenRequests, std::uint64_t studs,
            std::uint32_t superBricks, const std::function<void(chase::save::Bytes&)>& refreshLevelState);

}

// The progress that a real save holds (a copy of the user's savegame.dat, never committed) and what it does to the
// start of lvl_01. Usage: slot-progress-real-test <lego_city3ds.fib> <lego_city3ds.csv> <savegame.dat>; exits 77 (skip)
// when a file is missing or the save is not the one whose values are pinned below.
#include "runtime/level_start.hpp"
#include "save/completion_tables.hpp"
#include "save/slot_progress.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
using namespace chase;

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
// Stored CRC (file +0) of the copy of the user's save that the values below were read from.
constexpr std::uint32_t kPinnedCrc = 0x937dacf8;
constexpr std::size_t kBoundPoliceStationLower = 87;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 4 || !std::ifstream(argv[1]) || !std::ifstream(argv[2]) || !std::ifstream(argv[3])) {
        std::cout << "slot-progress-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        std::ifstream file(argv[3], std::ios::binary);
        const save::Bytes bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        const std::optional<save::SaveImage> image = save::SaveImage::fromBytes(bytes);
        require(image && image->validate() == save::Status::Ok, "the save is not a valid image");
        if (image->storedChecksum() != kPinnedCrc) {
            std::cout << "slot-progress-real-test: this is not the pinned save, skipped\n";
            return 77;
        }

        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const save::CompletionTotals totals = save::completionTotals(archive);
        const assets::FuseEntry* levelTable = archive.find("tables/tableslevels.bxls");
        require(levelTable != nullptr, "TablesLevels missing");
        const assets::Table levels = assets::parseTable(archive.read(*levelTable));

        // Provenance: TablesLevels sheet 0 column 21 (chase-tabledump tables/tableslevels.bxls): rows 1 and 2.
        require(save::levelIdForScene(levels, "lvl_00_policestation") == 1u, "police station level id");
        require(save::levelIdForScene(levels, "LVL_01_CherryTreeHills") == 2u, "lvl_01 level id");
        require(!save::levelIdForScene(levels, "lvl_the_city") && !save::levelIdForScene(levels, "lvl_customisation"), "rows outside 1..8");

        // Provenance (all values below): the pinned save, slot 0, read with the offsets of docs/re/save-format.md
        // (2026-10-14, level state block); StoryProgress, 1-1_IntroPlayed, Entered_Police_Station and Enter_Police_Car
        // were also read through a scratch decoder of the same layout. Mission 1-1 is done (StoryProgress 2).
        const save::SlotView slot = image->slotView(0);
        const save::SlotStart start = save::slotStart(slot, level, 2, totals.levelTotals[save::kGameCompleteLevel - save::kFirstCounterLevel]);
        require(start.stateSaved && start.storyProgress == 2, "saved StoryProgress");
        require(start.levelUnlocked == 1u << 2 && !start.gameComplete, "unlock bits");
        require(start.variables.size() == 43, "saved variable count (46 saved parameters minus a string and two vectors)");
        const auto saved = [&](const char* name) {
            for (const save::SavedVariable& variable : start.variables)
                if (variable.hash == assets::nameHash(name)) return variable.value;
            throw std::runtime_error(std::string("variable not saved: ") + name);
        };
        require(saved("StoryProgress") == 2.0f && saved("1-1_IntroPlayed") == 1.0f, "mission 1-1 variables");
        require(saved("Entered_Police_Station") == 1.0f && saved("Enter_Police_Car") == 1.0f, "police variables");
        require(saved("DIALOG_RETURNCODE") == 2.0f && saved("Music_NumberGen") == 1.0f, "dialog and music variables");

        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        assets::PlacedObjectState newGameState = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner newGame(level, newGameState);
        runtime::runLevelStartup(newGame, runtime::kStartupStepDt, runtime::kStartupMaxUpdates);
        require(newGame.boundsEnabled()[kBoundPoliceStationLower] == 0, "new game: Police_Station_Transition_Lower is off");

        assets::PlacedObjectState savedState = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner continued(level, savedState);
        runtime::ScriptProgress progress;
        progress.levelUnlocked = start.levelUnlocked;
        progress.gameComplete = start.gameComplete;
        continued.setProgress(progress);
        for (const save::SavedVariable& variable : start.variables) continued.setVariable(variable.hash, variable.value);
        runtime::runLevelStartup(continued, runtime::kStartupStepDt, runtime::kStartupMaxUpdates);
        require(continued.boundsEnabled()[kBoundPoliceStationLower] == 1, "saved game: Police_Station_Transition_Lower is on");
        require(continued.variable("StoryProgress") == 2.0f, "StoryProgress after startup");
        // Provenance: oracle state street-after-1-1 (the user's quick save after mission 1-1), memory read of the 482 bound
        // records (0x28 bytes from 0x08e02284, enable byte +0xa) with `read`; indices of the bounds that read 0
        // (artifacts/oracle/agentJ-out/compare/bounds-street-after-1-1-vs-save-route.txt, docs/re/save-format.md).
        const std::vector<int> oracleBoundsOff = {
            2, 3, 5, 6, 7, 8, 9, 10, 17, 18, 19, 20, 21, 22, 23, 24, 25, 29, 30, 36, 38, 42, 44, 45, 46, 47, 48, 51, 52, 53, 54, 55, 67, 68, 70,
            79, 80, 81, 82, 91, 92, 94, 95, 96, 99, 100, 101, 111, 118, 119, 120, 121, 122, 123, 126, 127, 129, 138, 139, 140, 141, 142, 143,
            150, 151, 152, 153, 154, 161, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 202, 203, 204, 223, 224, 251, 252,
            254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 266, 271, 272, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302,
            303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 332, 333, 334, 335, 336,
            337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362,
            363, 364, 402, 422, 423, 424, 425, 426, 427, 428, 429, 430, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454,
            455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 471, 472, 473, 474, 480};
        const std::vector<int> knownBoundDifferences = {266, 271, 272};
        require(level.bounds.size() == 482, "bound count changed");
        for (std::size_t i = 0; i < level.bounds.size(); ++i) {
            const bool off = std::find(oracleBoundsOff.begin(), oracleBoundsOff.end(), int(i)) != oracleBoundsOff.end();
            const bool known = std::find(knownBoundDifferences.begin(), knownBoundDifferences.end(), int(i)) != knownBoundDifferences.end();
            require((bool(continued.boundsEnabled()[i]) == !off) != known, "bound enable byte differs from street-after-1-1: " + level.bounds[i].name);
        }
        require(newGameState.disabled != savedState.disabled, "the save changes the placed objects");
    } catch (const std::exception& error) {
        std::cerr << "slot-progress-real: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "slot-progress-real: ok\n";
    return 0;
}

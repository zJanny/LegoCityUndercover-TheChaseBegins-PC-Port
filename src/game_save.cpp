#include "game_save.hpp"

#include "save/completion_tables.hpp"
#include "save/continue_start.hpp"

#include <iostream>
#include <stdexcept>

namespace game_save {
namespace {
using namespace chase::save;

chase::assets::Table levelTable(chase::assets::FuseArchive& archive) {
    const chase::assets::FuseEntry* entry = archive.find("tables/tableslevels.bxls");
    if (!entry) throw std::runtime_error("TablesLevels missing in the archive");
    return chase::assets::parseTable(archive.read(*entry));
}

SaveImage loadedImage(const std::string& file) {
    const LoadResult loaded = load(file);
    if (!loaded.found) throw std::runtime_error("save file not found: " + file);
    if (loaded.status != Status::Ok || !loaded.image) throw std::runtime_error("not a valid save file: " + file);
    return *loaded.image;
}

// The file the writer commits into: the target as it is when valid; otherwise the --save file, or for a new game the
// reset defaults of 0x2081cc that the load check 0x1d0acc falls back to.
SaveImage baseImage(const std::string& target, const game_options::Options& options) {
    const LoadResult existing = load(target);
    if (existing.found && existing.status == Status::Ok && existing.image) return *existing.image;
    if (!options.saveFile.empty()) return loadedImage(options.saveFile);
    SaveImage image;
    image.resetOptions(std::uint8_t(options.language));
    image.seal();
    return image;
}

std::optional<std::string> targetOrNothing(const game_options::Options& options) {
    try {
        return saveTarget(options);
    } catch (const std::exception& error) {
        std::cout << "save: writing disabled (" << error.what() << ")\n";
        return std::nullopt;
    }
}
}

std::string sceneNameOf(const std::string& levelPath) {
    const std::string folder = levelPath.substr(0, levelPath.find_last_of('/'));
    return folder.substr(folder.find_last_of('/') + 1);
}

std::string saveTarget(const game_options::Options& options) { return options.saveOut.empty() ? defaultSavePath() : options.saveOut; }

void chooseContinueLevel(chase::assets::FuseArchive& archive, game_options::Options& options) {
    const SaveImage image = loadedImage(options.saveFile);
    const ContinueStart start = continueStart(image.slotView(options.saveSlot), levelTable(archive));
    options.level = levelPathOfScene(start.scene);
    std::cout << "continue: slot " << options.saveSlot << " starts level " << start.level << " (" << start.scene << ")\n";
}

void start(Progress& progress, const game_options::Options& options, chase::assets::FuseArchive& archive, const std::string& levelPath) {
    if (options.saveSlot >= kSlotCount) throw std::runtime_error("--slot must be 0 or 1");
    const NewGameTables tables = newGameTables(archive);
    const bool newGame = options.saveFile.empty();
    if (newGame) {
        progress.slot = newGameSlot(tables);
    } else {
        const SaveImage image = loadedImage(options.saveFile);
        const SlotView slot = image.slotView(options.saveSlot);
        if (!slotInUse(slot)) throw std::runtime_error("save slot " + std::to_string(options.saveSlot) + " is empty");
        progress.slot.assign(slot.data, slot.data + slot.size);
    }
    const unsigned levelId = levelIdForScene(levelTable(archive), sceneNameOf(levelPath)).value_or(0);
    const LevelEntry entry = enterLevel(progress.slot, levelId, tables);
    progress.studs = entry.studs;
    progress.superBricks = entry.superBricks;
    progress.started = true;
    progress.volumes = {std::uint8_t(options.soundVolume), std::uint8_t(options.musicVolume)};
    const std::optional<std::string> target = targetOrNothing(options);
    SaveImage live = newGame ? baseImage(target.value_or(std::string()), options) : loadedImage(options.saveFile);
    if (newGame) {
        // STAND-IN UI-S10: the save of a new game's start (front-end saving screen) is not ported; its task would store +2.
        // The slot volumes are --sound-volume / --music-volume, by default the 10 and 8 of the slot callback 0x1a25cc.
        resetSlotOptions(live, options.saveSlot);
        live.options()[2] = std::uint8_t(options.saveSlot);
        live.setSoundVolume(options.saveSlot, std::uint8_t(options.soundVolume));
        live.setMusicVolume(options.saveSlot, std::uint8_t(options.musicVolume));
    }
    progress.options.assign(live.options(), live.options() + kOptionsSize);
    if (target) {
        SaveImage base = newGame ? live : baseImage(*target, options);
        progress.writer = SaveRequestWriter(*target, base, options.saveSlot, completionTotals(archive));
        std::cout << "save: " << (newGame ? "new game" : "loaded") << " in slot " << options.saveSlot << ", save requests write "
                  << *target << '\n';
    }
    std::cout << "save: level entry " << entry.level << ", studs " << entry.studs << ", super bricks " << entry.superBricks << '\n';
}

void loadingScreen(Progress& progress) {
    if (progress.options.size() == kOptionsSize) chase::runtime::loadingScreenVolumes(progress.options.data(), progress.volumes);
    std::cout << "loading screen: sound volume " << unsigned(progress.volumes.sound) << ", music volume " << unsigned(progress.volumes.music)
              << '\n';
}

void sceneEntered(Progress& progress, chase::assets::FuseArchive& archive, const std::string& levelPath) {
    const std::optional<unsigned> levelId = levelIdForScene(levelTable(archive), sceneNameOf(levelPath));
    if (levelId && progress.slot.size() > kSlotCurrentLevelOffset) progress.slot[kSlotCurrentLevelOffset] = std::uint8_t(*levelId);
}

SlotStart slotStartFor(const Progress& progress, chase::assets::FuseArchive& archive, const chase::assets::Level& level,
                       const std::string& levelPath) {
    const unsigned levelId = levelIdForScene(levelTable(archive), sceneNameOf(levelPath)).value_or(0);
    const CompletionTotals totals = completionTotals(archive);
    return slotStart(SlotView{progress.slot.data(), progress.slot.size()}, level, levelId,
                     totals.levelTotals[kGameCompleteLevel - kFirstCounterLevel]);
}

void apply(const SlotStart& start, chase::runtime::ScriptRunner& runner) {
    chase::runtime::ScriptProgress progress;
    progress.levelUnlocked = start.levelUnlocked;
    progress.gameComplete = start.gameComplete;
    runner.setProgress(progress);
    runner.progressNatives().setSlotImage(start.slotBytes);
    for (const SavedVariable& variable : start.variables) runner.setVariable(variable.hash, variable.value);
}

void setUpParty(const SlotStart& start, chase::assets::FuseArchive& archive, chase::runtime::ScriptRunner& runner) {
    namespace runtime = chase::runtime;
    const NewGameTables tables = newGameTables(archive);
    const Bytes& slot = start.slotBytes;
    if (start.level >= tables.levelParty.size() || slot.size() <= kSlotPartyOffset + runtime::kPartyLevelIds) return;
    std::array<std::uint8_t, runtime::kPartyLevelIds> slotIds{};
    for (unsigned i = 0; i < runtime::kPartyLevelIds; ++i) slotIds[i] = slot[kSlotPartyOffset + i];
    const bool ownParty = (slot[kSlotFlagsOffset] & kSlotOwnPartyBit) != 0;
    const runtime::PartyList list =
        runtime::partyList(tables.levelParty[start.level], tables.levelPartyExtras[start.level], slotIds, ownParty, false);
    runtime::PartyManager& party = runner.partyNatives().party();
    runtime::PartyFlags flags = party.flags();
    flags.groupBytes = {slot[kSlotGroupBitsOffset], slot[kSlotGroupBitsOffset + 1]};
    party.setFlags(flags);
    party.setLevel(std::int8_t(start.level));
    party.setUp(list);
    std::cout << "party: level " << start.level << ", ids";
    for (const runtime::PartySlot& member : party.slots()) std::cout << ' ' << unsigned(member.id) << (member.removed ? "-" : "");
    std::cout << ", usable " << party.usableCount() << ", walk " << party.walkCount() << ", player character " << unsigned(party.playerCharacter())
              << '\n';
}

LevelStateInput levelStateInput(const SlotStart& start, const chase::runtime::ScriptRunner& runner) {
    LevelStateInput input;
    input.variable = [&runner](std::uint32_t hash) { return runner.variable(hash); };
    input.keptRecords = start.objectRecords;
    input.endBitSetBytes = start.endBitSetBytes;
    return input;
}

void refreshLevelState(Bytes& slot, const chase::assets::Level& level, unsigned levelId, const LevelStateInput& input) {
    if (levelId < 1 || levelId > kGameCompleteLevel) return;
    const std::optional<Bytes> data = levelStateData(level, input);
    if (!data || !writeLevelState(slot, levelId, *data, storyProgressOf(level, input)))
        std::cout << "save: the level state of level " << levelId << " does not fit into its block\n";
}

void playTick(chase::runtime::ProgressNativeState& natives) {
    Bytes& slot = natives.slotImage();
    if (slot.size() == kSlotSize) addPlayTick(slot.data());
}

void update(Progress& progress, chase::runtime::ProgressNativeState& natives, unsigned& seenRequests, std::uint64_t studs,
            std::uint32_t superBricks, const std::function<void(Bytes&)>& refreshLevelState) {
    Bytes& slot = natives.slotImage();
    if (slot.size() != kSlotSize) return;
    addPlayTick(slot.data());
    if (natives.saveRequests() == seenRequests) return;
    seenRequests = natives.saveRequests();
    if (refreshLevelState) refreshLevelState(slot);
    SlotSummaryInput input;
    input.studs = studs;
    input.superBricks = superBricks;
    input.studType = slotStudType(SlotView{slot.data(), slot.size()});
    if (progress.writer.request(slot, input, progress.options.size() == kOptionsSize ? progress.options.data() : nullptr))
        std::cout << "save: request " << progress.writer.written() << " written to " << progress.writer.path() << '\n';
}

}

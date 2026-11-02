// The AI spawners (src/runtime/ai_spawner.*) on the level data, and the first fight chain of mission 1-4 run through the class, the level's reference
// records and its scripts. Parameter values read by 20261102-mission14-A (2026-11-02) with the C++ level loader on the eight main levels and the class
// file props/prop_aispawner; the rules are those of docs/re/scripts-missions-1-4-spawners.md (objdump of game.arm.elf), not measured in the original. Needs the
// extracted RomFS; exits with 77 (skipped) when missing.
#include "assets/assets.hpp"
#include "runtime/ai_spawner.hpp"
#include "runtime/reference_trigger_apply.hpp"
#include "runtime/reference_trigger_level.hpp"
#include "runtime/script_runner.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::Level load(assets::FuseArchive& archive, const std::string& file) {
    const assets::FuseEntry* entry = archive.find("levels/levels/" + file + "/" + file + "_main.blvl");
    require(entry != nullptr, "not in archive: " + file);
    return assets::parseLevel(archive.read(*entry));
}

runtime::AiSpawnerSet spawnersOf(const assets::Level& level, const assets::ClassLookup& classOf) {
    runtime::AiSpawnerSet set;
    set.load(level, classOf, [&](const std::string& character) -> std::optional<assets::Matrix> {
        const auto placement = assets::objectStart(level, character);
        if (!placement) return std::nullopt;
        return assets::placementMatrix(*placement);
    });
    return set;
}

struct LevelWorld : runtime::AiSpawnerWorld {
    LevelWorld(assets::PlacedObjectState& state, runtime::ReferenceTriggers& references, runtime::ScriptRunner& scripts)
        : state(state), references(references), scripts(scripts) {}
    assets::PlacedObjectState& state;
    runtime::ReferenceTriggers& references;
    runtime::ScriptRunner& scripts;
    std::map<std::string, assets::Matrix> released;
    std::vector<std::string> fired;
    bool held(const std::string& c) const override { return state.destroyed.count(c) != 0; }
    bool disabled(const std::string& c) const override { return state.disabled.count(c) != 0; }
    void hold(const std::string& c) override { state.destroyed.insert(c), state.disabled.insert(c); }
    void disable(const std::string& o) override { state.disabled.insert(o); }
    void restore(const std::string& c) override { state.destroyed.erase(c), state.disabled.erase(c); }
    bool controllerAllowsSpawn(const std::string&) const override { return true; }
    bool inView(const assets::Matrix&) const override { return false; }
    bool release(const std::string& c, const assets::Matrix& m) override {
        released[c] = m;
        state.destroyed.erase(c), state.disabled.erase(c);
        return true;
    }
    void aiSpawned(const std::string&) override {}
    void activateReference(const std::string& s) override {
        const auto id = references.findOwner(s);
        require(id.has_value(), "the spawner registers a reference record: " + s);
        const runtime::ReferenceActivation activation = references.activate(*id);
        if (activation.fired) fired.push_back(activation.target);
        runtime::applyReferenceActivation(activation, scripts);
    }
    void deactivateReference(const std::string&) override {}
    bool referenceIdle(const std::string& s) const override {
        const auto id = references.findOwner(s);
        return !id || references.kind(*id) == runtime::ReferenceKind::Idle;
    }
    void reveal(const std::string&, const std::string&) override {}
    bool playerInside(const runtime::AiSpawner&) const override { return true; }
};

void census(assets::FuseArchive& archive, const assets::ClassLookup& classOf) {
    const std::vector<std::pair<std::string, std::size_t>> levels = {
        {"lvl_00_policestation", 0}, {"lvl_01_cherrytreehills", 17}, {"lvl_02_albatrossisland", 9}, {"lvl_03_auburn", 10},
        {"lvl_04_fortmeadows", 7},   {"lvl_05_bluebellwood", 26},   {"lvl_06_apolloisland", 26},    {"lvl_07_downtown", 14}};
    std::size_t slots = 0, offScreen = 0, ownPlace = 0, waitForDead = 0, extras = 0, startActive = 0, startSpawned = 0;
    for (const auto& [file, count] : levels) {
        const runtime::AiSpawnerSet set = spawnersOf(load(archive, file), classOf);
        require(set.spawners().size() == count, file + ": spawners " + std::to_string(set.spawners().size()));
        for (const runtime::AiSpawner& s : set.spawners()) {
            slots += s.slots.size();
            offScreen += !s.params.spawnOnScreen;
            ownPlace += !s.params.spawnFromSpawner;
            waitForDead += s.params.waitForDead;
            startActive += s.params.startActive;
            startSpawned += s.params.startSpawned;
            extras += !s.params.mesh.empty() || !s.params.startEffect.empty() || !s.params.revealObject.empty() || !s.params.playerInsideBound.empty() ||
                      s.params.sfx != 0 || s.params.maxOnScreen != 0;
        }
    }
    require(slots == 239, "slots of the 109 spawners: " + std::to_string(slots));
    require(offScreen == 24 && ownPlace == 104 && waitForDead == 97, "SpawnOnScreen 0, SpawnFromSpawner 0, WaitForDead 1 counts");
    require(startActive == 2 && startSpawned == 1, "StartActive 1 (goonspawner03, 6-1_mine_goon_spawner01) and StartSpawned 1 (goonspawner03)");
    require(extras == 0, "no spawner has a mesh, StartEffect, RevealObject, PlayerInsideBound, ATTR_SFX or MaxOnScreen");
}

void missionFourteen(assets::FuseArchive& archive, const assets::ClassLookup& classOf) {
    const assets::Level level = load(archive, "lvl_01_cherrytreehills");
    runtime::AiSpawnerSet set = spawnersOf(level, classOf);
    struct Expected {
        const char* spawner;
        std::vector<std::string> characters;
        int maxSpawn;
        unsigned respawn, stagger, startState;
        bool waitForDead;
        const char* target;
    };
    const std::vector<Expected> expected = {
        {"1-4_spawner_05", {"1-4_ai_goon_03", "1-4_ai_goon_06"}, 2, 90, 0, 0, true, "1-4_so_phase1"},
        {"1-4_spawner_06", {"1-4_ai_goon_07", "1-4_ai_goon_08", "1-4_ai_goon_14"}, 3, 90, 18, 6, true, "1-4_spawner_06_script"},
        {"1-4_spawner_07", {"1-4_ai_goon_15", "1-4_ai_goon_16", "1-4_ai_goon_17"}, 3, 90, 18, 6, true, "1-4_so_phase2"},
        {"1-4_spawner_03", {"1-4_ai_goon_09", "1-4_ai_goon_10"}, 2, 0, 0, 0, false, ""},
        {"1-4_spawner_04", {"1-4_ai_goon_11", "1-4_ai_goon_12", "1-4_ai_goon_13"}, 3, 0, 0, 0, true, "1-4_hatch_open"},
        {"1-4_spawner_01", {"1-4_ai_goon_01", "1-4_ai_goon_02"}, 2, 0, 3, 6, true, ""},
        {"1-4_boosterchaser_spawner", {"1-4_booster_strange_chaser"}, 1, 90, 0, 0, true, ""},
        {"1-4_pursuit_booster_spawner", {"1-4_booster_strange_02"}, 1, 0, 0, 0, true, ""},
    };
    runtime::ReferenceTriggers references;
    runtime::registerLevelReferences(references, level, classOf);
    assets::PlacedObjectState state;
    runtime::ScriptRunner scripts(level, state);
    LevelWorld world(state, references, scripts);
    set.postLink(world);
    std::size_t held = 0;
    for (const Expected& e : expected) {
        const runtime::AiSpawner* s = set.find(e.spawner);
        require(s != nullptr, std::string("spawner ") + e.spawner);
        std::vector<std::string> characters;
        for (const runtime::AiSpawnerSlot& slot : s->slots) characters.push_back(slot.character);
        require(characters == e.characters, std::string("slots of ") + e.spawner);
        require(s->maxSpawn == e.maxSpawn && s->params.respawnFrames == e.respawn && s->params.staggerFrames == e.stagger &&
                    s->params.startState == e.startState && s->params.waitForDead == e.waitForDead,
                std::string("parameters of ") + e.spawner);
        require(!s->params.startActive && !s->params.startSpawned && !s->params.spawnFromSpawner && s->params.spawnOnScreen,
                std::string("StartActive 0, StartSpawned 0, SpawnFromSpawner 0, SpawnOnScreen 1: ") + e.spawner);
        const auto id = references.findOwner(e.spawner);
        require(id && references.target(*id) == e.target, std::string("TriggerObject of ") + e.spawner);
        for (const std::string& c : e.characters) held += world.held(c) && world.disabled(c);
    }
    require(held == 17, "the 15 goons and two Booster Strange of the spawners are held from the level load");

    require(set.message("1-4_spawner_05", runtime::kAiSpawnerMessageOn, world), "1-4_ShowroomArrival's TriggerObject reaches 1-4_Spawner_05");
    for (int i = 0; i < 4; ++i) set.update(world);
    require(world.released.count("1-4_ai_goon_03") && world.released.count("1-4_ai_goon_06"), "update 2 and 4 release goons 03 and 06");
    const auto goon = assets::objectStart(level, "1-4_AI_Goon_03");
    require(goon && std::fabs(world.released["1-4_ai_goon_03"][12] - goon->position[0]) < 1e-4f &&
                std::fabs(world.released["1-4_ai_goon_03"][14] - goon->position[2]) < 1e-4f,
            "SpawnFromSpawner 0: goon 03 comes out at its own placement");
    require(world.held("1-4_ai_goon_07"), "Spawner_06 still holds its goons");
    for (int i = 0; i < 3; ++i) set.update(world);
    require(world.fired.empty(), "WaitForDead: nothing fires while the goons are out");
    world.hold("1-4_ai_goon_03");
    world.hold("1-4_ai_goon_06");
    set.update(world);
    require(world.fired == std::vector<std::string>{"1-4_so_phase1"}, "both goons done: the reference fires 1-4_SO_Phase1");

    const float far[3] = {0, -1000, 0};
    bool phase1 = false;
    for (int i = 0; i < 300 && !phase1; ++i) {
        scripts.checkTriggers(far);
        scripts.update(1.0f / 30.0f);
        for (const runtime::ScriptObjectMessage& sent : scripts.takeObjectMessages())
            if (sent.instance == "1-4_spawner_06" && sent.message == runtime::kAiSpawnerMessageOn) phase1 = set.message(sent.instance, sent.message, world);
    }
    require(phase1, "the ObjectTriggered trigger of 1-4_SO_Phase1 runs the script, whose TriggerObject reaches 1-4_Spawner_06");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: ai-spawner-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "ai-spawner-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        census(archive, classOf);
        missionFourteen(archive, classOf);
    } catch (const std::exception& error) {
        std::cerr << "ai-spawner-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-spawner-real-test: ok\n";
    return 0;
}

// Tests of the first callers of the dropped studs (src/runtime/stud_callers.*), docs/re/studs.md entry "First callers of the dropped studs".
// Read in llvm-objdump of artifacts/game/game.arm.elf: 0x2bc370 (the call at 0x2bc594 with r1 = WobbleStudValue, r2 = 1, r3 = 1, stack word 1), 0x23a6b0,
// 0x2149ac, 0x1aad90 (the call at 0x1ab234 with r2 = StudValue, r3 = 1, stack words 0 and 1), 0x16cccc. Real-data counts: the levels of the FIB,
// counted with the loaders of this port (this test, 2026-10-19); the position of the van body is its placement in lvl_01 (printed by this test's loader).
#include "assets/assets.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/level_start.hpp"
#include "runtime/prop_wobbly_destructible.hpp"
#include "runtime/stud_callers.hpp"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) <= 1e-5f; }

constexpr std::array<float, 16> kNode = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 5, 6, 7, 1};
constexpr std::array<float, 16> kTurned = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 5, 6, 7, 1};

void requests() {
    const StudSpawnSource plain;
    const std::optional<StudDropRequest> hit = wobbleHitStudRequest(plain, 2, kNode);
    require(hit && hit->mode == StudDropMode::Count && hit->count == 2, "the wobble hit drops two studs in mode 0 (0x23a6b0 stack word 1)");
    require(near(hit->position[0], 5) && near(hit->position[1], 6) && near(hit->position[2], 7), "at the node");
    require(near(hit->direction[0], 0) && near(hit->direction[2], 1), "along the node's row 2 without SpawnDir");
    require(near(hit->spread, 1.0f) && !hit->directionIsVelocity && hit->skip == kStudDropDefaultSkip && !hit->collectAtOnce, "the fixed arguments of 0x2149ac");
    const std::optional<StudDropRequest> turned = wobbleHitStudRequest(plain, 1, kTurned);
    require(turned && near(turned->direction[0], 1) && near(turned->direction[2], 0), "row 2 of a turned node");
    require(!wobbleHitStudRequest(plain, 0, kNode), "WobbleStudValue 0 drops nothing (0x2149ac returns at count 0)");

    StudSpawnSource aimed;
    aimed.spawnDir = 90;
    aimed.spawnCone = 0.5f;
    const std::optional<StudDropRequest> turnedAway = wobbleHitStudRequest(aimed, 2, kNode);
    require(turnedAway && near(turnedAway->direction[0], 1) && std::fabs(turnedAway->direction[2]) < 1e-6f && near(turnedAway->spread, 0.5f), "SpawnDir 90 and SpawnCone");
    StudSpawnSource located;
    located.hasSpawnLoc = true;
    located.spawnLocNode = kTurned;
    const std::optional<StudDropRequest> atLocation = wobbleHitStudRequest(located, 2, kNode);
    require(atLocation && near(atLocation->direction[0], 1), "SpawnLoc replaces the node");

    const std::optional<StudDropRequest> built = builditStudRequest(plain, 25.0f, kNode);
    require(built && built->mode == StudDropMode::Random && built->count == 25, "a finished build drops StudValue 25 in mode 1 (0x1aad90 stack word 0 = 0)");
    require(near(built->position[0], 5) && near(built->direction[2], 1), "at the final prop, along its row 2");
    require(!builditStudRequest(plain, 0.0f, kNode), "StudValue 0 drops nothing");
    StudSpawnSource buildLoc = located;
    const std::optional<StudDropRequest> ignored = builditStudRequest(buildLoc, 25.0f, kNode);
    require(ignored && near(ignored->direction[2], 1), "0x1aad90 passes the final prop, not SpawnLoc");
}

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

    std::map<std::string, std::array<std::size_t, 2>> studClasses, forcedClasses;
    std::size_t spawnLocs = 0;
    for (const std::string& name : archive.names()) {
        if (name.rfind("levels/levels/", 0) != 0 || name.size() < 10 || name.substr(name.size() - 5) != ".blvl") continue;
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(name)));
        for (const assets::LevelObject& object : level.objects) {
            const assets::Level* cls = classOf(object.className);
            if (!cls) continue;
            if ((object.className == kWobblyClass || assets::objectKind(object, *cls) == kBuilditKind) && effectiveParam(object, *cls, assets::nameHash("SpawnLoc"))) ++spawnLocs;
            const assets::LevelParam* forced = effectiveParam(object, *cls, assets::nameHash("ForceStudValue"));
            if (forced) {
                float f = 0;
                if (forced->type == 0) std::memcpy(&f, &forced->value, 4);
                auto& frow = forcedClasses[object.className];
                ++frow[0];
                if (f != 0.0f) ++frow[1];
            }
            const assets::LevelParam* value = effectiveParam(object, *cls, assets::nameHash("StudValue"));
            if (!value) continue;
            float count = 0;
            if (value->type == 0) std::memcpy(&count, &value->value, 4);
            auto& row = studClasses[object.className];
            ++row[0];
            if (count != 0.0f) ++row[1];
        }
    }
    require(forcedClasses.empty(), "no class or instance of any level has ForceStudValue, so the callers of 0x22ca88 that force it (0x174eb4, 0x18c3dc, 0x1911c4, 0x1ba884, 0x2aab5c) drop nothing");
    require(spawnLocs == 0, "no wobbly prop or build-it has a SpawnLoc parameter, got " + std::to_string(spawnLocs));
    require(studClasses.at("leprops/leprop_wobblydestructible") == (std::array<std::size_t, 2>{2516, 2513}), "StudValue of the wobbly props (class value 25)");
    require(studClasses.at("props/prop_trafficlight") == (std::array<std::size_t, 2>{98, 0}), "StudValue 0 on every traffic light");
    require(studClasses.at("props/prop_buildit") == (std::array<std::size_t, 2>{87, 82}), "StudValue of the build-its");
    for (const auto& [name, row] : studClasses) require(name.rfind("vehicles/", 0) != 0 && name.rfind("lecars/", 0) != 0, "a vehicle class with StudValue: " + name);

    const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
    WobblyProps props;
    props.load(archive, level, classOf);
    std::size_t dropping = 0, silent = 0;
    for (const assets::LevelObject& object : level.objects) {
        const WobblyProp* prop = props.find(lowerCase(object.instance));
        if (!prop) continue;
        ++(prop->studValue > 0 ? dropping : silent);
        require(prop->studSpawn.spawnDir != 999 && !prop->studSpawn.hasSpawnLoc, "every wobbly prop has a SpawnDir and no SpawnLoc");
    }
    require(dropping == 645 && silent == 29, "lvl_01: 645 wobbly props drop studs on a hit, 29 have WobbleStudValue 0; got " + std::to_string(dropping) + "/" + std::to_string(silent));

    const WobblyProp* table = props.find("donuttable_14");
    require(table != nullptr, "DonutTable_14");
    WobbleHitMessage punch;
    punch.damage = 1;
    const std::optional<WobblyReaction> reaction = props.deliver("donuttable_14", punch);
    require(reaction && reaction->wobbleStuds, "the first punch drops studs");
    const StudDropRequest& drop = *reaction->wobbleStuds;
    require(drop.mode == StudDropMode::Count && drop.count == 2, "two studs in mode 0");
    require(near(drop.position[0], table->node[12]) && near(drop.position[1], table->node[13]) && near(drop.position[2], table->node[14]), "at the node");
    require(table->studSpawn.spawnDir == 0 && near(drop.direction[0], 0) && near(drop.direction[2], 1) && near(drop.spread, 1.0f), "SpawnDir 0 (class value) gives (0, 0, 1), SpawnCone 1.0");

    BuilditSet set;
    set.load(archive, level, classOf);
    require(set.objects().size() == 18, "18 build-its (kind 21: 15 props/prop_buildit, 3 leprops/leprop_buildit) in lvl_01");
    const BuilditObject* parts = set.find("1-1_bi_donutvanparts");
    require(parts != nullptr, "1-1_BI_DonutVanParts");
    const std::optional<StudDropRequest> finish = builditFinishStuds(*parts);
    require(finish && finish->mode == StudDropMode::Random && finish->count == 25, "the finished van parts drop 25 studs in mode 1");
    require(std::fabs(finish->position[0] - 869.439f) < 0.01f && std::fabs(finish->position[1] - 51.9511f) < 0.01f && std::fabs(finish->position[2] - 691.862f) < 0.01f,
            "at the placement of 1-1_Donut_Van_Body");
    require(near(finish->direction[0], 0) && near(finish->direction[2], 1), "SpawnDir 0");
    const BuilditObject* trampoline = set.find("trampoline_03_buildits");
    require(trampoline && trampoline->studSpawn.spawnDir == 3, "Trampoline_03_Buildits has SpawnDir 3 (instance value)");
}

}

int main(int argc, char** argv) {
    try {
        requests();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "stud-callers-test: data not available, skipped\n";
                return 77;
            }
            realData(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

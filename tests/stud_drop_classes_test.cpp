// Tests of the stud drops by class (src/runtime/stud_drop_classes.*), docs/re/studs.md entry "Which classes drop studs". Read in llvm-objdump of
// artifacts/game/game.arm.elf: 0x22c6d0 / 0x22c8a0 / 0x22ca88 / 0x2451d0 (the generic state machine and its count), 0x20bd18 (player hurt), 0x26d1f0 (player
// death), 0x2b7894 (hit-sequence slots), the reward sites 0x186158 / 0x1b062c / 0x2bd814 / 0x319fe0 / 0x28d8e4. The real-data counts are the placed objects
// of the level files of the FIB, counted with this port's loaders (this test, 2026-10-20).
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/stud_drop_classes.hpp"

#include <cmath>
#include <cstring>
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

assets::Level classWith(std::uint32_t type, std::uint32_t value, std::uint16_t flags = 1) {
    assets::Level cls;
    assets::LevelParam param;
    param.hash = assets::nameHash("StudValue");
    param.flags = flags;
    param.type = std::uint16_t(type);
    param.value = value;
    cls.defaults.push_back(param);
    return cls;
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    return bits;
}

void classes() {
    for (std::uint8_t kind : {38, 58, 100, 125, 138, 139, 141, 185, 186, 222}) require(runsGenericStudStateMachine(kind), "kind " + std::to_string(kind));
    for (std::uint8_t kind : {0, 21, 27, 45, 67, 102, 119, 183, 191, 195, 204}) require(!runsGenericStudStateMachine(kind), "kind " + std::to_string(kind));
    require(genericStateChangeDropsStuds(2) && genericStateChangeDropsStuds(3), "0x22c754 cmp #2, cmpne #3");
    for (std::uint16_t target : {0, 1, 4, 5, 6}) require(!genericStateChangeDropsStuds(target), "target " + std::to_string(target));
}

void parameter() {
    const assets::Level none;
    const assets::LevelObject plain;
    require(studValueParameter(plain, none, 25) == 25 && studValueParameter(plain, none, 0) == 0, "a missing StudValue gives the fallback (0x2451d0 r2)");
    require(studValueParameter(plain, classWith(0, bitsOf(25.0f)), 0) == 25, "float 25 casts to 25");
    require(studValueParameter(plain, classWith(0, bitsOf(2.9f)), 0) == 2 && studValueParameter(plain, classWith(0, bitsOf(-2.9f)), 0) == -2, "vcvt truncates towards zero");
    require(studValueParameter(plain, classWith(0, bitsOf(3e10f)), 0) == 2147483647, "vcvt saturates");
    require(studValueParameter(plain, classWith(5, 42), 0) == 42 && studValueParameter(plain, classWith(7, 0xfffffffeu), 0) == -2, "types 5 and 7 return the word");
    require(studValueParameter(plain, classWith(3, 42), 7) == 7, "other types give the fallback");
    assets::LevelObject own;
    assets::LevelParam instance;
    instance.hash = assets::nameHash("StudValue");
    instance.type = 0;
    instance.value = bitsOf(8.0f);
    own.params.push_back(instance);
    require(studValueParameter(own, classWith(0, bitsOf(25.0f), 3), 0) == 8, "a per-instance class parameter takes the instance's value (0x217080)");
    require(studValueParameter(own, classWith(0, bitsOf(25.0f), 1), 0) == 25, "an inline class value ignores the instance");
}

void requests() {
    const StudSpawnSource plain;
    const std::optional<StudDropRequest> generic = genericStateStudRequest(plain, 25, kNode);
    require(generic && generic->mode == StudDropMode::Random && generic->count == 25, "0x22c8a0: mode 1 (mode flag 0)");
    require(near(generic->position[0], 5) && near(generic->position[2], 7) && near(generic->direction[2], 1), "at the node, along its row 2 (forward 1)");
    require(!genericStateStudRequest(plain, 0, kNode) && !genericStateStudRequest(plain, -3, kNode), "count 0 drops nothing");
    StudSpawnSource located;
    located.hasSpawnLoc = true;
    located.spawnLocNode = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 9, 9, 9, 1};
    const std::optional<StudDropRequest> atLocation = genericStateStudRequest(located, 3, kNode);
    require(atLocation && near(atLocation->position[0], 9) && near(atLocation->direction[0], 1), "SpawnLoc replaces the node");

    require(studRewardSite(191) && studRewardSite(191)->forward && studRewardSite(195)->forward, "safecrack and wiretap pass forward 1");
    require(studRewardSite(150) && !studRewardSite(150)->forward && !studRewardSite(66)->forward && !studRewardSite(183)->forward, "jetpack pad, projectile and vehicle pass forward 0");
    require(!studRewardSite(185) && !studRewardSite(67), "no other kind");
    const std::optional<StudDropRequest> safe = studRewardRequest(*studRewardSite(191), plain, kStudRewardFallback, kNode);
    require(safe && safe->mode == StudDropMode::Random && safe->count == 25 && near(safe->direction[2], 1), "a safe drops 25 studs forward in mode 1");
    const std::optional<StudDropRequest> pad = studRewardRequest(*studRewardSite(150), plain, 25, kNode);
    require(pad && pad->mode == StudDropMode::Random && near(pad->direction[0], 0) && near(pad->direction[1], 0) && near(pad->direction[2], 0), "forward 0 without SpawnDir: no direction");
    StudSpawnSource aimed;
    aimed.spawnDir = 90;
    const std::optional<StudDropRequest> aimedPad = studRewardRequest(*studRewardSite(150), aimed, 25, kNode);
    require(aimedPad && near(aimedPad->direction[0], 1), "SpawnDir applies with forward 0");
}

void playerRequests() {
    const StudDropRequest hurt = playerHurtStudRequest({1, 2, 3});
    require(hurt.mode == StudDropMode::Count && hurt.count == 30 && hurt.skip == 30 && !hurt.directionIsVelocity && !hurt.collectAtOnce, "0x20bd18: mode 0, count 30, skip 30");
    require(near(hurt.spread, 1.0f) && near(hurt.position[0], 1) && near(hurt.position[2], 3) && hurt.direction == DebrisVec3{0, 0, 0}, "s0 = 1.0, the node, no direction");

    struct Row { std::uint64_t counter; unsigned count; };
    const Row rows[] = {{0, 0}, {9, 0}, {10, 1}, {99, 9}, {100, 10}, {190, 19}, {290, 28}, {500, 50}, {890, 82}, {990, 91}, {1000, 100}, {1090, 109}, {1100, 110},
                        {1190, 110}, {18446744073709551615ull, 110}};
    for (const Row& row : rows) {
        const PlayerDeathStudLoss loss = playerDeathStudLoss(row.counter, {4, 5, 6});
        require(loss.count == row.count, "0x26d1f0 count for " + std::to_string(row.counter) + ": " + std::to_string(loss.count));
        require(loss.counterAfter == row.counter - std::uint64_t(row.count) * 10, "the counter loses 10 per unit: " + std::to_string(row.counter));
        require(loss.noMultiplier && bool(loss.request) == (row.count != 0), "a drop exactly when the count is not 0");
        if (!loss.request) continue;
        require(loss.request->mode == StudDropMode::Count && loss.request->count == row.count && loss.request->skip == 30, "mode 0, skip 30");
        require(near(loss.request->position[0], 4) && near(loss.request->position[2], 6) && loss.request->direction == DebrisVec3{0, 0, 0}, "the node, no direction");
    }
}

void hitSequence() {
    const StudSpawnSource plain;
    HitSequenceStudInput input;
    input.slots[0] = {0, 3, 4, 255};
    input.counter = 0;
    input.health = 2;
    input.clipFrame = 4.5f;
    input.boxHalfX = 1.0f;
    input.boxHalfZ = 1.0f;
    const std::optional<StudDropRequest> fired = hitSequenceStudRequest(input, plain, kNode);
    require(fired && fired->mode == StudDropMode::Count && fired->count == 3, "0x2b7ac4..0x2b7b00: the slot's StudSpawnValue in mode 0");
    require(near(fired->direction[0], 0) && near(fired->direction[1], 0) && near(fired->direction[2], 0), "forward 0 for a box that is not longer than 2 x its depth");
    input.boxHalfX = 2.5f;
    const std::optional<StudDropRequest> longBox = hitSequenceStudRequest(input, plain, kNode);
    require(longBox && near(longBox->direction[2], 1), "forward 1 when half x > 2 half z");
    input.boxHalfX = 2.0f;
    require(near(hitSequenceStudRequest(input, plain, kNode)->direction[2], 0), "half x = 2 half z is not longer");
    input.clipFrame = 3.9f;
    require(!hitSequenceStudRequest(input, plain, kNode), "before the frame");
    input.clipFrame = 5.0f;
    require(!hitSequenceStudRequest(input, plain, kNode), "frame + 1.0 is exclusive");
    input.clipFrame = 4.0f;
    require(bool(hitSequenceStudRequest(input, plain, kNode)), "the frame itself fires");
    input.health = 1;
    require(!hitSequenceStudRequest(input, plain, kNode), "health 1 skips the sound and the studs (0x2b7a7c beq 0x2b7b1c)");
    input.health = 2;
    input.hasClip = false;
    require(!hitSequenceStudRequest(input, plain, kNode), "no clip: 0x23ad24 returns 0, 0x2b78b0 leaves");
    input.hasClip = true;
    input.slots[0].value = 0;
    require(!hitSequenceStudRequest(input, plain, kNode), "StudSpawnValue 0");
    input.slots[0].value = 3;
    input.slots[0].frame = 255;
    require(!hitSequenceStudRequest(input, plain, kNode), "no frame");

    HitSequenceStudInput later;
    later.slots[0] = {0, 1, 2, 255};
    later.slots[1] = {0, 4, 6, 255};
    later.slots[2] = {0, 0, 255, 255};
    later.health = 3;
    later.clipFrame = 6.2f;
    later.counter = 0;
    require(!hitSequenceStudRequest(later, plain, kNode), "counter 0 reads slot 0 only: frame 2 has passed");
    later.counter = 1;
    const std::optional<StudDropRequest> second = hitSequenceStudRequest(later, plain, kNode);
    require(second && second->count == 4, "counter 1 reads slot 1");
    later.counter = 3;
    const std::optional<StudDropRequest> past = hitSequenceStudRequest(later, plain, kNode);
    require(past && past->count == 4, "a counter past the last slot in use stays on the last one with a frame and a value");
}


struct Census {
    std::size_t objects = 0, dropping = 0;
    bool operator==(const Census& other) const { return objects == other.objects && dropping == other.dropping; }
};

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    std::map<unsigned, Census> generic, rewards;
    std::map<std::string, Census> classRows;
    std::size_t negative = 0;
    for (const std::string& name : archive.names()) {
        if (name.rfind("levels/levels/", 0) != 0 || name.size() < 10 || name.substr(name.size() - 5) != ".blvl") continue;
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(name)));
        for (const assets::LevelObject& object : level.objects) {
            const assets::Level* cls = classOf(object.className);
            if (!cls) continue;
            negative += studValueParameter(object, *cls, 0) < 0;
            const std::uint8_t kind = assets::objectKind(object, *cls);
            if (runsGenericStudStateMachine(kind)) {
                const bool drops = studValueParameter(object, *cls, 0) > 0;
                ++generic[kind].objects;
                generic[kind].dropping += drops;
                ++classRows[object.className].objects;
                classRows[object.className].dropping += drops;
            } else if (studRewardSite(kind)) {
                ++rewards[kind].objects;
                rewards[kind].dropping += studValueParameter(object, *cls, kStudRewardFallback) > 0;
            }
        }
    }
    require(negative == 0, "no placed object has a negative StudValue (STUD-S14)");
    const std::map<unsigned, Census> expectedGeneric = {{38, {17, 0}}, {58, {171, 66}}, {100, {18, 12}}, {138, {163, 0}}, {139, {15, 0}},
                                                        {141, {4, 0}}, {185, {233, 215}}, {186, {14, 0}}, {222, {8, 0}}};
    require(generic == expectedGeneric, "objects of the generic state machine kinds and those with StudValue above 0 (kind 125 is placed nowhere)");
    require((classRows.at("props/ability_axe_door") == Census{21, 21}) && (classRows.at("props/ability_electricrepair") == Census{33, 25}) &&
                (classRows.at("props/prop_switchkey_vertical") == Census{12, 7}) && (classRows.at("props/prop_switchpull") == Census{45, 0}) &&
                (classRows.at("props/prop_fire_4x4") == Census{45, 0}),
            "per class: the ability props drop, the fires and the pull switches have no StudValue");
    const std::map<unsigned, Census> expectedRewards = {{150, {45, 43}}, {183, {276, 276}}, {191, {20, 19}}, {195, {13, 6}}};
    require(rewards == expectedRewards, "objects of the reward kinds (the fallback 25 where StudValue is missing); kind 66 projectiles are nested objects and not counted");
}

}

int main(int argc, char** argv) {
    try {
        classes();
        parameter();
        requests();
        playerRequests();
        hitSequence();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "stud-drop-classes-test: data not available, skipped\n";
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

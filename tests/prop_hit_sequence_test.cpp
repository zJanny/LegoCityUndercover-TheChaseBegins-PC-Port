// Tests of message 0 of the ObjectType 67 props (src/runtime/prop_hit_sequence.*), docs/re/vehicles-prop-smash.md entry "The hit machine of
// ObjectType 67". Read in REA (Ghidra) pseudo code and llvm-objdump of artifacts/game/game.arm.elf: the handler 0x15e408 (0x15e594..0x15e8f8),
// the hit machine 0x2398d8 case 0 (0x239958, jump table 0x23998c), the create 0x15a4b4 (`Destructable`) and 0x16a268 (`Health` for the modes
// 1, 3, 4, 7, 11; `ProjectileType`). The vehicle message is 0x275e74 (damage r11 = 100, byte +0x14 r9 = 11). Real-data values: the level data
// of lvl_01 through the loaders of this port (this test, 2026-10-20); the oracle grid census of veh-smash-prop (servant answer r41-A-2,
// artifacts/oracle/r41-servant-out/a2_load1.txt: record +0xcc / +0xd4 = 1 / 1 for five kind 67 entries, 11 / 3 for two).
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "assets/object_collision.hpp"
#include "runtime/prop_hit_sequence.hpp"

#include <cmath>

#include <cstdio>
#include <vector>
#include <filesystem>
#include <map>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (condition) return;
    std::printf("FAIL: %s\n", message.c_str());
    ++failures;
}

HitMachineProp propOf(std::uint8_t mode, std::int8_t health) {
    HitMachineProp prop;
    prop.mode = mode;
    prop.health = health;
    return prop;
}

void modes() {
    const HitMessage car = vehicleSmashMessage();
    expect(car.damage == 100 && car.hitKind == 11 && car.projectile == 0 && car.sequence == 0 && car.word0 == 0 && car.senderKind == 183 &&
               !car.senderIsCharacter,
           "the vehicle's message: damage 100, byte +0x14 = 11, +0x15 = +0x16 = 0, sender kind 183, no character (0x275e74, oracle A-1)");

    for (std::uint8_t mode : {std::uint8_t(1), std::uint8_t(4), std::uint8_t(7)}) {
        HitMachineProp prop = propOf(mode, 1);
        expect(hitMachineMessage0(prop, car) == HitOutcome::Destroyed && prop.health == 0, "modes 1, 4 and 7: health 1 - 100 destroys (0x2399e8, 0x239f54)");
    }
    HitMachineProp tough = propOf(1, 120);
    expect(hitMachineMessage0(tough, car) == HitOutcome::Damaged && tough.health == 20, "health 120 - 100 = 20 stays above 0 (byte subtract, sxtb)");
    HitMachineProp wrapped = propOf(1, -56);
    expect(hitMachineMessage0(wrapped, car) == HitOutcome::Damaged && wrapped.health == 100, "Health 200 is the byte -56; -56 - 100 wraps to 100 (sxtb)");

    HitMessage typed = car;
    typed.word0 = 1;
    HitMachineProp untyped = propOf(7, 1);
    expect(hitMachineMessage0(untyped, typed) == HitOutcome::Ignored && untyped.health == 1, "mode 7 ignores a message whose word +0 is not 0 (0x2399bc)");

    HitMachineProp projectile = propOf(1, 1);
    projectile.projectileType = 3;
    expect(hitMachineMessage0(projectile, car) == HitOutcome::Ignored, "a ProjectileType that is not the message byte +0x15 ignores the hit (0x2399c8)");
    projectile.projectileType = 0;
    expect(hitMachineMessage0(projectile, car) == HitOutcome::Destroyed, "ProjectileType 0 equals the car's byte +0x15 = 0");
    HitMessage thrown = car;
    thrown.projectile = 3;
    HitMachineProp any = propOf(1, 1);
    expect(hitMachineMessage0(any, thrown) == HitOutcome::Ignored, "a message byte +0x15 of 1..6 is ignored by 0x15e5d8");

    for (std::uint8_t mode : {std::uint8_t(0), std::uint8_t(2), std::uint8_t(6), std::uint8_t(8), std::uint8_t(51)}) {
        HitMachineProp prop = propOf(mode, 0);
        expect(hitMachineMessage0(prop, car) == HitOutcome::Ignored, "modes 0, 2, 6, 8 and 51 have no case 0 action (jump table 0x23998c, >= 12)");
    }
    HitMachineProp fromProp = propOf(5, 0);
    expect(hitMachineMessage0(fromProp, car) == HitOutcome::Ignored, "mode 5 needs a sender of kind 67 (0x239a20)");
    HitMessage propSender = car;
    propSender.senderKind = 67;
    expect(hitMachineMessage0(fromProp, propSender) == HitOutcome::Destroyed, "mode 5 with a kind 67 sender destroys");

    HitMachineProp ability = propOf(11, 3);
    ability.abilityDestructable = 27;
    expect(hitMachineMessage0(ability, car) == HitOutcome::Ignored && ability.health == 3,
           "AbilityDestructable: a sender that is no character (0x245b00) is ignored, so a car never reaches the hit sequence");
    HitMessage miner = car;
    miner.senderIsCharacter = true;
    miner.senderKind = 27;
    expect(hitMachineMessage0(ability, miner) == HitOutcome::NotPorted, "a character sender of an AbilityDestructable prop: 0x23a70c is not read");
    HitMachineProp sequence = propOf(11, 3);
    expect(hitMachineMessage0(sequence, car) == HitOutcome::NotPorted, "mode 11 without AbilityDestructable runs the hit sequence 0x239c40 (not ported)");
    HitMachineProp lights = propOf(3, 2);
    expect(hitMachineMessage0(lights, car) == HitOutcome::NotPorted, "mode 3 (lights, area damage 0x239a30) is not ported");

    HitMachineProp characterHit = propOf(50, 0);
    expect(hitMachineMessage0(characterHit, car) == HitOutcome::Ignored, "mode 50 reacts to a kind 27 sender only");
    HitMessage character = car;
    character.senderKind = 27;
    character.damage = 1;
    expect(hitMachineMessage0(characterHit, character) == HitOutcome::Destroyed, "mode 50: the record health +0xd4 is 0 (not read for 50), 0 - 1 destroys");
    HitMachineProp hitKind2 = propOf(52, 0);
    HitMessage kind2 = car;
    kind2.hitKind = 2;
    expect(hitMachineMessage0(hitKind2, kind2) == HitOutcome::Destroyed, "mode 52 reacts to message byte +0x14 = 2 (0x15e74c)");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    if (!entry) {
        expect(false, "lvl_01 not in the archive");
        return;
    }
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    std::map<std::string, int> modes;
    int destroyedByCar = 0;
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != 67) continue;
        HitMachineProp prop = hitMachineProp(object, *cls);
        ++modes[object.className + " " + std::to_string(prop.mode)];
        if (hitMachineMessage0(prop, vehicleSmashMessage()) == HitOutcome::Destroyed) ++destroyedByCar;
    }
    expect(modes["props/prop_prop 0"] == 12 && modes["props/prop_prop 1"] == 4, "lvl_01 props/prop_prop: 12 of mode 0, 4 of mode 1 (Destructable)");
    expect(modes["leprops/leprop_prop 0"] == 3 && modes["leprops/leprop_prop 1"] == 1, "lvl_01 leprops/leprop_prop: 3 of mode 0, 1 of mode 1");
    expect(modes["props/ability_miner_rocksmash 11"] == 2, "the two rock smashes are mode 11 (the oracle records +0xcc = 11, +0xd4 = 3)");
    expect(destroyedByCar == 5, "a car's message destroys the five mode 1 objects of kind 67 in lvl_01 (Health 1); the grid census holds five such records");

    struct OracleShape {
        const char* instance;
        bool mesh;
        float centre[3];
        float half[3];
    };
    const OracleShape oracle[] = {
        {"1-2_RoofProp_03", true, {0.0f, 1.2194f, -0.0002f}, {0.1288f, 1.2194f, 0.7670f}},
        {"1-1_Donut_Van_Body", false, {0.0f, 3.1582f, 0.4652f}, {1.4859f, 2.6922f, 2.3552f}},
        {"1-2_Dog", false, {0.0f, 0.7476f, 0.1161f}, {0.1908f, 0.7476f, 0.8430f}},
        {"1-3_crane_built", true, {-0.0059f, 1.8848f, 1.9740f}, {0.5797f, 1.8848f, 2.3548f}},
        {"1-3_Crane_Girder", false, {0.0f, 0.3810f, 0.0625f}, {0.3810f, 0.3810f, 6.8573f}},
        {"1-3_Lawnmower_Final", false, {0.0f, 0.9817f, 0.0f}, {1.3152f, 1.0140f, 1.9700f}},
        {"1-4_Window", false, {0.0f, 2.0273f, 0.0f}, {0.0334f, 2.0273f, 2.7637f}},
        {"1-4_WindowSmashed", false, {0.0f, 2.0273f, 0.0f}, {0.0500f, 2.0273f, 2.7637f}},
        {"1-4_Roadblock_12", true, {0.0f, 0.0801f, -0.0953f}, {0.8087f, 0.8260f, 2.7641f}},
        {"FP_RockSmash_PoliceGarage_01", true, {-0.4362f, 0.8007f, 0.3487f}, {1.6091f, 0.8007f, 2.1802f}},
    };
    const std::vector<assets::ObjectCollision> candidates = assets::objectCollisionCandidates(archive, level);
    for (const OracleShape& shape : oracle) {
        const assets::ObjectCollision* found = nullptr;
        for (const assets::ObjectCollision& candidate : candidates)
            if (candidate.instance == shape.instance) found = &candidate;
        bool same = found && bool(found->mesh) == shape.mesh;
        for (int axis = 0; same && axis < 3; ++axis)
            same = std::fabs(found->centre[axis] - shape.centre[axis]) < 1e-3f && std::fabs(found->half[axis] - shape.half[axis]) < 1e-3f;
        expect(same, std::string("the grid entry of ") + shape.instance + " is the oracle's (entry kind, entity box +0x60 / +0x6c; r41-A-2 / A-3)");
    }
    bool sea = false;
    for (const assets::ObjectCollision& candidate : candidates) sea = sea || candidate.instance == "Sea";
    expect(!sea, "Sea (NoBoundCollisions, no item) has no grid entry, as in the oracle census");
}

}

int main(int argc, char** argv) {
    modes();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("prop-hit-sequence-test: data not available, skipped\n");
            return 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures != 0) return 1;
    std::printf("prop hit sequence tests passed\n");
    return 0;
}

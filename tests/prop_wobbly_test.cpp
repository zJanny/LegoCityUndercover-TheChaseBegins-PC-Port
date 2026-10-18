// The class leprops/leprop_wobblydestructible (src/runtime/prop_wobbly_destructible.*; handler 0x2a3390, destroy 0x23cd80, base handler 0x1f8948).
// Recorded values: Azahar oracle, user state m12-table-before (New 3DS), the table DonutTable_14 punched with Y (button held 3 updates, then 26 updates
// of waiting, five times; scratch scenario h1..h8 of agent r23-D, `ctrace 0x31a38c,0x23cd80,0x16b958,0x244f40,0x23211c,0x23cee8` and `read` of the
// record +0x10 at 0x08c9adf0): the health halfword reads 4, 3, 2, 1, 0 after the five punches (WobbleHealth 5, damage 1); the first four punches
// call the spring 0x16b958 and the event 0x23cee8 (*0x3775e0 = ObjectHit), the fifth calls 0x23cee8, 0x23cd80, 0x23cee8 (*0x3775c0 = Dead) and then 0x244f40
// on the object of TriggerObject followed by 0x23cee8 (*0x3775d0 = ObjectTriggered). The phone box of the vehicle smash (health 4, damage 100) reads
// 0xffa0 (docs/re/vehicles-physics-props.md). With data (FIB NAMES) it checks the table's class values and the chain to the build-it of
// Trampoline_03: the level start disables Trampoline_03_Buildits and Trampoline_03, the destroyed table enables the build-it through DonutTable_14_Script and
// the finished build switches Trampoline_03 on. Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/level_start.hpp"
#include "runtime/prop_wobbly_destructible.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void rules() {
    WobbleHitMessage punch;
    punch.damage = 1;
    std::int16_t health = 5;
    const std::int16_t recorded[5] = {4, 3, 2, 1, 0};
    for (int hit = 0; hit < 5; ++hit) {
        const WobbleHitResult result = wobblyHit(health, punch, false);
        require(!result.dropped && result.hitEvent, "the punch is handled and raises ObjectHit");
        require(result.health == recorded[hit], "health after punch " + std::to_string(hit + 1));
        require(result.wobble == (hit < 4) && result.destroyed == (hit == 4), "wobble for punches 1 to 4, destroy for punch 5");
        health = result.health;
    }

    WobbleHitMessage car;
    car.senderIsPlayer = false;
    car.senderType = 183;
    car.damage = 100;
    const WobbleHitResult smashed = wobblyHit(4, car, false);
    require(smashed.destroyed && smashed.health == -96, "health 4 minus 100 is -96 (record 0xffa0 in the oracle)");
    require(wobblyHit(4, car, true).dropped, "a sender other than the player is dropped while the prop wobbles (0x1c54ac)");
    require(!wobblyHit(4, punch, true).dropped, "the player's hit is not dropped while the prop wobbles");
    WobbleHitMessage zero = punch;
    zero.damage = 0;
    require(wobblyHit(4, zero, false).dropped, "damage 0 is dropped");
    WobbleHitMessage buddy;
    buddy.senderIsPlayer = false;
    buddy.senderType = kCharacterTypeByte;
    buddy.damage = 1;
    const WobbleHitResult killed = wobblyHit(5, buddy, false);
    require(killed.destroyed && killed.health == 0, "another sender of type 27 sets the health to 0");
    WobbleHitMessage nobody;
    nobody.senderPresent = false;
    nobody.senderIsPlayer = false;
    nobody.senderType = kCharacterTypeByte;
    nobody.damage = 1;
    require(wobblyHit(5, nobody, false).wobble, "a message without sender skips the type test");
}

constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find(kLevelFile);
    require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

    WobblyProps props;
    props.load(archive, level, classOf);
    require(props.size() == 674, "674 wobbly props in lvl_01, got " + std::to_string(props.size()));
    const WobblyProp* table = props.find("donuttable_14");
    require(table != nullptr, "DonutTable_14 is a wobbly prop");
    require(table->health == 5, "WobbleHealth 5 (instance value)");
    require(table->triggerObject == "donuttable_14_script", "TriggerObject " + table->triggerObject);
    require(table->burst.count == 1, "WobbleDebrisAmount 1 (instance value)");
    require(table->destroyEffects.debrisAmount == 16, "DebrisAmount 16 (instance value)");
    require(table->studValue == 2, "WobbleStudValue 2 (class default)");
    require(table->soundHit == 0xe3 && table->soundDestroyed == 0x22d, "ATTR_SFX_HIT 0xe3, ATTR_SFX_DESTROYED 0x22d (class defaults)");
    require(table->boundsHalf[1] > 0.1f, "the model bounds are read");

    WobbleHitMessage punch;
    punch.damage = 1;
    for (int hit = 1; hit <= 5; ++hit) {
        const std::optional<WobblyReaction> reaction = props.deliver("donuttable_14", punch);
        require(reaction.has_value(), "the table takes punch " + std::to_string(hit));
        if (hit < 5) {
            require(reaction->hit.wobble && !reaction->hit.destroyed && reaction->triggerObject.empty(), "punch " + std::to_string(hit) + " wobbles");
            require(reaction->wobbleBricks && reaction->wobbleBricks->count == 1, "one brick per wobbling hit");
            require(reaction->sound == 0xe3 && reaction->wobbleStudValue == 2, "hit sound and the studs of the hit");
        } else {
            require(reaction->hit.destroyed && reaction->triggerObject == "donuttable_14_script", "punch 5 destroys the table and names its script object");
            require(reaction->sound == 0x22d, "destroy sound");
        }
    }
    require(!props.deliver("donuttable_14", punch).has_value(), "a destroyed table takes no more hits");

    assets::PlacedObjectState state = levelStartState(level, classOf);
    require(state.isDisabled("Trampoline_03_Buildits") && state.isDisabled("Trampoline_03"), "the level start hides the build-it and the trampoline");
    require(!state.isDisabled("DonutTable_14"), "the table stands at the start");
    ScriptRunner scripts(level, state);
    BuilditSet set;
    set.load(archive, level, classOf);
    const BuilditObject* trampoline = set.find("trampoline_03_buildits");
    require(trampoline != nullptr && trampoline->triggerObject == "trampoline_03_buildits_script", "Trampoline_03_Buildits is a build-it with the script object as TriggerObject");
    require(trampoline->build.progress.size() > 0 && trampoline->hasZone, "its bricks and use zone");

    scripts.triggerObjectEnabling("donuttable_14_script");
    for (int i = 0; i < 4; ++i) {
        scripts.update(0.034f);
        scripts.checkTriggers(trampoline->position);
    }
    require(!state.isDisabled("Trampoline_03_Buildits"), "DonutTable_14_Script enables the build-it");
    require(state.isDisabled("Trampoline_03"), "the trampoline stays off until the build is done");

    float standing[3];
    for (int a = 0; a < 3; ++a) standing[a] = trampoline->position[a] + trampoline->zoneCentre[a];
    bool built = false;
    for (int update = 1; update <= 400 && !built; ++update) {
        const auto actions = set.update(state, standing, true, false);
        applyBuilditActions(set, actions, state, scripts);
        scripts.update(0.034f);
        scripts.checkTriggers(standing);
        for (const BuilditAction& action : actions) built = built || action.kind == BuilditActionKind::Built;
    }
    require(built, "the build-it finishes");
    for (int i = 0; i < 4; ++i) {
        scripts.update(0.034f);
        scripts.checkTriggers(standing);
    }
    require(!state.isDisabled("Trampoline_03"), "Trampoline_03_Buildits_Script switches the trampoline on");
    require(state.isDisabled("1-2_Temp_Trampoline"), "and the temporary trampoline off");
}
}

int main(int argc, char** argv) {
    try {
        rules();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "prop-wobbly-test: data not available, skipped\n";
                return 77;
            }
            realData(argv[1], argv[2]);
        } else if (argc != 1) {
            std::cerr << "usage: prop-wobbly-test [FIB NAMES]\n";
            return 2;
        }
    } catch (const std::exception& error) {
        std::cerr << "prop-wobbly-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-wobbly-test: ok\n";
    return 0;
}

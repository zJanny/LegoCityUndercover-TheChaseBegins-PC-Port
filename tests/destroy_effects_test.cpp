// Which routine of message 1 makes the visible effects of a destroyed entity per kind (src/runtime/destroy_message.*, src/runtime/prop_destroy.*,
// VehiclePropSmasher::destroyData), docs/re/script-natives-world.md entry 2026-10-19T10:02:26. Read in llvm-objdump of artifacts/game/game.arm.elf:
// the hit machine case 1 at 0x239f8c, the pushable 0x1a958c, the default handler 0x221760 -> 0x163f00. Real-data values: a throwaway C++ census over the
// 30 level files (DestroyProp / KillObject targets with the class parameters, 2026-10-19, round 28 agent C). Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/destroy_message.hpp"
#include "runtime/vehicle_prop_smash.hpp"

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const std::string& what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what.c_str());
        ++failures;
    }
}

void sets() {
    for (const int kind : {85, 119, 134, 214}) expect(destroyEffectSet(std::uint8_t(kind)) == DestroyEffectSet::BaseProp, "base prop handler kind " + std::to_string(kind));
    for (const int kind : {67, 204}) expect(destroyEffectSet(std::uint8_t(kind)) == DestroyEffectSet::HitMachine, "hit machine kind " + std::to_string(kind));
    expect(destroyEffectSet(71) == DestroyEffectSet::Pushable, "pushable");
    for (const int kind : {64, 132}) expect(destroyEffectSet(std::uint8_t(kind)) == DestroyEffectSet::DefaultHandler, "default handler kind " + std::to_string(kind));
    for (const int kind : {27, 121, 242}) expect(destroyEffectSet(std::uint8_t(kind)) == DestroyEffectSet::CharacterFallApart, "character kind " + std::to_string(kind));
    expect(destroyEffectSet(102) == DestroyEffectSet::Topple, "topple");
    for (const int kind : {1, 42, 58, 183, 255}) expect(destroyEffectSet(std::uint8_t(kind)) == DestroyEffectSet::None, "no effects kind " + std::to_string(kind));
    expect(destroyEffectsPorted(DestroyEffectSet::HitMachine) && destroyEffectsPorted(DestroyEffectSet::DefaultHandler) && destroyEffectsPorted(DestroyEffectSet::Pushable) &&
               destroyEffectsPorted(DestroyEffectSet::BaseProp),
           "ported sets");
    expect(!destroyEffectsPorted(DestroyEffectSet::CharacterFallApart) && !destroyEffectsPorted(DestroyEffectSet::Topple) && !destroyEffectsPorted(DestroyEffectSet::None),
           "not ported sets");
}

assets::Level loadLevel(assets::FuseArchive& archive, const char* path) {
    const assets::FuseEntry* entry = archive.find(path);
    if (!entry) {
        std::printf("FAIL: not in archive: %s\n", path);
        ++failures;
        return {};
    }
    return assets::parseLevel(archive.read(*entry));
}

VehiclePropSmasher loaded(assets::FuseArchive& archive, const assets::Level& level) {
    VehiclePropSmasher smasher;
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    smasher.load(level, classOf, assets::objectCollisionCandidates(archive, level));
    smasher.loadScriptDestroyed(level, classOf, assets::objectPlacements(level, classOf), [&archive](const assets::Placement& placement) -> std::optional<SmashBox> {
        float centre[3], half[3];
        if (!assets::modelBounds(archive, assets::meshModelPath(placement.mesh), centre, half)) return std::nullopt;
        return SmashBox{{centre[0], centre[1], centre[2]}, {half[0], half[1], half[2]}};
    });
    return smasher;
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    {
        const assets::Level level = loadLevel(archive, "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        const VehiclePropSmasher smasher = loaded(archive, level);
        const auto crate = smasher.destroyData("1-3_smash_crate");
        expect(crate.has_value(), "1-3_smash_crate (leprops/leprop_prop, kind 67) has destroy data");
        if (crate) {
            const PropDestroyEffects& e = crate->effects;
            expect(e.explosionParticle == 10 && e.spawnDebris && e.debrisAmount == 2 && e.colour == 0xff666666u && e.secondColour == 0xff00d7ffu && e.sound == 557,
                   "1-3_smash_crate: ExplosionParticleDef 10, SpawnDebris 1, DebrisAmount 2, DebrisColour (102, 102, 102), DebrisColour2 (255, 215, 0), ATTR_SFX_DESTROYED 557");
        }
        const auto skylight = smasher.destroyData("1-4_showroom_skylight");
        expect(skylight.has_value(), "1-4_Showroom_Skylight (props/prop_prop, kind 67) has destroy data");
        if (skylight)
            expect(skylight->effects.explosionParticle == 10 && !skylight->effects.spawnDebris && skylight->effects.sound == 557,
                   "1-4_Showroom_Skylight: ExplosionParticleDef 10, no SpawnDebris, ATTR_SFX_DESTROYED 557");
        expect(!smasher.destroyData("1-4_police_car"), "a vehicle (kind 183) has no prop destroy data");
        expect(!smasher.destroyData("nobody"), "an unknown instance has none");
    }
    {
        const assets::Level level = loadLevel(archive, "levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl");
        const VehiclePropSmasher smasher = loaded(archive, level);
        const auto platform = smasher.destroyData("4-1_crate_faller_platform");
        expect(platform.has_value(), "4-1_Crate_Faller_Platform (props/prop_platform, kind 64) has destroy data");
        if (platform) {
            const PropDestroyEffects& e = platform->effects;
            expect(e.explosionParticle == 10 && e.spawnDebris && e.debrisAmount == kDebrisDefaultCount && e.colour == 0xff203c51u && e.secondColour == 0xff203c51u,
                   "4-1_Crate_Faller_Platform: ExplosionParticleDef 10, SpawnDebris 1, no DebrisAmount, DebrisColour and DebrisColour2 (81, 60, 32)");
            expect(e.sound == kDefaultDestroySound && e.sound == 29, "the default handler's sound is the initial halfword 29 of 0x377160 + 6, not ATTR_SFX_DESTROYED (the class has none)");
        }
    }
    {
        const assets::Level level = loadLevel(archive, "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl");
        const VehiclePropSmasher smasher = loaded(archive, level);
        const auto plank = smasher.destroyData("6-1_plankwall");
        expect(plank.has_value() && plank->effects.explosionParticle == 18 && plank->effects.sound == 557, "6-1_PlankWall (kind 67): ExplosionParticleDef 18, sound 557");
        const auto rock = smasher.destroyData("6-9_silverboulderpile01");
        expect(rock.has_value() && rock->effects.explosionParticle == 10, "6-9_SilverBoulderPile01 (props/ability_rocksmash, kind 67): ExplosionParticleDef 10");
        expect(!smasher.destroyData("6-4_grapplecrate"), "6-4_GrappleCrate (kind 102, topple) is not ported");
        expect(!smasher.destroyData("6-9_playermech"), "6-9_PlayerMech (kind 242, falls apart) is not ported");
    }
    {
        const assets::Level level = loadLevel(archive, "levels/levels/lvl_07_downtown/lvl_07_downtown_main.blvl");
        const VehiclePropSmasher smasher = loaded(archive, level);
        const auto statue = smasher.destroyData("7-8_statuel");
        expect(statue.has_value(), "7-8_StatueL (leprops/leprop_pushable, kind 71) has destroy data");
        if (statue)
            expect(statue->effects.explosionParticle == 10 && !statue->effects.spawnDebris && statue->effects.sound == 549,
                   "7-8_StatueL: ExplosionParticleDef 10, no SpawnDebris, ATTR_SFX_DESTROYED 549");
    }
}

}

int main(int argc, char** argv) {
    sets();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("destroy-effects-test: data not available, skipped\n");
            return failures ? 1 : 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

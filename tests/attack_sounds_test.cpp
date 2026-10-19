// Tests of the attack sounds (src/runtime/attack_sounds.*; docs/re/player-attack.md). Recorded values: llvm-objdump of 0x307e7c (swing: melee row
// byte +0x17 of the character record, weapon table *0x376050 stride 24, u16 +0xc, played by 0x23ae84 at 0x3082f4), 0x319ffc (hit: u16 +0xa, 0x31a378),
// 0x23fa54 (id 0 returns at once), 2026-10-19; table values from build/chase-tabledump tables/tablesweapons.bxls and tablescharacters.bxls (sheet 0),
// 2026-10-19. Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/attack_sounds.hpp"

#include <cstdio>
#include <filesystem>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

void calls() {
    const std::array<float, 3> at{1.0f, 2.0f, 3.0f};
    const AttackSounds sounds(assets::WeaponSounds{501, 517});
    AttackStep idle;
    expect(sounds.calls(idle, at).empty(), "an update without a new state or hit plays nothing");
    AttackStep started;
    started.started = true;
    started.messages.push_back({0, kAttackMessageNotice, 0, 0});
    const std::vector<SoundCall> swing = sounds.calls(started, at);
    expect(swing.size() == 1 && swing[0].id == 517 && swing[0].position == at, "entering the state plays the +0xc sound at the player's position");
    AttackStep hit;
    hit.messages.push_back({0, kAttackMessageHit, kAttackDamage, kAttackHitKind});
    const std::vector<SoundCall> struck = sounds.calls(hit, at);
    expect(struck.size() == 1 && struck[0].id == 501 && struck[0].position == at, "a hit message plays the +0xa sound at the player's position");
    AttackStep both = hit;
    both.started = true;
    const std::vector<SoundCall> order = sounds.calls(both, at);
    expect(order.size() == 2 && order[0].id == 517 && order[1].id == 501, "the swing sound comes before the hit sound");
    const AttackSounds silent{assets::WeaponSounds{0, 0}};
    expect(silent.calls(both, at).empty(), "sound id 0 plays nothing");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* weaponEntry = archive.find("tables/tablesweapons.bxls");
    const assets::FuseEntry* characterEntry = archive.find("tables/tablescharacters.bxls");
    if (!weaponEntry || !characterEntry) {
        std::printf("FAIL: weapon or character table not in archive\n");
        ++failures;
        return;
    }
    const assets::Table weapons = assets::parseTable(archive.read(*weaponEntry));
    const assets::Table characters = assets::parseTable(archive.read(*characterEntry));
    expect(assets::characterMeleeWeapon(characters, 9) == 13, "Chase (row 9, police) uses melee weapon row 13 (byte +0x17)");
    expect(assets::weaponSounds(weapons, 8).hit == 501 && assets::weaponSounds(weapons, 8).swing == 0,
           "weapon row 8 (grapplegun): +0xa = 501, +0xc = 0 (columns 5 and 6)");
    for (const std::uint8_t row : {0, 13, 15, 16}) {
        const assets::WeaponSounds sounds = assets::weaponSounds(weapons, row);
        expect(sounds.hit == 0 && sounds.swing == 0, "the melee weapon rows 0, 13, 15 and 16 have no attack sounds");
    }
    bool every = true;
    for (std::size_t row = 0; row < characters.sheets[0].rows; ++row) {
        const AttackSounds sounds = attackSoundsOf(characters, weapons, row);
        every = every && sounds.hitId() == 0 && sounds.swingId() == 0;
    }
    expect(every, "no character of the table has a melee hit or swing sound (all 192 rows use weapon rows 0, 13, 15 or 16)");
    const AttackSounds chase = attackSoundsOf(characters, weapons, 9);
    AttackStep step;
    step.started = true;
    step.messages.push_back({0, kAttackMessageHit, kAttackDamage, kAttackHitKind});
    expect(chase.calls(step, {0, 0, 0}).empty(), "Chase's attack plays no sound");
}

}  // namespace

int main(int argc, char** argv) {
    calls();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("attack-sounds-test: data not available, skipped\n");
            return 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures != 0) return 1;
    std::printf("attack sounds tests passed\n");
    return 0;
}

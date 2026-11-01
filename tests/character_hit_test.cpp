// A character takes a hit (src/runtime/character_hit.*), docs/re/combat-characters.md entry 2026-11-01 "The hit receiver": the rules of 0x1b8570,
// 0x15afb4, 0x18ee2c, 0x1d6c84, 0x2a47b0, 0x283c38, 0x2a4814, 0x1b8478, 0x25f0ec, 0x2b900c and the class parameters of 0x236e48 / 0x237234 were read
// with REA (Ghidra 12.1.2) and llvm-objdump of artifacts/game/game.arm.elf by 20261101-mission-E (2026-11-01); the expected values follow those rules
// by hand. The real-data part reads the mission 1-4 goons of lvl_01 and their clips; it exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/character_hit.hpp"

#include <cstdio>
#include <filesystem>
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

CharacterHitMessage punch() { return characterHitFromPlayer(1, kHitKindPunch); }

CharacterHitTarget goon() {
    CharacterHitTarget target;
    target.health = 2;
    target.maxHealth = 2;
    target.arrestHealth = 2;
    return target;
}

void punches() {
    CharacterHitTarget target = goon();
    CharacterHitResult r = receiveHit(target, punch());
    expect(r.outcome == CharacterHitOutcome::Reacted && r.damage == 0 && target.health == 2, "a punch (kind 1 from the player) takes no health (0x1b8bb4)");
    expect(r.reaction == kCharacterStatePunched && target.pending == kCharacterStatePunched, "message 46 requests PUNCHED (190)");
    expect(r.cooldown == 0 && target.cooldown == 0, "the punch skips the hit list 0x22c60c");
    expect(!hitReactionState(target, punch()), "no reaction while the request is pending (ctrl+0x6e differs from +0x6c)");
    target.pending.reset();
    target.state = kCharacterStatePunched;
    expect(hitReactionState(target, punch()) == kCharacterStatePunchedRepeat, "a punch in PUNCHED requests PUNCHED REPEAT (191)");
    target.state = kCharacterStatePunchedRepeat;
    expect(hitReactionState(target, punch()) == kCharacterStatePunched, "a punch in PUNCHED REPEAT requests PUNCHED again");
    target.state = 0x123;
    expect(!hitReactionState(target, punch()), "no reaction in a takedown state (0x23d610)");
    target.state = 0xa3;
    expect(hitReactionState(target, punch()) == kCharacterStatePunched, "an attack (0xa3) outside its window takes the reaction");
    target.attackWindow = true;
    expect(!hitReactionState(target, punch()), "no reaction in the window of an attack (0x25ed90)");
    target.attackWindow = false;
    target.state = 0xf1;
    expect(!hitReactionState(target, punch()), "no reaction in 0xf1 (0x25ed90)");
    target.state = kCharacterStateIdle;
    target.reactionLocked = true;
    expect(!hitReactionState(target, punch()), "no reaction with ctrl+0xd8 bit 6");
}

void guards() {
    CharacterHitTarget target = goon();
    target.disabled = true;
    expect(receiveHit(target, punch()).outcome == CharacterHitOutcome::Ignored, "a disabled entity (+0x10 bit 0) takes no hit");
    target = goon();
    CharacterHitMessage seven = punch();
    seven.kind = 7;
    expect(receiveHit(target, seven).outcome == CharacterHitOutcome::Ignored, "kind 7 is ignored");
    target = goon();
    target.spawnCounter = 135;
    expect(receiveHit(target, punch()).outcome == CharacterHitOutcome::Ignored, "a come-back counter below 136 turns the hit away");
    target = goon();
    target.cooldown = 3;
    expect(receiveHit(target, punch()).outcome == CharacterHitOutcome::Ignored, "a character in the hit list 0x593178 takes no hit below 100");
    target = goon();
    target.state = 323;
    expect(receiveHit(target, punch()).outcome == CharacterHitOutcome::Ignored, "ARREST (323) takes no hit");
    target = goon();
    target.selectMode = 4;
    expect(receiveHit(target, punch()).outcome == CharacterHitOutcome::Ignored, "0x219ba0 mode 4 refuses the player's hit");
    target = goon();
    target.playerSlot = true;
    expect(receiveHit(target, punch()).outcome == CharacterHitOutcome::Ignored, "an entity named Player1.PlayerNN takes no hit (0x23df6c)");
}

void damage() {
    CharacterHitMessage hit;
    hit.damage = 1;
    hit.kind = 0;
    CharacterHitTarget target = goon();
    CharacterHitResult r = receiveHit(target, hit);
    expect(r.outcome == CharacterHitOutcome::Damaged && r.damage == 1 && target.health == 1, "a hit without a sender takes its damage (0x15afb4)");
    expect(r.cooldown == kHitCooldownOther && target.cooldown == 10, "and lists the character for 10 updates");
    expect(r.reaction == kCharacterStateKnockBack, "the reaction on the ground is KNOCK BACK (188)");
    target.cooldown = 0;
    target.pending.reset();
    r = receiveHit(target, hit);
    expect(r.outcome == CharacterHitOutcome::Destroyed && target.health == 0 && target.destroyed, "the last health point destroys the entity (0x23cd80)");
    expect(r.cooldown == 0 && !r.reaction, "a destroyed entity gets no list entry (+0xc bit 4) and 0x2414b4 refuses the reaction of message 46");

    target = goon();
    target.onGround = false;
    expect(receiveHit(target, hit).reaction == kCharacterStateFall, "in the air the reaction is FALL (5)");
    target = goon();
    target.state = kCharacterStateKnockBack;
    expect(!receiveHit(target, hit).reaction, "no new KNOCK BACK while in KNOCK BACK");

    target = goon();
    target.boss = true;
    target.health = 5;
    hit.damage = 3;
    r = receiveHit(target, hit);
    expect(r.damage == 0 && target.health == 5, "a boss turns the damage to 0 in 0x22e884");
    target = goon();
    target.oneHitKill = true;
    target.health = 4;
    hit.damage = 1;
    expect(receiveHit(target, hit).outcome == CharacterHitOutcome::Destroyed, "ctrl+0x315 bit 5 makes the damage 100");
    target = goon();
    target.health = 1;
    hit.damage = -3;
    expect(receiveHit(target, hit).outcome == CharacterHitOutcome::Healed && target.health == 2, "a negative damage heals up to sub +0x128");
    target = goon();
    target.destroyed = true;
    hit.damage = 1;
    r = receiveHit(target, hit);
    expect(r.damage == 0 && target.health == 2 && r.cooldown == 0 && !r.reaction, "a destroyed entity (+0xc bit 4) takes no damage, entry or reaction");
    target = goon();
    hit.kind = 13;
    r = receiveHit(target, hit);
    expect(r.damage == 0 && target.health == 2, "a kind 13 hit is a takedown in 0x22e884: no damage");
    target = goon();
    hit.kind = 0;
    hit.move = std::uint8_t(3);
    expect(receiveHit(target, hit).damage == 0, "a move record of type 3 is a takedown: no damage");
    target = goon();
    hit.move = std::uint8_t(8);
    expect(receiveHit(target, hit).damage == 1, "a move record of type 8 reaches no takedown state (0x22e9dc): the damage is taken");
    hit.move.reset();
    target = goon();
    target.kind = 159;
    expect(receiveHit(target, hit).damage == 0, "a ride pig (kind 159) takes no hit but kind 4");
    target = goon();
    target.kind = 159;
    hit.kind = 4;
    expect(receiveHit(target, hit).damage == 1, "a ride pig takes kind 4");
    hit.kind = 0;
    target = goon();
    target.player = true;
    r = receiveHit(target, punch());
    expect(r.damage == 1 && target.health == 1 && r.cooldown == kHitCooldownPlayer, "the player takes the punch's damage, listed for 20 updates");
    target = goon();
    hit.kind = 1;
    expect(receiveHit(target, hit).cooldown == kHitCooldownPunch, "a kind 1 hit without a sender lists the character for 5 updates");
}

void special() {
    CharacterHitTarget target = goon();
    CharacterHitMessage hit;
    hit.damage = 0;
    hit.kind = 4;
    hit.weapon = 2;
    expect(hitReactionState(target, hit) == kCharacterStateKnockBackCont, "kind 4 weapon 2 requests KNOCK BACK CONT (189)");
    hit.kind = 6;
    hit.weapon = 6;
    expect(hitReactionState(target, hit) == kCharacterStateThrown, "kind 6 weapon 6 requests 224");
    hit.weapon = 4;
    expect(hitReactionState(target, hit) == kCharacterStateKnockBackCont, "kind 6 weapon 4 (outside 6..11) requests 189");
}

void arrest() {
    expect(arrestHealthOf(1) == 2 && arrestHealthOf(0) == 1 && arrestHealthOf(300) == 255, "sub +0x122 = ArrestHealth + 1, at most 255");
    CharacterHitTarget target = goon();
    expect(!changeArrestHealth(target, -1) && target.arrestHealth == 1, "a takedown of 1 leaves 1");
    expect(changeArrestHealth(target, -3) && target.arrestHealth == 0, "the counter stops at 0 and reports it");
}

void states() {
    CharacterHitStates s;
    s.request(kCharacterStatePunched);
    CharacterHitStep step = s.update(false, false);
    expect(s.state() == kCharacterStatePunched && step.clip == kClipPunched && step.facesSender, "PUNCHED enter 0x1d6c84 plays clip 180 facing the sender");
    step = s.update(false, false);
    expect(!step.request, "PUNCHED waits for its clip");
    step = s.update(true, false);
    expect(step.request == kCharacterStateIdle, "0x2a47b0 requests IDLE once the clip finished");
    step = s.update(false, false);
    expect(s.state() == kCharacterStateIdle && !s.active() && !step.clip, "the character is back in IDLE");

    s.request(kCharacterStateKnockedDown);
    step = s.update(false, false);
    expect(step.clip == kClipKnockedDown && s.knockedDownWait() == 60, "KNOCKED DOWN enter 0x283c38: clip 177, wait 60");
    for (int i = 0; i < 59; ++i) expect(!s.update(true, false).request, "KNOCKED DOWN waits after the clip");
    step = s.update(true, false);
    expect(step.request == kCharacterStateGetUp, "the 60th finished update requests GET UP (0x2a4814)");
    step = s.update(false, false);
    expect(step.clip == kClipGetUp, "GET UP enter 0x1b8478 plays clip 178");
    expect(s.update(true, false).request == kCharacterStateIdle, "GET UP 0x25f0ec requests IDLE after its clip");
    s.update(false, false);
    s.request(kCharacterStateKnockedDown);
    s.update(false, true);
    expect(s.knockedDownWait() == 2, "a boss waits 2 updates");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    const assets::FuseEntry* tableEntry = archive.find("tables/tablesanimations.bxls");
    expect(tableEntry != nullptr, "tables/tablesanimations.bxls");
    if (tableEntry) {
        const assets::Table table = assets::parseTable(archive.read(*tableEntry));
        const auto name = [&](int row) { return assets::tableValue(table.sheets.at(1), std::size_t(row), 1).text; };
        expect(name(kClipPunched) == "punched01" && name(kClipKnockedDown) == "react_knockd" && name(kClipGetUp) == "react_knockd_getup",
               "rows 180, 177, 178 of tablesanimations sheet 1");
        for (int row : {kClipPunched, kClipKnockedDown, kClipGetUp})
            expect(archive.find("models/char_minifig/class_generic/" + name(row) + "_minifig.bfnanm") != nullptr,
                   name(row) + " exists for class_generic");
    }
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    expect(levelEntry != nullptr, "lvl_01");
    if (!levelEntry) return;
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    unsigned goons = 0;
    for (const assets::LevelObject& object : level.objects) {
        if (object.instance.rfind("1-4_AI_Goon_", 0) != 0 || object.className.rfind("chars/", 0) != 0) continue;
        const assets::Level* cls = classOf(object.className);
        expect(cls != nullptr, object.instance + " has its class");
        if (!cls) continue;
        ++goons;
        const CharacterHitTarget target = characterHitTargetOf(object, *cls);
        expect(target.maxHealth == 2 && target.health == 2 && target.arrestHealth == 2 && !target.boss,
               object.instance + ": Health 2.0, ArrestHealth 1.0 (sub +0x122 = 2), no Boss_IsBoss");
        CharacterHitTarget punched = target;
        const CharacterHitResult r = receiveHit(punched, punch());
        expect(r.reaction == kCharacterStatePunched && punched.health == 2, object.instance + " reacts to the punch with PUNCHED and keeps its health");
    }
    expect(goons == 15, "15 mission 1-4 goons in lvl_01: 1-4_AI_Goon_01..17 without 04 and 05");
}

}

int main(int argc, char** argv) {
    punches();
    guards();
    damage();
    special();
    arrest();
    states();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("character-hit-test: data not available, skipped\n");
            return failures ? 1 : 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

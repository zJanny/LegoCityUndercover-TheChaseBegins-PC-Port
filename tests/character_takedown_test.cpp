// The takedown of a level character (src/runtime/character_takedown.*), docs/re/combat-characters.md entry 2026-11-02 "The takedown states": the rules
// of 0x1d6d04, 0x27726c, 0x22e884, 0x28cbf8, 0x29ad90, 0x283a0c, 0x2b900c, 0x192fd4, 0x21e700, 0x2345dc and 0x1d1170 were read with llvm-objdump of
// artifacts/game/game.arm.elf by 20261102-mission14-B (2026-11-02); the expected values follow those rules by hand. The table values are the rows of
// tables/tablescombat.bxls printed by `build/chase-tabledump tables/tablescombat.bxls` (sheet 1 row 3, sheet 2 row 9, sheet 5 rows 0 and 1). The
// real-data part reads those tables, the clip names and the mission 1-4 goons of lvl_01; it exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/character_takedown.hpp"

#include <cctype>
#include <cmath>
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

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

constexpr int kMove = 3;          // fight move 3, `grab_putdown_04` (oracle 20261101-mission-D-1)
constexpr std::uint8_t kRow = 9;  // its sheet 1 byte +0xa

TakedownTables tables() {
    TakedownTables t;
    t.fightRows = {0, 7, 8, 9, 10, 11, 12, 13};
    t.fightRowsAbility51 = {45, 46, 47, 48, 49, 50, 51, 52};
    t.moves.resize(57);
    t.moves[kRow] = {{468, 308, 309, 315, 316}, 1, 20};
    t.moves[48] = {{638, 0, 0, 0, 0}, 0, 0};
    t.timings.resize(3);
    t.timings[0].times = {0.35f, 0.35f, 0.35f, 0.5f, 1.0f, 3.0f, 2.5f, 4.0f, 1.0f, 2.5f, 1.0f, 3.0f};
    t.timings[1].times = {4.0f, 4.0f, 4.0f, 1.0f, 2.0f, 6.0f, 5.0f, 8.0f, 10.0f, 5.0f, 1.0f, 3.0f};
    t.timings[2].times = {3.0f, 1.0f, 3.0f, 0.5f, 1.0f, 3.0f, 2.5f, 4.0f, 5.0f, 2.5f, 1.0f, 3.0f};
    return t;
}

CharacterHitTarget goon() {
    CharacterHitTarget target;
    target.health = 2;
    target.maxHealth = 2;
    target.arrestHealth = 2;
    return target;
}

struct Run {
    TakedownTables t = tables();
    TakedownCharacter c;
    CharacterHitTarget target = goon();
    ArrestableList list;
    CharacterTakedownStates s;
    std::uint32_t word = 0;
    TakedownStep step(bool finished, float dt = 0.25f) {
        return s.update(c, target, t, list, "goon", finished, dt, [this] { return word; });
    }
};

// 227 MELEE PUTDOWN -> 309 PUTDOWN STUN -> 313 GENERIC GETUP -> IDLE, with the arrest health 2 -> 1 (not arrestable).
void firstThrow() {
    Run r;
    r.s.startFightMove(kMove);
    TakedownStep st = r.step(false);
    expect(r.s.state() == kTakedownMeleePutdown && st.clip == 468 && !st.clipLoops, "MELEE PUTDOWN plays sheet 2 row 9 clip 0 (468)");
    expect(r.c.moveRow == kRow && (r.c.selectByte & 7) == 1 && r.c.grabbed, "the enter takes row +0xa, target mode 1 and entity +0xc bit 21");
    st = r.step(false);
    expect(!st.request && r.s.state() == kTakedownMeleePutdown, "MELEE PUTDOWN waits for its clip");
    st = r.step(true);
    expect(r.s.state() == kTakedownPutdownStun && st.clip == 308 && st.clipLoops, "the clip done: 0x22e884 kind 16 -> 309 entered in the same update, clip 1 (308) mode 1");
    expect(st.arrestHealthBefore == 2 && r.target.arrestHealth == 1 && st.effect265 && !st.arrestable, "the arrest health drops 2 -> 1 by byte +10");
    expect(near(r.c.timer, 0.1f) && (r.c.selectByte & 7) == 1 && r.list.size() == 0, "timer 0.35 counted once by the hook of the same update, mode 1");
    st = r.step(false);
    expect(st.request == kTakedownGenericGetUp, "timer at or below 0: GENERIC GETUP");
    st = r.step(false);
    expect(r.s.state() == kTakedownGenericGetUp && st.clip == 309 && !st.clipLoops && !st.unlisted, "GENERIC GETUP plays clip 2 (309)");
    expect((r.c.selectByte & 7) == 1, "GENERIC GETUP is a takedown state: mode 1");
    st = r.step(false);
    expect(!st.request, "GENERIC GETUP waits for its clip");
    st = r.step(true);
    expect(st.request == kCharacterStateIdle, "GENERIC GETUP done: IDLE");
    r.step(false);
    expect(!r.s.active() && (r.c.selectByte & 7) == 0 && !r.c.grabbed, "IDLE: mode and bit 21 restored by the exit 0x283a0c");
}

// The second throw takes the arrest health to 0: listed, CharacterArrestable, the arrestable time 4.0, the mode restored; the exit unlists it.
void secondThrow() {
    Run r;
    r.target.arrestHealth = 1;
    r.s.startFightMove(kMove);
    r.step(false);
    TakedownStep st = r.step(true);
    expect(st.arrestHealthBefore == 1 && r.target.arrestHealth == 0 && st.arrestable && r.list.contains("goon"), "1 -> 0: listed (0x192fd4)");
    expect(near(r.c.timer, 3.75f) && near(r.c.arrestTime, 4.0f) && (r.c.selectByte & 7) == 0 && r.c.grabbed, "timer 4.0 of row 1, mode restored, bit 21 kept");
    int updates = 0;
    while (!(st = r.step(false)).request && updates < 100) ++updates;
    expect(updates == 14 && st.request == kTakedownGenericGetUp, "4.0 s at dt 0.25: the 16th countdown requests GENERIC GETUP");
    st = r.step(false);
    expect(st.unlisted && !r.list.contains("goon"), "the exit of PUTDOWN STUN removes it (0x21e700)");
    Run again;
    again.target.arrestHealth = 0;
    again.s.startFightMove(kMove);
    again.step(false);
    st = again.step(true);
    expect(st.arrestHealthBefore == 0 && !st.effect265 && st.arrestable, "at 0 a throw lists it again, without effect 265");
}

// Oracle 20261102-mission14-B-1 (servant 20261102-mission14-servant, 2026-11-02, recorded from a save state of the first showroom fight of mission
// 1-4, dt 1/30): a throw with row 12 enters 309 in the update of the 0x22e884 call (u93) with the timer 0.317 after it, requests 313 in u103 and
// enters it in u104; a throw with row 21 (arrest damage 2) lists the goon in the update of its landing, timer 3.967 after it, ctrl+0xda 0.
void oracleTiming() {
    const float dt = 1.0f / 30.0f;
    Run r;
    r.t.moves[12] = {{474, 308, 309, 315, 316}, 1, 20};
    r.c.moveRow = 12;
    r.s.request(kTakedownPutdownStun);
    TakedownStep st = r.step(false, dt);
    expect(r.target.arrestHealth == 1 && std::fabs(r.c.timer - 0.317f) < 5e-4f, "u93: 309, arrest health 1, timer 0.317");
    int updates = 0;
    while (!(st = r.step(false, dt)).request && updates < 100) ++updates;
    expect(updates == 9, "u103: the 10th update after u93 requests 313");
    r.step(false, dt);
    expect(r.s.state() == kTakedownGenericGetUp, "u104: 313");
    Run a;
    a.t.moves[21] = {{488, 308, 309, 315, 316}, 2, 50};
    a.c.moveRow = 21;
    a.s.request(kTakedownPutdownStun);
    st = a.step(false, dt);
    expect(st.arrestHealthBefore == 2 && a.target.arrestHealth == 0 && st.arrestable && std::fabs(a.c.timer - 3.967f) < 5e-4f,
           "row 21: 2 -> 0, listed and CharacterArrestable in the landing update, timer 3.967");
    expect((a.c.selectByte & 7) == 0 && a.c.grabbed, "ctrl+0xda 0, entity +0xc bit 21 set (0x00220004)");
}

void avoidArrest() {
    Run r;
    r.target.arrestHealth = 1;
    r.c.avoidArrestTrigger = true;
    r.s.startFightMove(kMove);
    r.step(false);
    TakedownStep st = r.step(true);
    expect(r.target.arrestHealth == 0 && !st.arrestable && r.list.size() == 0 && near(r.c.timer, 0.35f), "a CharacterAvoidArrest trigger: not listed");
    expect(st.request == kTakedownAvoidArrest, "the stun hook requests AVOID ARREST (0x13a)");
    st = r.step(false);
    expect(r.s.state() == kTakedownAvoidArrest && st.clip == 309, "AVOID ARREST plays clip 2");
    st = r.step(true);
    expect(st.request == kCharacterStateIdle, "AVOID ARREST done: IDLE");
    st = r.step(false);
    expect(st.avoidArrest && !r.s.active(), "its exit raises CharacterAvoidArrest");
}

void abilityAndFlags() {
    Run r;
    r.c.ability51 = true;
    r.s.startFightMove(kMove);
    TakedownStep st = r.step(false);
    expect(r.c.moveRow == 48 && st.clip == 638, "ability 51: row +0xb (48)");
    r.step(true);
    expect(!r.s.active() && (r.c.selectByte & 7) == 0, "ability 51: IDLE after the clip (0x2246c0)");
    Run d;
    d.c.destroyed = true;
    d.s.startFightMove(kMove);
    d.step(false);
    d.step(true);
    expect(d.s.state() == kTakedownMeleePutdown, "a destroyed character stays");
    CharacterHitTarget t = goon();
    TakedownCharacter c;
    t.state = kTakedownMeleePutdown;
    expect(putdownRequest(t, c) == kTakedownPutdownStun, "kind 16: 309");
    c.autoArrestFlag = true;
    expect(putdownRequest(t, c) == kTakedownAutoArrested, "sub +0x131 & 0xc0: 311");
    c.autoArrestFlag = false;
    t.boss = true;
    expect(!putdownRequest(t, c), "a boss: 0x22e884 returns 1 without a request");
    t.boss = false;
    t.kind = 159;
    expect(!putdownRequest(t, c), "a ride pig takes only kind 4");
    t.kind = 27;
    t.state = kTakedownPutdownStun;
    expect(!putdownRequest(t, c), "already in a takedown state");
    t.state = 0x133;
    expect(putdownRequest(t, c) == kTakedownPutdownStun, "0x133 is not in the list of 0x22ea24");
}

void otherStates() {
    Run r;
    r.c.moveRow = kRow;
    r.s.request(0x123);
    TakedownStep st = r.step(false, 0);
    expect(st.clip == 308 && near(r.c.timer, 0.5f) && r.target.arrestHealth == 1, "ENERGYGUN STUN: time +12 (0.5), arrest drop");
    Run hit;
    hit.c.moveRow = kRow;
    hit.s.request(0x122);
    st = hit.step(true, 0);
    expect(st.clip == 468 && !st.arrestHealthBefore && !st.request, "ENERGYGUN HIT: clip 0, no drop, its new clip is not finished");
    st = hit.step(true, 0);
    expect(st.request == 0x123, "a HIT state goes to its STUN state on the clip end");
    Run run;
    run.c.moveRow = kRow;
    run.s.request(kTakedownRunOverStun);
    st = run.step(false, 0);
    expect(near(run.c.timer, kRunOverStunTime) && !st.arrestHealthBefore, "RUN OVER STUN: timer 2.0, no drop");
    Run boss;
    boss.c.moveRow = kRow;
    boss.c.timer = 7;
    boss.s.request(kTakedownBossStun);
    st = boss.step(false, 0);
    expect(near(boss.c.timer, 0) && boss.target.arrestHealth == 1, "BOSS STUN: timer 0, arrest drop");
    expect(!boss.step(true).request, "BOSS STUN: no hand-over in 0x29ad90");
    Run random;
    random.c.moveRow = kRow;
    random.c.timing = {2, 2};
    random.word = 0x18000;
    random.s.request(kTakedownPutdownStun);
    random.step(false, 0);
    expect(near(random.c.timer, 2.0f), "PUTDOWN STUN: 1.0 + (3.0 - 1.0) x 0.5");
    expect(near(takedownRandom(0x18000), 0.5f) && near(takedownRandom(0xffff), 65535.0f / 65536.0f), "0x2345dc: low 16 bits x 2^-16");
}

void list() {
    ArrestableList l;
    for (int i = 0; i < 8; ++i) expect(l.add("c" + std::to_string(i)), "slot " + std::to_string(i));
    expect(!l.add("c8") && l.size() == 8, "eight slots");
    l.remove("c3");
    expect(!l.contains("c3") && l.add("c9") && l.contains("c9"), "a freed slot is taken again");
}

void aggro() {
    AggroTarget goonTarget;
    goonTarget.badGuy = true;
    AggroSender player{"player", false, true};
    AggroResult r = aggroOnHit(goonTarget, player);
    expect(r.pursuit && !r.switchTarget && !r.counter, "not engaged: 0x21102c, nothing else");
    goonTarget.state = 82;
    expect(!aggroOnHit(goonTarget, player).pursuit, "state 82: 0x21102c does nothing");
    goonTarget.state = 1;
    AggroSender friendGoon{"goon2", true, false};
    expect(!aggroOnHit(goonTarget, friendGoon).pursuit, "the same side: no aggro");
    goonTarget.pursuitActive = true;
    goonTarget.target = "player";
    goonTarget.targetIsPlayer = true;
    r = aggroOnHit(goonTarget, player);
    expect(r.counter == 90 && !r.switchTarget, "its target is the player: ctrl+0x72 = 3 x 30");
    goonTarget.ignorePlayer = true;
    expect(!aggroOnHit(goonTarget, player).counter, "AIIgnorePlayer: nothing");
    AggroTarget cop;
    cop.pursuitActive = true;
    cop.target = "player";
    cop.targetIsPlayer = true;
    cop.range = 5;
    cop.targetDistance = 8;
    AggroSender goonSender{"goon", true, false};
    r = aggroOnHit(cop, goonSender);
    expect(r.switchTarget && r.counter == 90, "farther than ctrl+0x80 from its target: switch to the sender");
    cop.targetDistance = 4;
    expect(!aggroOnHit(cop, goonSender).switchTarget, "within ctrl+0x80: keep the target");
    cop.boss = true;
    cop.targetDistance = 8;
    expect(!aggroOnHit(cop, goonSender).switchTarget, "a boss keeps the player against a non-player sender");
    cop.counter = 5;
    cop.boss = false;
    expect(!aggroOnHit(cop, goonSender).switchTarget, "ctrl+0x72 running: nothing");
    CharacterHitTarget target = goon();
    expect(receiveHit(target, characterHitFromPlayer(1, kHitKindPunch)).aggro, "the receiver calls 0x1d1170 for the player's punch on a goon");
    CharacterHitTarget playerTarget = goon();
    playerTarget.player = true;
    expect(!receiveHit(playerTarget, characterHitFromPlayer(1, kHitKindPunch)).aggro, "not on the player");
    CharacterHitMessage noSender;
    expect(!receiveHit(target, noSender).aggro, "not without a character sender");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* combat = archive.find("tables/tablescombat.bxls");
    const assets::FuseEntry* animations = archive.find("tables/tablesanimations.bxls");
    expect(combat && animations, "tablescombat and tablesanimations");
    if (!combat || !animations) return;
    const TakedownTables t = takedownTables(assets::parseTable(archive.read(*combat)));
    const TakedownTables pinned = tables();
    expect(t.fightRows.size() == 14 && t.moves.size() == 57 && t.timings.size() == 3, "sheet 1: 14 rows, sheet 2: 57, sheet 5: 3");
    for (int move = 0; move < 8; ++move)
        expect(t.fightRows[std::size_t(move)] == pinned.fightRows[std::size_t(move)] &&
                   t.fightRowsAbility51[std::size_t(move)] == pinned.fightRowsAbility51[std::size_t(move)],
               "fight move " + std::to_string(move) + ": bytes +0xa / +0xb");
    expect(t.moves[kRow].clips == pinned.moves[kRow].clips && t.moves[kRow].arrestDamage == 1 && t.moves[kRow].studs == 20, "sheet 2 row 9");
    for (std::size_t row = 0; row < 2; ++row)
        for (std::size_t i = 0; i < 12; ++i) expect(near(t.timings[row].times[i], pinned.timings[row].times[i]), "sheet 5 row " + std::to_string(row));
    const assets::Table anim = assets::parseTable(archive.read(*animations));
    const auto name = [&](int row) { return assets::tableValue(anim.sheets.at(1), std::size_t(row), 1).text; };
    for (int row = 7; row <= 13; ++row)
        for (std::size_t clip = 0; clip < 3; ++clip) {
            const std::string n = name(t.moves[std::size_t(row)].clips[clip]);
            expect(archive.find("models/char_minifig/class_generic/" + n + "_minifig.bfnanm") != nullptr, n + " exists for class_generic");
        }
    expect(name(468) == "grabbed_putdown_04" && name(308) == "react_knockd_stunb" && name(309) == "react_knockd_stunb_up", "rows 468, 308, 309");
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    expect(levelEntry != nullptr, "lvl_01");
    if (!levelEntry) return;
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    unsigned goons = 0;
    for (const assets::LevelObject& object : level.objects) {
        if (object.instance.rfind("1-4_AI_Goon_", 0) != 0 || object.className.rfind("chars/", 0) != 0) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        ++goons;
        const TakedownTiming timing = takedownTimingOf(object, *cls);
        expect(timing.stun == 0 && timing.arrestable == 1, object.instance + ": HitTimingStun 0, HitTimingArrestable 1");
    }
    expect(goons == 15, "15 mission 1-4 goons");
    unsigned arrestable = 0;
    std::string avoid;
    const auto lower = [](std::string text) {
        for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
        return text;
    };
    for (const assets::LevelTrigger& trigger : level.triggers) {
        if (lower(trigger.event) == "characterarrestable" && lower(trigger.objectName).rfind("1-4_ai_goon_", 0) == 0) ++arrestable;
        if (lower(trigger.event) == "characteravoidarrest") avoid += lower(trigger.objectName) + ";";
    }
    expect(arrestable == 2, "lvl_01: CharacterArrestable on two goons (03 and 06)");
    expect(avoid == "1-8_ai_tony_knuckles;", "lvl_01: the only CharacterAvoidArrest trigger is on 1-8_AI_Tony_Knuckles");
}

}

int main(int argc, char** argv) {
    firstThrow();
    secondThrow();
    oracleTiming();
    avoidArrest();
    abilityAndFlags();
    otherStates();
    list();
    aggro();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("character-takedown-test: data not available, skipped\n");
            return failures ? 1 : 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

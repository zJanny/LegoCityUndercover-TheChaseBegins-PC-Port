// The player's arrest of a level character (src/runtime/character_arrest.*), docs/re/combat-characters-arrest.md entry 2026-11-02 "The arrest by A": the
// rules of 0x1fdbd4 (the A tap), 0x219c08 (fifth argument), 0x2107e0, 0x21e0d4, 0x23df04, 0x26ac14, 0x28ba78, 0x1c7208, 0x26abcc, 0x1b8150, 0x25ebe8,
// 0x282e6c and 0x1d4af0 were read with llvm-objdump of artifacts/game/game.arm.elf by 20261102-mission14b-B (2026-11-02); the expected values follow
// those rules by hand. The table values are the rows of `build/chase-tabledump tables/tablescombat.bxls` (sheet 2 row 9) and
// `tables/tablesanimations.bxls` (sheet 1 rows 314..316). The real-data part reads those tables, the `cuffing` clip and the arrest triggers of lvl_01;
// it exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/character_arrest.hpp"

#include <cctype>
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

constexpr int kMove = 3;          // fight move 3, `grab_putdown_04`
constexpr std::uint8_t kRow = 9;  // its sheet 1 byte +0xa

TakedownTables tables() {
    TakedownTables t;
    t.fightRows = {0, 7, 8, 9, 10, 11, 12, 13};
    t.fightRowsAbility51 = {45, 46, 47, 48, 49, 50, 51, 52};
    t.moves.resize(57);
    t.moves[kRow] = {{468, 308, 309, 315, 316}, 1, 20};
    t.timings.resize(3);
    t.timings[0].times = {0.35f, 0.35f, 0.35f, 0.5f, 1.0f, 3.0f, 2.5f, 4.0f, 1.0f, 2.5f, 1.0f, 3.0f};
    t.timings[1].times = {4.0f, 4.0f, 4.0f, 1.0f, 2.0f, 6.0f, 5.0f, 8.0f, 10.0f, 5.0f, 1.0f, 3.0f};
    return t;
}

// A goon lying in PUTDOWN STUN after the throw that took its arrest health to 0: listed and arrestable.
struct Lying {
    TakedownTables t = tables();
    TakedownCharacter c;
    CharacterHitTarget target;
    ArrestableList list;
    CharacterTakedownStates takedown;
    CharacterArrestStates arrest;
    Lying() {
        target.health = 2;
        target.maxHealth = 2;
        target.arrestHealth = 1;
        takedown.startFightMove(kMove);
        takedown.update(c, target, t, list, "goon", false, 1.0f / 30, [] { return 0u; });
        const TakedownStep landed = takedown.update(c, target, t, list, "goon", true, 1.0f / 30, [] { return 0u; });
        expect(landed.arrestable && takedown.state() == kTakedownPutdownStun && target.arrestHealth == 0 && list.contains("goon"),
               "set-up: the second throw leaves the goon listed in PUTDOWN STUN with arrest health 0");
    }
    ArrestStep step(bool finished) { return arrest.update(c, target, t, list, takedown, "goon", finished, 1.0f / 30); }
};

void pairedStates() {
    for (std::uint16_t s : {227, 228, 229, 230, 231, 254, 255, 260, 262, 263, 268, 269, 270, 272, 273, 419, 420})
        expect(characterPaired(s, 0), "0x21e0d4 takes state " + std::to_string(s));
    for (std::uint16_t s : {1, 226, 232, 253, 261, 274, 309, 313, 323, 324, 418, 421})
        expect(!characterPaired(s, 0), "0x21e0d4 refuses state " + std::to_string(s));
    expect(characterPaired(163, 4) && characterPaired(164, 10) && !characterPaired(163, 3) && !characterPaired(164, 11),
           "163 / 164 only with the attack record byte +9 4..10");
}

void arrestTest() {
    ArrestCandidate goon;
    goon.state = kTakedownPutdownStun;
    goon.arrestHealth = 0;
    goon.arrestDamage = 1;
    expect(arrestTargetPasses(goon), "arrest health 0 in PUTDOWN STUN with a row of arrest damage 1");
    for (std::uint16_t s : {0x123, 0x125, 0x127, 0x129, 0x12b, 0x12d, 0x12f, 0x134, 0x135, 0x13c}) {
        ArrestCandidate c = goon;
        c.state = s;
        expect(arrestTargetPasses(c), "stun state " + std::to_string(s) + " is arrestable");
    }
    for (std::uint16_t s : {0x122, 0x124, 0x12c, 0x12e, 0x130, 0x133, 0x136, 0x139, 0x13a, 0x13e, 227, 323, 324, 1}) {
        ArrestCandidate c = goon;
        c.state = s;
        expect(!arrestTargetPasses(c), "state " + std::to_string(s) + " is not arrestable");
    }
    ArrestCandidate health = goon;
    health.arrestHealth = 1;
    ArrestCandidate avoid = goon;
    avoid.avoidArrestTrigger = true;
    ArrestCandidate noDamage = goon;
    noDamage.arrestDamage = 0;
    expect(!arrestTargetPasses(health) && !arrestTargetPasses(avoid) && !arrestTargetPasses(noDamage),
           "not with arrest health left, a CharacterAvoidArrest trigger or a row without arrest damage");
}

AttackCandidate character(const std::string& name, float x, float z, std::uint8_t kind = kEntityKindCharacter) {
    AttackCandidate c;
    assets::Matrix node{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, z, 1};
    c.entity.kind = kind;
    c.entity.word = kind == kEntityKindCharacter ? kEntityWordBadGuy : 0;
    c.entity.boxCentre = {0, 0.5f, 0};
    c.entity.boxHalf = {0.3f, 0.5f, 0.3f};
    c.body = attackBody(node, c.entity.boxCentre, c.entity.boxHalf);
    c.name = name;
    return c;
}

void choose() {
    const AttackBody player = playerAttackBody(0, 0, 0, 0);
    const AttackEntity entity = playerAttackEntity();
    const auto lying = [](const std::string& name) -> std::optional<ArrestCandidate> {
        ArrestCandidate c;
        c.state = kTakedownPutdownStun;
        c.arrestHealth = name == "standing" ? 1 : 0;
        c.arrestDamage = 1;
        return c;
    };
    std::vector<AttackCandidate> candidates{character("goon", 0, 1.5f)};
    expect(chooseArrestTarget(player, entity, 0, candidates, lying) == std::size_t(0), "a lying goon 1.5 in front is arrested");
    candidates = {character("prop", 0, 0.8f, kEntityKindWobblyProp), character("goon", 0, 1.5f)};
    expect(chooseArrestTarget(player, entity, 0, candidates, lying) == std::size_t(1), "the search takes characters only (fifth argument 1)");
    candidates = {character("standing", 0, 1.0f), character("goon", 0, 1.8f)};
    expect(!chooseArrestTarget(player, entity, 0, candidates, lying), "the nearer goon is found and is not arrestable: no arrest");
    candidates = {character("goon", 0, 3.5f)};
    expect(!chooseArrestTarget(player, entity, 0, candidates, lying), "beyond the range 2.2 of ctrl+0x80");
    candidates = {character("goon", 1.5f, 0)};
    expect(chooseArrestTarget(player, entity, 0, candidates, lying) == std::size_t(0), "a bad guy beside the player: searched all around (r1 = 1)");
    candidates.front().entity.word = 0;
    expect(!chooseArrestTarget(player, entity, 0x4000, candidates, lying), "a character of the player's side is dropped by flag 1 of 0x21a2d4");
}

void turn() {
    expect(turnHeadingToward(kArrestingTurnStep, 0, 10000) == 6000, "0x23df04 turns by at most 0x1770");
    expect(turnHeadingToward(kArrestingTurnStep, 0, 3000) == 3000, "and stops at the target");
    expect(turnHeadingToward(kArrestingTurnStep, 30000, -30000) == -30000, "the short way across 0x8000");
    expect(turnHeadingToward(kArrestingTurnStep, -30000, 20000) == 29536, "the difference -15536 is clamped to -6000");
}

void player() {
    PlayerArrest arrest;
    LocomotionState motion;
    motion.facing = 0;
    arrest.start("goon", 28);
    expect(arrest.active() && arrest.target() == "goon", "the A tap enters ARRESTING");
    unsigned updates = 0;
    PlayerArrestStep step;
    do {
        step = arrest.update(motion, std::int16_t(10000));
        ++updates;
        if (updates == 1) expect(motion.facing == 6000 && motion.lastIntent == 6000, "the first update turns ctrl+8 and ctrl+0xa by 0x1770");
    } while (!step.finished && updates < 100);
    expect(updates == 28 && motion.facing == 10000 && !arrest.active(), "a 28-update clip: IDLE requested 28 updates after the tap");
}

void target() {
    Lying r;
    r.arrest.start();
    ArrestStep step = r.step(false);
    expect(r.arrest.state() == kArrestState && r.takedown.state() == kArrestState && r.target.state == kArrestState, "the next update enters ARREST");
    expect(step.unlisted && !r.list.contains("goon"), "the exit of PUTDOWN STUN removes the goon from the list (0x21e700)");
    expect(step.clip == 315 && !step.clipLoops && (r.c.selectByte & 7) == kArrestTargetModeNobody && r.c.timer == 0.0f,
           "ARREST plays sheet 2 row 9 u16 +6 (315), target mode 4, ctrl+0x24c 0");
    step = r.step(false);
    expect(!step.clip && r.arrest.state() == kArrestState, "ARREST waits for its clip");
    step = r.step(true);
    expect(r.arrest.state() == kArrestedState && step.clip == 316 && step.clipLoops && step.arrested, "the clip done: ARRESTED in the same update, 316 mode 1, CharacterArrested");
    expect((r.c.selectByte & 7) == kArrestedTargetMode && !r.target.destroyed, "ARRESTED sets target mode 1");
    unsigned updates = 0;
    do {
        step = r.step(false);
        ++updates;
    } while (!step.done && updates < 100);
    expect(updates == 30 && r.c.destroyed && r.target.destroyed && r.arrest.done(),
           "thirty float steps of 1/30 s take ctrl+0x24c past 1.0 in the 30th update: destroyed");
    expect(!r.step(false).done && r.arrest.state() == kArrestedState, "nothing more after the end");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* combat = archive.find("tables/tablescombat.bxls");
    const assets::FuseEntry* animations = archive.find("tables/tablesanimations.bxls");
    expect(combat && animations, "tablescombat and tablesanimations");
    if (!combat || !animations) return;
    const TakedownTables t = takedownTables(assets::parseTable(archive.read(*combat)));
    expect(t.moves[kRow].clips[kArrestClipColumn] == 315 && t.moves[kRow].clips[kArrestedClipColumn] == 316, "sheet 2 row 9: 315 and 316");
    const assets::Table anim = assets::parseTable(archive.read(*animations));
    const auto name = [&](int row) { return assets::tableValue(anim.sheets.at(1), std::size_t(row), 1).text; };
    expect(name(kArrestingClip) == "cuffing" && name(315) == "cuffed_back" && name(316) == "cuffed_back_idle" && name(317) == "cuffed_front" &&
               name(318) == "cuffed_front_idle",
           "rows 314..318");
    const assets::FuseEntry* cuffing = archive.find("models/char_minifig/class_generic/cuffing_minifig.bfnanm");
    expect(cuffing != nullptr, "cuffing exists for class_generic");
    if (cuffing) expect(makeAttackClip(kArrestingClip, assets::parseAnimation(archive.read(*cuffing))).updates == 28, "cuffing lasts 28 updates");
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    expect(levelEntry != nullptr, "lvl_01");
    if (!levelEntry) return;
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const auto lower = [](std::string text) {
        for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
        return text;
    };
    std::string arrestable, arrested;
    for (const assets::LevelTrigger& trigger : level.triggers) {
        if (lower(trigger.event) == "characterarrestable") arrestable += lower(trigger.objectName) + ">" + lower(trigger.scriptName) + ";";
        if (lower(trigger.event) == "characterarrested") arrested += lower(trigger.objectName) + ">" + lower(trigger.scriptName) + ";";
    }
    expect(arrestable == "1-4_ai_goon_06>1-4_character_arrestable_script;1-4_ai_goon_03>1-4_character_arrestable_script;",
           "lvl_01: CharacterArrestable of goons 03 and 06: " + arrestable);
    expect(arrested == "1-4_booster_strange_chaser>1-4_robbercaught;1-8_ai_tony_knuckles>1-8_missionend;1-4_booster_strange_02>1-4_pursuit_booster_spawner_script;",
           "lvl_01: CharacterArrested: " + arrested);
}

}

int main(int argc, char** argv) {
    pairedStates();
    arrestTest();
    choose();
    turn();
    player();
    target();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("character-arrest-test: data not available, skipped\n");
            return failures ? 1 : 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

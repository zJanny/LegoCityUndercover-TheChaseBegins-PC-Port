// Tests of the attack's character targets (src/runtime/attack_targets.*, choosePlayerAttack in src/runtime/player_attack.*; docs/re/player-attack.md).
// Rules transcribed from 0x21a2d4, 0x219ba0, 0x245b00, 0x1b7358, 0x219c08, 0x28265c, 0x23ad70, 0x23d610, 0x1fdbd4 and 0x1d3980 (REA with Ghidra
// and llvm-objdump, 20261101-mission-D, 2026-11-01); made-up characters in the first part. With FIB and NAMES the second part checks the goons
// of mission 1-4 in lvl_01 (level data: AIBadGuy, CharacterEnum 142), Chase's and the goons' table rows and the fight moves of
// tables/tablescombat.bxls; it exits with 77 when the data is missing.
#include "assets/assets.hpp"
#include "runtime/ability_table.hpp"
#include "runtime/attack_targets.hpp"
#include "runtime/collision.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_attack.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

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

AttackEntity character(bool badGuy) {
    AttackEntity entity;
    entity.kind = kEntityKindCharacter;
    if (badGuy) entity.word |= kEntityWordBadGuy;
    return entity;
}

AttackCandidate standing(const AttackEntity& entity, float x, float z) {
    AttackCandidate candidate;
    const PlayerBox box;
    candidate.body = attackBody({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, z, 1}, {box.centre[0], box.centre[1], box.centre[2]}, {box.half[0], box.half[1], box.half[2]});
    candidate.entity = entity;
    return candidate;
}

void gate() {
    const AttackEntity player = playerAttackEntity();
    expect(attackCandidatePasses(character(true), player, kAttackSearchFlags), "a bad guy passes the Y tap flags 9");
    expect(!attackCandidatePasses(character(false), player, kAttackSearchFlags), "a character on the player's side is dropped by flag 1");
    expect(!attackCandidatePasses(character(true), player, kAttackStrikeFlagsPunch), "flags 0xb at the punch's strike drop characters of both sides");
    expect(attackCandidatePasses(character(false), player, kAttackStrikeFlags), "flags 0xc keep every character when the player attacks");
    AttackEntity gone = character(true);
    gone.flags |= kEntityFlagGone;
    expect(!attackCandidatePasses(gone, player, kAttackSearchFlags), "entity +0x10 bit 0 drops it");
    AttackEntity noCollision = character(true);
    noCollision.flags = 0;
    expect(!attackCandidatePasses(noCollision, player, kAttackSearchFlags), "without +0x10 bit 9 it is dropped");
    AttackEntity hidden = character(true);
    hidden.hidden = true;
    expect(!attackCandidatePasses(hidden, player, kAttackSearchFlags), "flag 8 drops a hidden node (0x23de70)");
    for (const int state : {0xb, 0x52, 0x143, 0x144}) {
        AttackEntity down = character(true);
        down.character.state = state;
        expect(!attackCandidatePasses(down, player, kAttackSearchFlags), "states 0xb, 0x52, 0x143 and 0x144 are no targets");
    }
    AttackEntity state19 = character(true);
    state19.character.state = 0x13;
    expect(attackCandidatePasses(state19, player, kAttackSearchFlags) && !attackCandidatePasses(state19, player, kAttackSearchFlags | kAttackGateNotState19),
           "state 0x13 is dropped only with flag 0x20");
    AttackEntity prop;
    expect(attackCandidatePasses(prop, player, kAttackSearchFlags), "a wobbly prop (kind 119) passes");
    for (const std::uint8_t kind : {std::uint8_t(100), std::uint8_t(155), std::uint8_t(85), std::uint8_t(214)}) {
        prop.kind = kind;
        expect(attackCandidatePasses(prop, player, kAttackSearchFlags), "kinds 100, 155, 85 and 214 pass");
    }
    prop.kind = 19;
    expect(!attackCandidatePasses(prop, player, kAttackSearchFlags), "kind 19 needs entity +0x90");
    prop.pathRecord = true;
    expect(attackCandidatePasses(prop, player, kAttackSearchFlags), "kind 19 with entity +0x90 passes");
    prop.kind = 158;
    expect(!attackCandidatePasses(prop, player, kAttackSearchFlags), "kind 158 never passes");
    prop.kind = 5;
    expect(!attackCandidatePasses(prop, player, kAttackSearchFlags), "other kinds do not pass");
}

void targetModes() {
    const AttackEntity player = playerAttackEntity();
    AttackEntity attacker = character(false);
    AttackEntity goon = character(true);
    expect(targetModeAllows(goon, player), "mode 0: anyone");
    goon.character.targetMode = 1;
    expect(!targetModeAllows(goon, player) && !attackCandidatePasses(goon, player, kAttackSearchFlags), "mode 1 (EnableTargetable 0): nobody");
    goon.character.targetMode = 2;
    expect(!targetModeAllows(goon, player), "mode 2: only ctrl+0xc4");
    goon.character.modeTargetIsAttacker = true;
    expect(targetModeAllows(goon, player), "mode 2 with ctrl+0xc4 the attacker");
    goon.character.targetMode = 3;
    expect(targetModeAllows(goon, player) && !targetModeAllows(goon, attacker), "mode 3: only the player");
    goon.character.targetMode = 4;
    expect(!targetModeAllows(goon, player), "mode 4: nobody");
}

void weights() {
    expect(attackWeight(character(true)) == 0.4f, "a bad guy weighs 0.4");
    expect(attackWeight(character(false)) == 1.0f, "a character on no side weighs 1.0");
    AttackEntity boss = character(true);
    boss.word |= kEntityWordBoss;
    expect(std::fabs(attackWeight(boss) - 0.3f) < 1e-7f, "a boss weighs 0.3 (0x3e99999a)");
    expect(attackWeight(playerAttackEntity()) == 0.25f, "the player weighs 0.25");
    AttackEntity prop;
    expect(attackWeight(prop) == 1.0f, "a prop weighs 1.0");
    prop.kind = 100;
    expect(std::fabs(attackWeight(prop) - 0.4f) < 1e-7f, "kind 100 weighs 0.4 (0x3ecccccd)");
    AttackEntity hidden = character(true);
    hidden.hidden = true;
    expect(attackWeight(hidden) == 1.0f, "a hidden node weighs 1.0");
    AttackEntity marked = character(true);
    marked.word |= kEntityWordNoHitReset;
    expect(attackWeight(marked) == 1.0f, "+0xc bit 4 weighs 1.0");
    expect(attackSearchAngle(character(true), 90.0f, true) == 180.0f && attackSearchAngle(character(true), 90.0f, false) == 90.0f &&
               attackSearchAngle(character(false), 90.0f, true) == 90.0f,
           "a bad guy is searched all around when the caller asks for it");
}

void fightTest() {
    AttackEntity player = playerAttackEntity();
    AbilitySet moves{};
    moves[46 >> 3] |= std::uint8_t(1u << (46 & 7));
    player.character.abilities = moves;
    const AttackEntity goon = character(true);
    expect(fightTargetPasses(goon, player, moves, true), "a goon on the ground passes 0x28265c");
    expect(!fightTargetPasses(goon, player, moves, false), "a blocked line of sight fails");
    AttackEntity untrained = player;
    untrained.character.abilities = {};
    expect(!fightTargetPasses(goon, untrained, moves, true), "the player needs one of the fight move abilities");
    AttackEntity jumping = goon;
    jumping.character.onGround = false;
    expect(!fightTargetPasses(jumping, player, moves, true), "both must be on the ground (ctrl+0x314 bit 6)");
    AttackEntity higher = goon;
    higher.character.groundY = 0.5f;
    expect(fightTargetPasses(higher, player, moves, true), "a ground gap of 0.5 passes");
    higher.character.groundY = 0.51f;
    expect(!fightTargetPasses(higher, player, moves, true), "a ground gap above 0.5 fails");
    AttackEntity refusing = goon;
    refusing.character.classFlag0 = true;
    expect(!fightTargetPasses(refusing, player, moves, true), "the class bit of row +0x1c refuses (0x23ad70)");
    AttackEntity waiting = goon;
    waiting.character.state = 0xdb;
    waiting.character.abilities[0x2b >> 3] |= std::uint8_t(1u << (0x2b & 7));
    expect(!fightTargetPasses(waiting, player, moves, true), "state 0xdb with ability 0x2b refuses");
    AttackEntity held = goon;
    held.character.heldByOther = true;
    expect(!fightTargetPasses(held, player, moves, true), "a goon another engaged entity holds (next state 0x132) fails");
    AttackEntity boss = goon;
    boss.word |= kEntityWordBoss;
    expect(!fightTargetPasses(boss, player, moves, true), "a boss fails 0x28265c");
    expect(!grabStateAllows(0x122) && !grabStateAllows(0x12f) && !grabStateAllows(0x13e) && !grabStateAllows(0x143) && grabStateAllows(1) &&
               grabStateAllows(0x130) && grabStateAllows(0x136),
           "0x23d610 refuses the grab and struggle states");
}

void fightMoveChoice() {
    std::vector<FightMove> moves(kFightMoveCount);
    for (std::size_t i = 0; i < moves.size(); ++i) {
        moves[i].clip = int(100 + i);
        moves[i].ability = std::uint8_t(40 + i);
    }
    AbilitySet player{};
    player[42 >> 3] |= std::uint8_t(1u << (42 & 7));
    GameRandom random(1);
    expect(chooseFightMove(-1, player, moves, random) == 2, "the only move the player can do");
    expect(chooseFightMove(2, player, moves, random) == 2, "the last move stays when no other move is allowed");
    expect(!chooseFightMove(3, AbilitySet{}, moves, random), "no move when the player has none of the abilities, the last one included");
    player[43 >> 3] |= std::uint8_t(1u << (43 & 7));
    expect(chooseFightMove(2, player, moves, random) == 3, "a move other than the last");
    GameRandom a(7), b(7);
    std::vector<int> allowed{2, 3};
    const std::uint32_t word = b.next();
    expect(chooseFightMove(-1, player, moves, a) == allowed[word % 2], "0x235498: the next random word modulo the count picks the move");
}

void playerChoice() {
    AttackEntity player = playerAttackEntity();
    PlayerAttackTables tables;
    tables.fightMoveAbilities[46 >> 3] |= std::uint8_t(1u << (46 & 7));
    player.character.abilities = tables.fightMoveAbilities;
    const AttackBody attacker = playerAttackBody(0, 0, 0, 0);
    const std::array<float, 3> ahead{0, 0, 1};
    {
        const std::vector<AttackCandidate> candidates{standing(character(true), 0, 1.5f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::FightMove && choice.target == 0u, "a goon 1.5 ahead gets the fight move (0x1d3980)");
    }
    {
        const std::vector<AttackCandidate> candidates{standing(character(true), 0, -2.5f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::FightMove && choice.target == 0u, "a goon 2.5 behind is found too: bad guys are searched all around");
    }
    {
        const std::vector<AttackCandidate> candidates{standing(character(true), 0, 3.3f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::Punch && !choice.target, "a goon 3.3 ahead is beyond the grab range; the melee search drops it (weapon row 13)");
        PlayerAttackTables weapon16 = tables;
        weapon16.weaponRow = kWeaponRowCharacterMelee;
        const PlayerAttackChoice running = choosePlayerAttack(attacker, player, ahead, true, candidates, weapon16);
        expect(running.move == PlayerAttackMove::Punch && running.target == 0u && running.running,
               "with weapon row 16 the running punch (range 3.3) keeps it");
    }
    {
        AttackEntity busy = character(true);
        busy.character.state = 0x143;
        AttackEntity grabbed = character(true);
        grabbed.character.state = 0x130;
        const std::vector<AttackCandidate> candidates{standing(busy, 0, 1.0f), standing(grabbed, 0, 2.0f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::FightMove && choice.target == 1u, "a goon in state 0x143 is no candidate; the next one is taken");
    }
    {
        AttackEntity struggling = character(true);
        struggling.character.abilities[0x32 >> 3] |= std::uint8_t(1u << (0x32 & 7));
        const std::vector<AttackCandidate> candidates{standing(struggling, 0, 1.5f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::Struggle, "a target with ability 0x32 gets the grab struggle (0x19bfc0)");
    }
    {
        AttackEntity resting = character(true);
        resting.character.state = 0x13a;
        const std::vector<AttackCandidate> candidates{standing(resting, 0, 1.5f)};
        expect(!grabStateAllows(0x13a), "state 0x13a refuses the grab");
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::Punch && !choice.target, "0x23d610 refusing leaves the punch without a target, no melee search");
    }
    {
        const std::vector<AttackCandidate> candidates{standing(character(false), 0, 1.5f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::Punch && !choice.target, "a character on the player's side is no target");
    }
    {
        AttackCandidate table = standing(AttackEntity{}, 0, 1.8f);
        const std::vector<AttackCandidate> candidates{table, standing(character(true), 0, 1.0f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, player, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::FightMove && choice.target == 1u, "the goon wins over a prop in the grab search");
        const std::vector<AttackCandidate> propOnly{table};
        const PlayerAttackChoice punch = choosePlayerAttack(attacker, player, ahead, false, propOnly, tables);
        expect(punch.move == PlayerAttackMove::Punch && punch.target == 0u, "a prop alone is the punch's target");
    }
    {
        AttackEntity untrained = player;
        untrained.character.abilities = {};
        const std::vector<AttackCandidate> candidates{standing(character(true), 0, 1.5f)};
        const PlayerAttackChoice choice = choosePlayerAttack(attacker, untrained, ahead, false, candidates, tables);
        expect(choice.move == PlayerAttackMove::Punch && !choice.target, "without a fight move ability the goon is not grabbed and the punch drops it");
    }
}

// Oracle, recorded from a save state (mission 1-4, first fight phase in the showroom hall, a goon in front of Chase), 20261101-mission-D-1 and -2,
// 2026-11-01: RPC, fixed dt 1/30, traps of 0x244fc8, 0x1d3980, 0x214f40, 0x231d20. Chase at (577.761, 27.6005, 967.0345), facing -5232, state 1;
// goon A (kind 27, +0x10 0x1240, +0xc 0x00020004, +0xda 0, state 1) at (577.19, 27.566, 968.139); goon B (same flags, state 111) at
// (575.258, 27.566, 967.710). A one-update Y tap without stick, with the stick fully left and fully down: 0x1d3980(player, goon A), 0x214f40
// variant 4, player state 163 in the update after the press and goon A state 227 one update later; no message 0 and no message 62 within 21 updates.
// The stick headings are the world directions the player walked in the press update: left moved him by (0, -0.2275), heading -32768; down moved
// him away from goon A, heading atan2(0.571, -1.105) = 27790.
void oracleFightStates() {
    AttackEntity player = playerAttackEntity();
    PlayerAttackTables tables;
    for (std::size_t i = 0; i < kFightMoveCount; ++i) {
        FightMove move;
        move.clip = 400 + int(i);
        move.ability = 46;
        tables.moves.push_back(move);
    }
    tables.fightMoveAbilities = fightMoveAbilities(tables.moves);
    player.character.abilities = tables.fightMoveAbilities;
    AttackEntity goonA = character(true);
    goonA.word = 0x00020004;
    goonA.flags = 0x1240;
    AttackEntity goonB = goonA;
    goonB.character.state = 111;
    std::vector<AttackCandidate> candidates{standing(goonB, 575.258f, 967.710f), standing(goonA, 577.19f, 968.139f)};
    for (AttackCandidate& candidate : candidates) candidate.body.node[13] = 27.566f;
    expect(attackCandidatePasses(goonA, player, kAttackSearchFlags), "D-1: goon A's flags pass the gate");
    AttackClips clips;
    for (const FightMove& move : tables.moves) {
        AttackClip clip;
        clip.id = move.clip;
        clip.updates = 36;
        clip.framesPerUpdate = 0.5f;
        clip.root.assign(clip.updates + 1, 0.0f);
        clips.clips.push_back(clip);
    }
    const std::int16_t headings[] = {0, -32768, 27790, 7886};
    for (const std::int16_t heading : headings) {
        for (const bool stick : {false, true}) {
            if (!stick && heading != 0) continue;
            PlayerAttack attack;
            LocomotionState motion;
            motion.x = 577.761f;
            motion.z = 967.0345f;
            motion.facing = -5232;
            motion.lastIntent = -5232;
            GameRandom random(1);
            AttackInput input;
            input.tap = true;
            input.allowed = true;
            input.intent.active = stick;
            input.intent.heading = heading;
            input.x = motion.x;
            input.y = 27.6005f;
            input.z = motion.z;
            input.candidates = &candidates;
            input.player = player;
            input.tables = &tables;
            input.random = &random;
            const AttackStep first = attack.update(input, motion, clips);
            expect(first.started && first.state == kAttackStateAction && attack.variant() == kFightMoveVariant, "D-1/D-2: the tap enters state 163 with variant 4");
            expect(first.fight && first.fight->target == 1 && first.fight->move == PlayerAttackMove::FightMove, "D-1/D-2: the fight move takes goon A in every direction");
            bool message = !first.messages.empty();
            input.tap = false;
            for (int k = 0; k < 20; ++k) message = message || !attack.update(input, motion, clips).messages.empty();
            expect(!message, "D-1/D-2: no message 0 or 62 reaches the goon during the fight move");
        }
    }
}

std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* value = effectiveParam(object, cls, assets::nameHash(name));
    if (!value) return fallback;
    if (value->type == 5 || value->type == 7) return std::int32_t(value->value);
    if (value->type == 0) {
        float f = 0;
        std::memcpy(&f, &value->value, 4);
        return std::int32_t(f);
    }
    return fallback;
}

int realData(const char* fib, const char* names) {
    if (!std::filesystem::exists(fib) || !std::filesystem::exists(names)) {
        std::printf("attack-targets-real-data: data not available, skipped\n");
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::Table combat = assets::parseTable(archive.read(*archive.find("tables/tablescombat.bxls")));
    const std::vector<FightMove> moves = fightMoves(combat);
    expect(moves.size() == kFightMoveCount, "eight fight moves in tablescombat sheet 1");
    const int clips[kFightMoveCount] = {261, 437, 453, 467, 469, 471, 473, 475};
    for (std::size_t i = 0; i < moves.size(); ++i) expect(moves[i].clip == clips[i] && moves[i].ability == 46, "fight move clips (grab_putdown..08) and ability 46");
    expect(std::fabs(moves[0].pairA + 0.06f) < 1e-6f && std::fabs(moves[0].pairB - 1.58f) < 1e-6f, "fight move 0 pair floats -0.06, 1.58");
    const assets::Table characters = assets::parseTable(archive.read(*archive.find("tables/tablescharacters.bxls")));
    const assets::TableSheet& sheet = characters.sheets[0];
    AbilityTable abilities;
    abilities.load(characters);
    expect(abilities.characterHas(9, 46), "Chase (row 9) has ability 46, so every fight move is his");
    expect(sheet.rowData[9 * sheet.stride + 0x17] == 13, "Chase's weapon row is 13: the punch drops characters");
    expect(!abilities.characterHas(142, 0x32) && !abilities.characterHas(142, 0x35) && (sheet.rowData[142 * sheet.stride + 0x1c] & 1) == 0,
           "the goon row 142 gets the fight move, not the struggle, and does not refuse it");

    const assets::Level level = assets::parseLevel(archive.read(*archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl")));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    const assets::DrawableObjects drawable = assets::drawableObjects(archive, level, classOf);
    std::vector<AttackCandidate> candidates;
    std::optional<std::size_t> goon01, goon02;
    std::size_t goons = 0;
    for (const assets::CharacterObject& object : drawable.characters) {
        const assets::LevelObject& levelObject = level.objects[object.placement.objectIndex];
        const assets::Level* cls = classOf(object.placement.className);
        AttackEntity entity = character(intParam(levelObject, *cls, "AIBadGuy", 0) != 0);
        const std::string& name = object.placement.instance;
        if (name.rfind("1-4_AI_Goon_", 0) == 0) {
            ++goons;
            expect(entity.word & kEntityWordBadGuy, "every 1-4_AI_Goon has AIBadGuy");
            expect(intParam(levelObject, *cls, "CharacterEnum", 192) == 142, "every 1-4_AI_Goon is CharacterEnum 142");
        }
        if (name == "1-4_AI_Goon_01") goon01 = candidates.size();
        if (name == "1-4_AI_Goon_02") goon02 = candidates.size();
        candidates.push_back(standing(entity, object.placement.position[0], object.placement.position[2]));
        candidates.back().body.node[13] = object.placement.position[1];
    }
    expect(goons == 15, "15 goons of mission 1-4 in lvl_01");
    expect(goon01 && goon02, "1-4_AI_Goon_01 and _02 are level characters");
    if (!goon01 || !goon02) return 1;
    for (const std::size_t i : {*goon01, *goon02}) {
        const AttackCandidate& c = candidates[i];
        expect(std::fabs(c.body.node[12] - (i == *goon01 ? 552.138f : 552.297f)) < 1e-3f, "goon x from the level data (probe of 2026-11-01)");
    }
    AttackEntity player = playerAttackEntity();
    PlayerAttackTables tables;
    tables.moves = moves;
    tables.fightMoveAbilities = fightMoveAbilities(moves);
    tables.weaponRow = 13;
    for (std::size_t i = 0; i < player.character.abilities.size(); ++i) player.character.abilities[i] = sheet.rowData[9 * sheet.stride + kCharacterAbilityMaskOffset + i];
    const AttackCandidate& first = candidates[*goon01];
    const float px = first.body.node[12], py = first.body.node[13], pz = first.body.node[14] - 1.5f;
    const AttackBody attacker = playerAttackBody(px, py, pz, 0);
    const PlayerAttackChoice facing = choosePlayerAttack(attacker, player, {0, 0, 1}, false, candidates, tables);
    expect(facing.move == PlayerAttackMove::FightMove && facing.target == *goon01, "1.5 in front of 1-4_AI_Goon_01 the tap grabs it");
    const PlayerAttackChoice away = choosePlayerAttack(attacker, player, {0, 0, -1}, false, candidates, tables);
    expect(away.move == PlayerAttackMove::FightMove && away.target == *goon01, "facing away the goon behind is grabbed (all around)");
    return 0;
}

}

int main(int argc, char** argv) {
    if (argc == 3) {
        const int result = realData(argv[1], argv[2]);
        if (result == 77) return 77;
        if (failures) std::printf("%d failures\n", failures);
        return failures || result ? 1 : 0;
    }
    gate();
    targetModes();
    weights();
    fightTest();
    fightMoveChoice();
    playerChoice();
    oracleFightStates();
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

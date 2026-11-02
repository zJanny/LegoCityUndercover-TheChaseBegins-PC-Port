// Tests of the world facts of the player's target search (src/runtime/attack_facts.*; docs/re/player-attack-characters.md): the line of sight
// 0x219990 (ray 0x21ab20 with the query words {5, 0x10, ...}), the player's near list (0x2e5408 and 0x23d4c0) and the box a character class
// gives its entity (0x23aff8). Read from the code by 20261102-mission14-D on 2026-11-02 (llvm-objdump; REA was unavailable); the geometry of
// the first part is made up and hand-calculated. With FIB and NAMES the second part checks the CollisionBound records of the characters of
// lvl_01 and the line of sight at the positions of the oracle's fight state; it exits with 77 when the data is missing.
#include "assets/assets.hpp"
#include "runtime/attack_facts.hpp"
#include "runtime/attack_targets.hpp"
#include "runtime/collision.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_attack.hpp"

#include <cmath>
#include <cstdio>
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

assets::CollisionItem wall(float z, bool facesPositive = false) {
    assets::CollisionItem item;
    item.name = "wall";
    const float quad[4][3] = {{-50, -50, z}, {50, -50, z}, {50, 50, z}, {-50, 50, z}};
    for (const auto& v : quad) item.vertices.insert(item.vertices.end(), {v[0], v[1], v[2]});
    assets::CollisionTriangle a, b;
    a.vertex[0] = 0, a.vertex[1] = facesPositive ? 1 : 2, a.vertex[2] = facesPositive ? 2 : 1;
    b.vertex[0] = 0, b.vertex[1] = facesPositive ? 2 : 3, b.vertex[2] = facesPositive ? 3 : 2;
    item.triangles = {a, b};
    assets::CollisionNode root;
    root.centre[2] = z;
    root.half[0] = root.half[1] = 50.0f;
    root.half[2] = 0.1f;
    root.info = (2u << 1) | 1u;
    item.nodes = {root};
    return item;
}

AttackCandidate goon(float x, float z, std::uint16_t flags = kEntityFlagCollision) {
    AttackCandidate candidate;
    candidate.entity.kind = kEntityKindCharacter;
    candidate.entity.word |= kEntityWordBadGuy;
    candidate.entity.flags = flags;
    const PlayerBox box;
    candidate.body = attackBody({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, z, 1}, {box.centre[0], box.centre[1], box.centre[2]}, {box.half[0], box.half[1], box.half[2]});
    return candidate;
}

// Provenance: hand calculation. The player's box centre (0, 1.15, 0.08) is the start of the ray; a goon's box (0, 1.15, 0.08) +- (0.6, 0.85, 0.52)
// at node z = Z spans z in [Z - 0.44, Z + 0.6]. The wall at z = 2 faces the player (wound like the camera probe's test wall).
void lineOfSight() {
    const AttackBody player = playerAttackBody(0, 0, 0, 0);
    {
        CollisionWorld world;
        world.add(wall(2.0f));
        const std::vector<AttackCandidate> near{goon(0, 1.5f)};
        expect(attackLineClear(world, player, near, 0), "a goon 1.5 ahead with a wall behind it: the ray ends before the wall");
        const std::vector<AttackCandidate> far{goon(0, 3.0f)};
        expect(!attackLineClear(world, player, far, 0), "a goon at 3.0 behind a wall at 2.0: blocked (the first hit is the wall)");
        CollisionWorld facingAway;
        facingAway.add(wall(2.0f, true));
        expect(attackLineClear(facingAway, player, far, 0), "a wall seen from its back is not hit (0x17e0f8 culls back faces)");
    }
    {
        CollisionWorld world;
        const std::vector<AttackCandidate> inline_{goon(0, 3.0f), goon(0, 1.5f)};
        expect(!attackLineClear(world, player, inline_, 0), "another character's box in the ray blocks the target behind it");
        expect(attackLineClear(world, player, inline_, 1), "the nearer character itself is clear");
        const std::vector<AttackCandidate> aside{goon(0, 3.0f), goon(2.0f, 1.5f)};
        expect(attackLineClear(world, player, aside, 0), "a character beside the ray does not block");
        const std::vector<AttackCandidate> noCollision{goon(0, 3.0f), goon(0, 1.5f, 0)};
        expect(attackLineClear(world, player, noCollision, 0), "a character without owner flag 0x200 is not in the ray");
        const std::vector<AttackCandidate> gone{goon(0, 3.0f), goon(0, 1.5f, kEntityFlagCollision | kEntityFlagGone)};
        expect(attackLineClear(world, player, gone, 0), "a character with owner flag 0 set is not in the ray");
        const std::vector<AttackCandidate> inside{goon(0, 0.5f)};
        expect(attackLineClear(world, player, inside, 0), "the target's own box is the nearest hit when the start is inside it");
        const std::vector<AttackCandidate> prop{goon(0, 3.0f), [] {
            AttackCandidate p;
            p.entity.kind = kEntityKindWobblyProp;
            p.body = attackBody({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1.5f, 1}, {0, 1, 0}, {1, 1, 1});
            return p;
        }()};
        expect(attackLineClear(world, player, prop, 0), "a prop candidate is not a character box (props come from the world)");
    }
    {
        const std::vector<AttackCandidate> far{goon(0, 3.0f)};
        for (const std::uint32_t mask : {std::uint32_t(1), std::uint32_t(4), std::uint32_t(6)}) {
            CollisionWorld world;
            world.add(wall(2.0f), mask);
            expect(!attackLineClear(world, player, far, 0), "masks 1, 4 and 6 are accepted by the query words (include 5)");
        }
        for (const std::uint32_t mask : {std::uint32_t(2), std::uint32_t(17), std::uint32_t(16)}) {
            CollisionWorld world;
            world.add(wall(2.0f), mask);
            expect(attackLineClear(world, player, far, 0), "masks 2, 16 and 17 are not (no common bit with 5, or exclude 0x10)");
        }
    }
}

// Provenance: hand calculation with the half extent 5.0 of *0x377538 (read in the oracle by 20261101-mission-D) and the box (0.6, 0.85, 0.52).
void nearList() {
    std::vector<AttackCandidate> candidates{goon(4.0f, 0), goon(5.5f, 0), goon(5.7f, 0), goon(0, -5.4f), goon(0, 20.0f), goon(1.0f, 0, 0),
                                            goon(1.0f, 1.0f, kEntityFlagCollision | kEntityFlagGone)};
    markAttackNearList(candidates, {0, 0, 0});
    expect(candidates[0].listed, "a goon 4.0 away is in the near list");
    expect(candidates[1].listed, "x 5.5 minus the half width 0.6 is 4.9: its box meets the near box");
    expect(!candidates[2].listed, "x 5.7 minus 0.6 is 5.1: outside the box of half 5.0");
    expect(candidates[3].listed, "z -5.4 plus the box's far side 0.6 - 0.08 reaches into the box");
    expect(!candidates[4].listed, "20 away is not listed");
    expect(!candidates[5].listed, "without owner flag 0x200 an entry is not gathered");
    expect(!candidates[6].listed, "with owner flag bit 0 an entry is not gathered");
    std::vector<AttackCandidate> many;
    for (int i = 0; i < 120; ++i) many.push_back(goon(0, 0.1f * float(i % 10)));
    markAttackNearList(many, {0, 0, 0});
    std::size_t listed = 0;
    for (const AttackCandidate& c : many) listed += c.listed ? 1 : 0;
    expect(listed == kAttackNearListMax, "the gather holds at most 100 entries");
    std::vector<AttackCandidate> unlisted{goon(0, 1.5f)};
    unlisted[0].listed = false;
    expect(!findAttackTarget(3.0f, 45.0f, playerAttackBody(0, 0, 0, 0), {0, 0, 1}, unlisted), "the search skips a candidate outside the near list");
}

void characterBox() {
    assets::Level classFile;
    assets::LevelBound bound;
    bound.name = "CollisionBound";
    bound.centre[1] = 1.15f;
    bound.centre[2] = 0.08f;
    bound.half[0] = 0.6f;
    bound.half[1] = 0.85f;
    bound.half[2] = 0.52f;
    classFile.bounds.push_back(bound);
    const EntityBox box = characterEntityBox(classFile, true);
    expect(box.collision && std::fabs(box.centre[1] - 1.15f) < 1e-6f && std::fabs(box.half[2] - 0.52f) < 1e-6f, "the CollisionBound record is the box and sets flag 0x200");
    expect(!characterEntityBox(classFile, false).collision, "BoundCollision 0 clears flag 0x200 again");
    expect(!characterEntityBox(assets::Level{}, true).collision, "a class without the record has no box and no collision");
}

void chooseWithSight() {
    AttackEntity player = playerAttackEntity();
    PlayerAttackTables tables;
    tables.fightMoveAbilities[46 >> 3] |= std::uint8_t(1u << (46 & 7));
    player.character.abilities = tables.fightMoveAbilities;
    const AttackBody attacker = playerAttackBody(0, 0, 0, 0);
    const std::vector<AttackCandidate> candidates{goon(0, 1.5f)};
    expect(choosePlayerAttack(attacker, player, {0, 0, 1}, false, candidates, tables).move == PlayerAttackMove::FightMove, "without a sight function the line is clear");
    std::size_t asked = 0;
    tables.lineOfSight = [&](const AttackBody&, const std::vector<AttackCandidate>&, std::size_t target) {
        ++asked;
        return target != 0;
    };
    const PlayerAttackChoice blocked = choosePlayerAttack(attacker, player, {0, 0, 1}, false, candidates, tables);
    expect(asked == 1 && blocked.move == PlayerAttackMove::Punch && !blocked.target, "a blocked line takes the target out of the grab test (0x28265c)");
    tables.lineOfSight = [](const AttackBody&, const std::vector<AttackCandidate>&, std::size_t) { return true; };
    expect(choosePlayerAttack(attacker, player, {0, 0, 1}, false, candidates, tables).move == PlayerAttackMove::FightMove, "a clear line keeps the fight move");
}

int realData(const char* fib, const char* names) {
    if (!std::filesystem::exists(fib) || !std::filesystem::exists(names)) {
        std::printf("attack-facts-real-data: data not available, skipped\n");
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::Level level = assets::parseLevel(archive.read(*archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl")));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    const assets::DrawableObjects drawable = assets::drawableObjects(archive, level, classOf);
    std::size_t checked = 0;
    for (const assets::CharacterObject& object : drawable.characters) {
        const std::string& className = object.placement.className;
        if (className.rfind("chars/char_ai_", 0) != 0 && className != "chars/char_journalist" && className != "chars/char_boosterstrange" && className != "chars/char_racecardriver") continue;
        const assets::Level* cls = classOf(className);
        expect(cls != nullptr, "character class is in the archive");
        if (!cls) continue;
        const EntityBox box = characterEntityBox(*cls, true);
        expect(box.collision && box.centre[0] == 0.0f && std::fabs(box.centre[1] - 1.15f) < 1e-6f && std::fabs(box.centre[2] - 0.08f) < 1e-6f &&
                   std::fabs(box.half[0] - 0.6f) < 1e-6f && std::fabs(box.half[1] - 0.85f) < 1e-6f && std::fabs(box.half[2] - 0.52f) < 1e-6f,
               "the CollisionBound of a level character class is the player's box (0, 1.15, 0.08) / (0.6, 0.85, 0.52)");
        ++checked;
    }
    expect(checked >= 15, "the 15 goons of mission 1-4 and the other level characters are checked");
    const assets::Level* pedestrian = classOf("chars/pedestrian");
    expect(pedestrian && !characterEntityBox(*pedestrian, true).collision, "chars/pedestrian has no CollisionBound record");

    CollisionWorld world;
    const std::string name = "lvl_01_cherrytreehills";
    for (const auto& item : assets::parseModel(archive.read(*archive.find("models/" + name + "/" + name + ".bfnmdl"))).collision) world.add(item);
    const assets::PlacedObjectState startState = levelStartState(level, classOf);
    for (const auto& object : assets::objectCollisions(archive, level, startState)) world.addObject(object);
    // Provenance: recorded from a save state of mission 1-4 (first fight phase, a goon in front of Chase) by 20261101-mission-D-1 on 2026-11-01:
    // Chase at (577.761, 27.6005, 967.0345) facing -5232, goon A (577.19, 27.566, 968.139) 1.0 m away, goon B (575.258, 27.566, 967.710) 2.6 m away;
    // the tap took goon A into the fight move, whose test 0x28265c ends with this line of sight.
    const AttackBody chase = playerAttackBody(577.761f, 27.6005f, 967.0345f, std::int16_t(-5232));
    std::vector<AttackCandidate> goons;
    for (const std::array<float, 3>& p : {std::array<float, 3>{577.19f, 27.566f, 968.139f}, std::array<float, 3>{575.258f, 27.566f, 967.710f}}) {
        AttackCandidate candidate = goon(p[0], p[2]);
        candidate.body.node[13] = p[1];
        goons.push_back(candidate);
    }
    expect(attackLineClear(world, chase, goons, 0), "the real street and props leave the line to goon A clear (oracle: the tap took it)");
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
    lineOfSight();
    nearList();
    characterBox();
    chooseWithSight();
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

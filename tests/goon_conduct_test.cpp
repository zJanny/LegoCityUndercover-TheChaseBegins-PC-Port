// The idle decision of the AI characters (src/runtime/goon_conduct.*) on made-up characters: the rules are those read from game.arm.elf by
// 20261103-large-C (2026-11-03, llvm-objdump of 0x2a9874, 0x1f8d5c, 0x1f6958, 0x21102c, 0x206d18, 0x21decc, 0x22471c and 0x1c6e84; corrected 2026-11-07 for 0x1f906c, 0x1f6aa0 and 0x2a9bc4;
// docs/re/scripts-missions-1-4-spawners-conduct.md), not measured in the original.
#include "runtime/goon_conduct.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

AiConductEntity goon(const std::string& name, float x, float z) {
    AiConductEntity entity;
    entity.name = name;
    entity.params.badGuy = true;
    entity.params.viewDistance = 50;
    entity.position = {x, 0.0f, z};
    return entity;
}

AiConductEntity thePlayer(float x, float z) {
    AiConductEntity entity;
    entity.name = "player1";
    entity.player = true;
    entity.position = {x, 0.0f, z};
    return entity;
}

const AiConductBoundTest kNoBound = [](std::size_t, const std::array<float, 3>&) { return false; };
// A made-up bound 0: |x| < 20.
const AiConductBoundTest kBoundZero = [](std::size_t bound, const std::array<float, 3>& point) { return bound == 0 && point[0] > -20.0f && point[0] < 20.0f; };

std::size_t countKind(const std::vector<AiConductEvent>& events, AiConductEventKind kind) {
    std::size_t n = 0;
    for (const AiConductEvent& event : events) n += event.kind == kind;
    return n;
}

void sideSlots() {
    AiConduct conduct;
    std::vector<AiConductEntity> entities;
    for (int i = 0; i < 7; ++i) entities.push_back(goon("goon" + std::to_string(i), 1000.0f, 0.0f));
    entities.push_back(thePlayer(0.0f, 0.0f));
    conduct.reset(entities);
    for (int i = 0; i < 7; ++i) conduct.engage(conduct.entities()[std::size_t(i)]);
    for (int i = 0; i < 6; ++i) require(conduct.entities()[std::size_t(i)].engaged && conduct.badSide()[std::size_t(i)] == "goon" + std::to_string(i), "0x206d18 fills the six slots in order");
    require(!conduct.entities()[6].engaged, "a seventh bad guy finds no slot");
    conduct.entities()[6].aggroCounter = 7;
    conduct.engage(conduct.entities()[6]);
    require(conduct.entities()[6].aggroCounter == 0, "0x206e74 clears ctrl+0x72 whenever the entity was not engaged");
    conduct.release(conduct.entities()[2]);
    require(!conduct.entities()[2].engaged && conduct.badSide()[2].empty(), "0x21decc frees the slot");
    conduct.engage(conduct.entities()[6]);
    require(conduct.badSide()[2] == "goon6", "the first free slot is taken");
    AiConductEntity& boss = conduct.entities()[0];
    boss.params.boss = true;
    conduct.release(boss);
    require(boss.engaged, "a boss keeps its slot unless its AI state is 1");
    boss.aiState = 1;
    conduct.release(boss);
    require(!boss.engaged, "a boss in AI state 1 releases");
    AiConductEntity good = goon("cop", 0.0f, 0.0f);
    good.params.badGuy = false;
    std::vector<AiConductEntity> two{good};
    conduct.reset(two);
    conduct.engage(conduct.entities()[0]);
    require(conduct.goodSide()[0] == "cop" && conduct.badSide()[0].empty(), "a character without AIBadGuy goes to the list 0x58dec8");
}

void sight() {
    AiConduct conduct;
    conduct.reset({goon("goon", 0.0f, 0.0f), thePlayer(30.0f, 0.0f)});
    const AiConductEntity& viewer = conduct.entities()[0];
    AiConductEntity& target = conduct.entities()[1];
    float distance = 0;
    require(conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound, &distance) && distance == 30.0f, "within AIViewDistance 50 without a bound");
    require(!conduct.sees(viewer, target, 29.0f, -1.0f, kNoBound), "beyond the limit");
    for (std::uint8_t mode : {1, 5, 6, 7}) {
        target.params.targetMode = mode;
        require(!conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "target modes 1 and 5 to 7 refuse");
    }
    target.params.targetMode = 4;
    require(conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "target mode 4 takes everybody");
    target.params.targetMode = 2;
    require(!conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "mode 2 only for the target's own ctrl+0xc4");
    target.target = "goon";
    require(conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "mode 2 with the viewer as its target");
    target.params.targetMode = 3;
    require(!conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "mode 3 only for the player as viewer");
    target.params.targetMode = 0;
    target.invisible = true;
    require(!conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "ctrl+0x316 bits 6..7 hide the target");
    conduct.entities()[0].params.seesInvisible = true;
    require(conduct.sees(viewer, target, -1.0f, -1.0f, kNoBound), "AISeesInvisibleChars");
    target.invisible = false;
    conduct.entities()[0].rangeBound = 0;
    require(!conduct.sees(viewer, target, -1.0f, -1.0f, kBoundZero), "AIRangeBound: the target at x 30 is outside");
    target.position = {10.0f, 0.0f, 0.0f};
    require(conduct.sees(viewer, target, -1.0f, -1.0f, kBoundZero), "AIRangeBound: the target at x 10 is inside");
    target.position = {30.0f, 0.0f, 0.0f};
    conduct.entities()[0].params.friendly = true;
    require(!conduct.sees(viewer, target, -1.0f, 40.0f, kBoundZero), "AIFriendly (ctrl+0xdb bit 2) is not the shortcut of 0x1f6aa0");
    conduct.entities()[0].params.closeSight = true;
    require(conduct.sees(viewer, target, -1.0f, 40.0f, kBoundZero), "ctrl+0xdb bit 1: a target within the second distance and below 45 degrees is seen whatever the bound");
    target.position = {25.0f, 26.0f, 0.0f};
    require(!conduct.sees(viewer, target, -1.0f, 40.0f, kBoundZero), "above 45 degrees the shortcut does not apply (and the target is outside the bound)");
}

void targets() {
    AiConduct conduct;
    AiConductEntity cop = goon("cop", 20.0f, 0.0f);
    cop.params.badGuy = false;
    conduct.reset({goon("goon", 0.0f, 0.0f), cop, thePlayer(40.0f, 0.0f)});
    conduct.engage(conduct.entities()[1]);
    AiConductEntity& self = conduct.entities()[0];
    const AiConductEntity* found = conduct.findTarget(self, 50.0f, -1.0f, kNoBound);
    require(found && found->name == "player1", "the player comes first when seen, even with a closer good guy");
    self.params.ignorePlayer = true;
    found = conduct.findTarget(self, 50.0f, -1.0f, kNoBound);
    require(found && found->name == "cop", "AIIgnorePlayer: the engaged good guy");
    conduct.entities()[1].params.targetMode = 1;
    require(!conduct.findTarget(self, 50.0f, -1.0f, kNoBound), "a good guy nobody may target is not found");
    self.params.ignorePlayer = false;
    self.aggroCounter = 3;
    self.target = "cop";
    found = conduct.findTarget(self, 50.0f, -1.0f, kNoBound);
    require(found && found->name == "cop", "with ctrl+0x72 set the current target stays");
    self.aggroCounter = 0;
    self.target.clear();
    conduct.entities()[2].position = {80.0f, 0.0f, 0.0f};
    require(!conduct.findTarget(self, 50.0f, -1.0f, kNoBound), "nothing within the view distance");
    AiConductEntity second = goon("cop2", 30.0f, 0.0f);
    second.params.badGuy = false;
    conduct.reset({goon("goon", 0.0f, 0.0f), goon("mate", 5.0f, 0.0f), cop, second, thePlayer(80.0f, 0.0f)});
    for (AiConductEntity& e : conduct.entities())
        if (!e.player) conduct.engage(e);
    conduct.entities()[1].target = "player1";
    conduct.entities()[2].target = "player1";
    conduct.entities()[2].aiState = 3;
    found = conduct.findTarget(conduct.entities()[0], 50.0f, -1.0f, kNoBound);
    require(found && found->name == "cop2", "the first unclaimed visible candidate: cop is claimed by the mate (0x1fbb54)");

    AiConductEntity claimed = goon("cop", 60.0f, 0.0f);
    claimed.params.badGuy = false;
    claimed.target = "cop2";
    claimed.aiState = 3;
    AiConductEntity self2 = goon("goon", 0.0f, 0.0f);
    self2.target = "cop2";
    conduct.reset({self2, claimed, second, thePlayer(80.0f, 0.0f)});
    for (AiConductEntity& e : conduct.entities())
        if (!e.player) conduct.engage(e);
    require(!conduct.findTarget(conduct.entities()[0], 50.0f, -1.0f, kNoBound),
            "0x1f906c..0x1f90b4: the own candidate, claimed once and not seen, ends the search with none (cop2 is not searched)");
    conduct.entities()[1].position = {40.0f, 0.0f, 0.0f};
    found = conduct.findTarget(conduct.entities()[0], 50.0f, -1.0f, kNoBound);
    require(found && found->name == "cop", "the own candidate when seen");
}

void idle() {
    AiConduct conduct;
    AiConductEntity bounded = goon("goon", 0.0f, 0.0f);
    bounded.rangeBound = 0;
    conduct.reset({bounded, thePlayer(30.0f, 0.0f)});
    conduct.update(kBoundZero);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(events.size() == 1 && events[0].kind == AiConductEventKind::Engaged && events[0].badSide && events[0].slot == 0,
            "outside the bound the idle goon only takes a slot (0x2a9c60 -> 0x206d18)");
    require(conduct.entities()[0].aiState == 0, "and stays idle");
    conduct.entities()[1].position = {10.0f, 0.0f, 0.0f};
    conduct.update(kBoundZero);
    events = conduct.takeEvents();
    require(events.size() == 1 && events[0].kind == AiConductEventKind::Sighted && events[0].target == "player1" && events[0].combat && events[0].distance == 10.0f,
            "inside the bound the goon sees the player");
    require(conduct.entities()[0].aiState == kAiCombatState && conduct.entities()[0].target == "player1" && conduct.entities()[0].seenBits == kAiSeenBits,
            "0x21102c: AI state 2 on the player, ctrl+0xdc bits 3..4 = 2");
    conduct.entities()[0].aiState = 0;
    conduct.update(kBoundZero);
    events = conduct.takeEvents();
    require(countKind(events, AiConductEventKind::Released) == 1 && countKind(events, AiConductEventKind::Sighted) == 1,
            "entering state 0 releases the slot (0x289650 -> 0x21decc), then the decision engages again on the sighting");

    conduct.reset({goon("goon", 0.0f, 0.0f), thePlayer(10.0f, 0.0f)});
    conduct.entities()[0].countdown = 2;
    conduct.update(kNoBound);
    require(conduct.entities()[0].countdown == 1 && conduct.entities()[0].aiState == 0, "ctrl+0x84 holds the search and counts down");
    conduct.entities()[0].countdown = 0;
    conduct.entities()[0].cantGetThere = true;
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == 0, "ctrl+0xd9 bit 2 skips the search");
    conduct.entities()[0].cantGetThere = false;
    conduct.entities()[0].locomotionState = kAiSightRefusedLocomotionState;
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == 0, "0x21102c does nothing in locomotion state 82");

    conduct.reset({goon("goon", 0.0f, 0.0f), thePlayer(100.0f, 0.0f)});
    conduct.entities()[0].params.millAbout = true;
    conduct.update(kNoBound);
    events = conduct.takeEvents();
    require(countKind(events, AiConductEventKind::MillAbout) == 1 && conduct.entities()[0].aiState == 9 && conduct.entities()[0].countdown == 3 &&
                conduct.entities()[0].engaged,
            "0x22471c: slot, AI state 9, ctrl+0x84 = 3");

    AiConductEntity friendly = goon("cop", 0.0f, 0.0f);
    friendly.params.badGuy = false;
    friendly.params.friendly = true;
    AiConductEntity foe = goon("goon", 5.0f, 0.0f);
    conduct.reset({friendly, foe, thePlayer(100.0f, 0.0f)});
    conduct.engage(conduct.entities()[1]);
    conduct.takeEvents();
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == 0 && conduct.entities()[0].engaged,
            "a friendly character sees but does not fight; at its spawn point it goes on to 0x2a9c60 and takes a slot");
    require(conduct.entities()[0].spawn && (*conduct.entities()[0].spawn)[0] == 0.0f, "the spawn point is its first position");

    conduct.reset({friendly, foe, thePlayer(100.0f, 0.0f)});
    conduct.entities()[0].spawn = std::array<float, 3>{1.84f, 0.0f, 0.0f};
    conduct.update(kNoBound);
    require(conduct.entities()[0].engaged, "within 8 x ctrl+0x8c (8 x 0.23) of the spawn point the slot is taken");
    conduct.reset({friendly, foe, thePlayer(100.0f, 0.0f)});
    conduct.entities()[0].spawn = std::array<float, 3>{1.9f, 0.0f, 0.0f};
    conduct.entities()[0].params.returnToSpawnPoint = false;
    conduct.update(kNoBound);
    require(!conduct.entities()[0].engaged, "farther without ReturnToSpawnPoint: the decision ends (0x2a9bf8)");
    conduct.reset({friendly, foe, thePlayer(100.0f, 0.0f)});
    conduct.entities()[0].spawn = std::array<float, 3>{1.9f, 0.0f, 0.0f};
    conduct.entities()[0].hidden = true;
    conduct.update(kNoBound);
    require(conduct.entities()[0].engaged, "farther with ReturnToSpawnPoint: 0x2a9c0c engages even when hidden");

    conduct.reset({goon("goon", 0.0f, 0.0f), thePlayer(100.0f, 0.0f)});
    conduct.update(kNoBound);
    conduct.entities()[0].hidden = true;
    conduct.update(kNoBound);
    require(!conduct.entities()[0].engaged, "a hidden engaged character releases its slot");
    conduct.entities()[0].params.boss = true;
    conduct.entities()[0].hidden = false;
    conduct.update(kNoBound);
    require(!conduct.entities()[0].engaged, "a boss does not take a slot in state 0");
    conduct.entities()[0].thinks = false;
    conduct.entities()[0].params.boss = false;
    conduct.update(kNoBound);
    require(!conduct.entities()[0].engaged, "no decision while the AI controller does not run");
}

void startRequest() {
    AiConductParams params;
    require(aiStartRequest(params) == 0, "idle");
    params.millAbout = true;
    require(aiStartRequest(params) == 0, "mill about alone is idle");
    params.friendly = true;
    require(aiStartRequest(params) == 9, "friendly and mill about");
    params.patrolPath = true;
    require(aiStartRequest(params) == 14, "a patrol path");
}

}

int main() {
    try {
        sideSlots();
        sight();
        targets();
        idle();
        startRequest();
    } catch (const std::exception& error) {
        std::cerr << "goon-conduct-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "goon-conduct-test: ok\n";
    return 0;
}

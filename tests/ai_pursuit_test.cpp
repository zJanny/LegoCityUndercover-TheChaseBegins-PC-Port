// The fight states of an AI character (src/runtime/ai_pursuit.*) on made-up characters. The rules are those read from game.arm.elf by
// 20261112-enemy-A (2026-11-12, llvm-objdump of 0x2c4708, 0x2bb558, 0x2b82ec, 0x2a92a4, 0x2896d4, 0x2c5d3c, 0x2c9d24, 0x1f6b40, 0x1f6d90, 0x2080a0,
// 0x22d85c, 0x2a136c, 0x2bcea0 and 0x26aa1c; docs/re/ai-states.md), not measured in the original. The table rows are those of
// `chase-tabledump tables/tablescharacters.bxls` and `tables/tablesweapons.bxls` (20261112-enemy-A, 2026-11-12): row 142 `ai_goon_fists`, the
// `CharacterEnum` of the mission 1-4 goons, and row 9 `police`; the goon's delays are the `AIMeleeFireDelay` block 30, 45 of lvl_01 (C++ level
// loader, same agent and date).
#include "runtime/ability_table.hpp"
#include "runtime/ai_pursuit.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;
namespace assets = chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr float kGoonMelee = 2.2f;

AiConductEntity goon(const std::string& name, float x, float z) {
    AiConductEntity entity;
    entity.name = name;
    entity.params.badGuy = true;
    entity.params.viewDistance = 50;
    entity.params.strike = true;
    entity.params.meleeDelay = 30;
    entity.params.meleeDelaySpan = 15;
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

const AiConductBoundTest kEverywhere = [](std::size_t, const std::array<float, 3>&) { return true; };
// A made-up bound 0: |x| < 20.
const AiConductBoundTest kBoundZero = [](std::size_t bound, const std::array<float, 3>& point) { return bound == 0 && point[0] > -20.0f && point[0] < 20.0f; };

std::vector<AiConductEvent> ofKind(const std::vector<AiConductEvent>& events, AiConductEventKind kind) {
    std::vector<AiConductEvent> found;
    for (const AiConductEvent& event : events)
        if (event.kind == kind) found.push_back(event);
    return found;
}

float distance(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

// A walker that reaches its destination in the follow after the start, like a path of one step.
struct OneStepWalker {
    AiConduct& conduct;
    std::array<float, 3> destination{};
    unsigned starts = 0;
    bool reachable = true;
    void install() {
        AiConductWalker walker;
        walker.start = [this](const AiConductEntity&, const std::array<float, 3>& where) {
            ++starts;
            destination = where;
            return reachable;
        };
        walker.follow = [this](const AiConductEntity& entity) {
            if (!reachable) return 1;
            AiConductEntity* moved = conduct.find(entity.name);
            if (moved->position == destination) return 0;
            moved->position = destination;
            return 4;
        };
        conduct.setWalker(walker);
    }
};

void combatBehaviourAndWeaponMode() {
    require(aiCombatBehaviour(0, false, false) == 3 && aiCombatBehaviour(0, true, false) == 2 && aiCombatBehaviour(0, false, true) == 1 && aiCombatBehaviour(0, true, true) == 0,
            "0x2bceec: behaviour 0 becomes what the character can do");
    require(aiCombatBehaviour(1, true, false) == 3 && aiCombatBehaviour(1, false, true) == 1 && aiCombatBehaviour(2, false, true) == 3 && aiCombatBehaviour(2, true, false) == 2,
            "0x2bcebc: 1 needs a ranged weapon, 2 a melee weapon");
    require(aiCombatBehaviour(4, false, false) == 4 && aiCombatBehaviour(7, false, false) == 7 && aiCombatBehaviour(3, true, true) == 3, "the other values stay");

    require(aiWeaponMode(3, true, true, 2.2f, 15.0f, 1.0f) == 3, "0x1f6d98: behaviour 3 wants no weapon");
    require(aiWeaponMode(2, true, false, 2.2f, 15.0f, 3.3f) == 2 && aiWeaponMode(2, true, false, 2.2f, 15.0f, 3.4f) == 3, "melee within 1.5 x AIMeleeFireDistance");
    require(aiWeaponMode(0, true, true, 2.2f, 15.0f, 3.0f) == 2 && aiWeaponMode(0, true, true, 2.2f, 15.0f, 10.0f) == 1 && aiWeaponMode(0, true, true, 2.2f, 15.0f, 16.0f) == 3,
            "behaviour 0 with both: melee near, ranged up to AIFireDistance");
    require(aiWeaponMode(1, false, true, 2.2f, 15.0f, 15.0f) == 1 && aiWeaponMode(1, false, true, 2.2f, 15.0f, 15.1f) == 3, "ranged only: within AIFireDistance");
    require(aiWeaponMode(4, false, false, 2.2f, 15.0f, 100.0f) == 4 && aiWeaponMode(5, true, true, 2.2f, 15.0f, 10.0f) == 3, "behaviour 4 is the third weapon, 5 has no ranged mode");
}

assets::TableSheet sheetOf(std::size_t stride, const std::vector<std::vector<std::pair<std::size_t, std::uint8_t>>>& rows) {
    assets::TableSheet sheet;
    sheet.stride = stride;
    sheet.rows = unsigned(rows.size());
    sheet.rowData.assign(stride * rows.size(), 0);
    for (std::size_t r = 0; r < rows.size(); ++r)
        for (const auto& [offset, value] : rows[r]) sheet.rowData[r * stride + offset] = value;
    return sheet;
}

void combatFactsOfTheRows() {
    const auto mask = [](const std::array<std::uint8_t, 8>& bytes, std::uint8_t melee, std::uint8_t ranged) {
        std::vector<std::pair<std::size_t, std::uint8_t>> row;
        for (std::size_t i = 0; i < 8; ++i) row.push_back({kCharacterAbilityMaskOffset + i, bytes[i]});
        row.push_back({kCharacterRowMeleeWeapon, melee});
        row.push_back({kCharacterRowRangedWeapon, ranged});
        return row;
    };
    assets::Table characters, weapons;
    characters.sheets.push_back(sheetOf(40, {mask({49, 78, 2, 0, 1, 0, 64, 0}, 13, 0), mask({119, 206, 26, 2, 3, 192, 3, 32}, 13, 8), mask({119, 206, 26, 2, 3, 192, 3, 32}, 13, 2),
                                             mask({115, 206, 26, 2, 3, 192, 3, 32}, 13, 1)}));
    weapons.sheets.push_back(sheetOf(24, {{}, {{kWeaponRowAbility, 2}}, {}}));
    const AiCombatFacts fists = aiCombatFacts(characters, weapons, 0);
    require(!fists.melee && !fists.ranged && fists.strike && !fists.charge, "row 142 ai_goon_fists: no ability 19 or 20, ability 54, no ability 63");
    const AiCombatFacts police = aiCombatFacts(characters, weapons, 1);
    require(police.melee && !police.ranged, "a ranged row beyond the weapon table gives no ranged weapon");
    require(aiCombatFacts(characters, weapons, 2).ranged, "0x21e7e8: a weapon row without an ability");
    require(aiCombatFacts(characters, weapons, 1 + 2).melee && !aiCombatFacts(characters, weapons, 3).ranged, "0x21e7f0: the weapon row's ability 2 is missing");
    require(!aiCombatFacts(characters, weapons, 9).melee, "a row beyond the table");

    AiConductParams params;
    params.combatBehaviour = 0;
    applyAiCombatFacts(params, fists);
    require(params.combatBehaviour == 3 && params.strike && !params.melee && !params.closeSight, "a mission 1-4 goon: AICombatBehaviour 0 becomes 3");
}

void pointsAndCooldown() {
    const std::array<float, 3> near = aiPointNear({10.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}, 1.76f);
    require(std::fabs(near[0] - 11.76f) < 1e-5f && near[1] == 0.0f && near[2] == 0.0f, "0x2a13e4: the point 0.8 x range from the target towards the character");
    const std::array<float, 3> same = aiPointNear({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}, 2.0f);
    require(same[0] == 1.0f && same[1] == 2.0f && same[2] == 5.0f, "0x257ea0: a zero vector normalises to (0, 0, 1)");
    std::array<std::uint16_t, 2> cooldowns{0, 10};
    aiSideCooldownStep(cooldowns);
    require(cooldowns[0] == 0 && cooldowns[1] == 9, "0x26aa1c counts both words down to 0");
    require(aiFightState(2) && aiFightState(3) && aiFightState(4) && aiFightState(5) && aiFightState(31) && aiFightState(31)->enter && !aiFightState(0) && !aiFightState(14) &&
                !aiFightState(25),
            "the states of the table");
}

// A goon alone with the player: sighting, state 3, the chase of state 5 to 0.8 x range, state 3 again, the wind-up request and the stand-in idle.
void aGoonRunsUpAndStrikes() {
    EngagedTable table;
    AiConduct conduct;
    conduct.reset(table, {goon("goon", 12.0f, 0.0f), thePlayer(0.0f, 0.0f)});
    OneStepWalker walker{conduct};
    walker.install();
    AiConductEntity& self = conduct.entities()[0];

    conduct.update(kEverywhere);
    require(self.aiState == kAiPursueState && self.target == "player1" && self.engaged, "the idle decision sees the player: AI state 2");
    conduct.takeEvents();

    conduct.update(kEverywhere);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(self.aiState == kAiAttackState && ofKind(events, AiConductEventKind::FightState).size() == 1 && ofKind(events, AiConductEventKind::FightState)[0].state == 3,
            "0x2bb740: with a ticket and the target inside the bound, state 2 hands over to state 3");
    require(self.destination == self.position && self.heading && *self.heading == 49152, "0x2080a0 keeps the position; the heading is towards the player (-x: 0xc000)");

    conduct.update(kEverywhere);
    require(self.aiState == kAiAttackState && conduct.sideCooldown(true) == kAiSideCooldown && walker.starts == 0 && conduct.takeEvents().empty(),
            "0x289a0c: with behaviour 3 the attack routine takes the side cool-down and answers 0");

    conduct.update(kEverywhere);
    events = conduct.takeEvents();
    require(self.aiState == kAiChaseState && walker.starts == 1 && self.countdown == kAiTargetDelayDefault, "0x2a96d0: out of range, the chase starts: state 5, ctrl+0x84 = AITargetDelay");
    require(std::fabs(walker.destination[0] - kGoonMelee * kAiChaseFactor) < 1e-5f && walker.destination[2] == 0.0f && ofKind(events, AiConductEventKind::RunTo).size() == 1,
            "0x2a136c: the run ends 0.8 x 2.2 from the player");
    require(conduct.sideCooldown(true) == kAiSideCooldown - 1, "the cool-down runs");

    self.conductWalk = true;
    conduct.update(kEverywhere);
    require(self.aiState == kAiChaseState && std::fabs(self.position[0] - 1.76f) < 1e-5f, "state 5 follows the path");
    conduct.takeEvents();
    conduct.update(kEverywhere);
    events = conduct.takeEvents();
    require(self.aiState == kAiAttackState && self.countdown == kAiTargetDelayDefault, "0x2c5ff4: within range the chase hands over to state 3");
    self.conductWalk = false;

    for (unsigned i = 0; i < kAiTargetDelayDefault; ++i) {
        conduct.update(kEverywhere);
        require(self.aiState == kAiAttackState && self.countdown == kAiTargetDelayDefault - 1 - i, "0x2a9390: state 3 counts ctrl+0x84 down");
    }
    conduct.takeEvents();
    conduct.update(kEverywhere);
    events = conduct.takeEvents();
    require(self.aiState == kAiWindUpState && (self.weaponBits & kAiWeaponMelee) && ofKind(events, AiConductEventKind::FightState).back().state == kAiWindUpState,
            "0x2b842c: ability 54, a ticket and the player within 3: AI state 31, whose enter takes the melee weapon out");

    for (int i = 0; i < 5; ++i) conduct.update(kEverywhere);
    require(self.aiState == kAiWindUpState && conduct.takeEvents().empty(), "STAND-IN SCRIPT-S343: the goon holds while the player is within 4.5 (0x2c9dec)");

    conduct.entities()[1].position = {-10.0f, 0.0f, 0.0f};
    conduct.update(kEverywhere);
    events = conduct.takeEvents();
    require(self.aiState == 0 && self.target.empty() && !self.engaged && self.countdown >= 30 && self.countdown < 45 && ofKind(events, AiConductEventKind::Idle).size() == 1,
            "0x2c9eac: out of reach the wind-up ends: idle, ctrl+0x84 = 30 + random % 15");
    bool again = false;
    for (int i = 0; i < 60 && !again; ++i) {
        conduct.update(kEverywhere);
        again = self.aiState == kAiPursueState && self.target == "player1";
    }
    require(again, "then it sees the player again");
}

// A second goon whose side already has a melee attacker on the player holds at 3 x AIMeleeFireDistance.
void withoutATicketTheGoonHolds() {
    EngagedTable table;
    AiConduct conduct;
    conduct.reset(table, {goon("first", 2.0f, 0.0f), goon("second", 15.0f, 0.0f), thePlayer(0.0f, 0.0f)});
    OneStepWalker walker{conduct};
    walker.install();
    AiConductEntity& first = conduct.entities()[0];
    AiConductEntity& second = conduct.entities()[1];
    conduct.engage(first);
    conduct.engage(second);
    first.aiState = kAiAttackState;
    first.target = "player1";
    first.weaponBits = kAiWeaponMelee;
    first.countdown = 1000;
    second.aiState = kAiPursueState;
    second.target = "player1";
    require(aiAttackTicket(conduct, first) && !aiAttackTicket(conduct, second), "0x1f6b40: one melee attacker per target");

    conduct.update(kEverywhere);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(second.aiState == kAiMoveState && walker.starts == 1 && std::fabs(walker.destination[0] - 3.0f * kGoonMelee) < 1e-4f, "0x2bb928: the hold point 3 x 2.2 from the player");
    require(second.countdown >= kAiRetryBase && second.countdown < kAiRetryBase + kAiRetrySpan, "0x2bb968: ctrl+0x84 = 10 + random % 10");

    second.conductWalk = true;
    conduct.update(kEverywhere);
    require(second.aiState == kAiMoveState && walker.starts == 2 && std::fabs(walker.destination[0] - kGoonMelee * kAiChaseFactor) < 1e-5f,
            "0x2c6024: the player is more than max(range, 5) from the hold point, so state 4 asks for the chase path and stays state 4");
    conduct.update(kEverywhere);
    require(second.aiState == kAiPursueState, "0x2c6020: within range and without a ticket state 4 ends in the sighting");
    second.conductWalk = false;

    conduct.update(kEverywhere);
    require(second.aiState == kAiAttackState && distance(second.position, {0.0f, 0.0f, 0.0f}) < kGoonMelee, "0x2bb6dc: within range state 2 hands over whatever the ticket");
}

// A character whose 3 x AIMeleeFireDistance is within 5 keeps its hold point.
void aShortRangeHolds() {
    EngagedTable table;
    AiConduct conduct;
    AiConductEntity near = goon("second", 15.0f, 0.0f);
    near.params.meleeFireDistance = 1.5f;
    conduct.reset(table, {goon("first", 1.0f, 0.0f), near, thePlayer(0.0f, 0.0f)});
    OneStepWalker walker{conduct};
    walker.install();
    AiConductEntity& first = conduct.entities()[0];
    AiConductEntity& second = conduct.entities()[1];
    conduct.engage(first);
    conduct.engage(second);
    first.aiState = kAiAttackState;
    first.target = "player1";
    first.weaponBits = kAiWeaponMelee;
    first.countdown = 1000;
    second.aiState = kAiPursueState;
    second.target = "player1";
    conduct.update(kEverywhere);
    second.conductWalk = true;
    conduct.takeEvents();
    for (int i = 0; i < 3; ++i) conduct.update(kEverywhere);
    second.conductWalk = false;
    require(ofKind(conduct.takeEvents(), AiConductEventKind::MoveEnded).size() == 1, "0x2c6304: the walk of state 4 ends with the AIMove event and a new sighting");
    require(walker.starts == 1 && std::fabs(second.position[0] - 4.5f) < 1e-4f && second.aiState == kAiPursueState, "the walk to 3 x 1.5 from the player, then state 2 again");
    second.countdown = 0;
    conduct.update(kEverywhere);
    require(second.aiState == kAiPursueState && walker.starts == 1, "0x2bb8d4: within 3 x AIMeleeFireDistance and without a ticket the character stays");
}

void boundAndTargetRules() {
    EngagedTable table;
    AiConduct conduct;
    AiConductEntity bounded = goon("bounded", 10.0f, 0.0f);
    bounded.rangeBound = 0;
    AiConductEntity lazy = goon("lazy", 0.0f, 30.0f);
    lazy.params.dontChase = true;
    conduct.reset(table, {bounded, lazy, thePlayer(25.0f, 0.0f)});
    OneStepWalker walker{conduct};
    walker.install();
    AiConductEntity& self = conduct.entities()[0];
    AiConductEntity& still = conduct.entities()[1];
    AiConductEntity& player = conduct.entities()[2];
    conduct.engage(self);
    conduct.engage(still);
    self.aiState = kAiPursueState;
    self.target = "player1";
    conduct.update(kBoundZero);
    require(self.aiState == 0 && self.target.empty(), "0x2bb700: a target outside the AIRangeBound is not seen: idle");

    player.position = {15.0f, 0.0f, 0.0f};
    self.aiState = kAiPursueState;
    self.target = "player1";
    self.countdown = 3;
    conduct.update(kBoundZero);
    require(self.aiState == kAiPursueState && self.countdown == 2 && walker.starts == 0, "0x2bb6e0: while ctrl+0x84 counts, state 2 waits");

    player.arrestTarget = true;
    conduct.update(kBoundZero);
    require(self.aiState == 0, "0x2c47a0: a target under arrest ends the fight");
    player.arrestTarget = false;

    still.aiState = kAiPursueState;
    still.target = "player1";
    still.countdown = 0;
    self.aiState = kAiAttackState;
    self.target = "player1";
    self.weaponBits = kAiWeaponMelee;
    self.countdown = 1000;
    conduct.update(kEverywhere);
    require(still.aiState == kAiPursueState && still.countdown == 0xffffffffu && walker.starts == 0, "0x2bb7ec: AIDontChase only counts ctrl+0x84 down, below 0 too");

    self.aiState = kAiPursueState;
    self.weaponBits = 0;
    self.countdown = 0;
    self.position = {30.0f, 0.0f, 0.0f};
    player.position = {10.0f, 0.0f, 0.0f};
    conduct.takeEvents();
    conduct.update(kBoundZero);
    const std::vector<AiConductEvent> events = conduct.takeEvents();
    require(self.aiState == kAiAttackState && ofKind(events, AiConductEventKind::OutsideBound).size() == 1 && self.moveWanted,
            "0x1fab60: a character outside its AIRangeBound gets the move bit (STAND-IN SCRIPT-S342: the walk back is not applied)");
}

void aBlockedMove() {
    EngagedTable table;
    AiConduct conduct;
    conduct.reset(table, {goon("goon", 12.0f, 0.0f), thePlayer(0.0f, 0.0f)});
    OneStepWalker walker{conduct};
    walker.reachable = false;
    walker.install();
    AiConductEntity& self = conduct.entities()[0];
    conduct.engage(self);
    self.aiState = kAiAttackState;
    self.target = "player1";
    conduct.sideCooldown(true) = 5;
    conduct.update(kEverywhere);
    const std::vector<AiConductEvent> events = conduct.takeEvents();
    require(walker.starts == 1 && ofKind(events, AiConductEventKind::CantGetThere).size() == 1 && self.aiState == kAiPursueState &&
                self.countdown >= kAiRetryBase && self.countdown < kAiRetryBase + kAiRetrySpan,
            "0x2a9714: a chase the path finder refuses goes back to state 2 with ctrl+0x84 = 10 + random % 10");
}

}

int main() {
    try {
        combatBehaviourAndWeaponMode();
        combatFactsOfTheRows();
        pointsAndCooldown();
        aGoonRunsUpAndStrikes();
        withoutATicketTheGoonHolds();
        aShortRangeHolds();
        boundAndTargetRules();
        aBlockedMove();
    } catch (const std::exception& error) {
        std::cerr << "ai-pursuit-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-pursuit-test: ok\n";
    return 0;
}

#include "runtime/ai_pursuit.hpp"

#include "runtime/ability_table.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

namespace chase::runtime {

namespace {

constexpr float kHeadingUnitsPerRadian = 10430.378f;     // literal 0x2bb888
constexpr std::uint32_t kTinyLengthBits = 0x34000000;    // 0x257ec8: a shorter vector normalises to (0, 0, 1)
constexpr std::uint16_t kPressSwitchMelee = 0x200;       // ctrl+0xe bits of 0x1f6e40
constexpr std::uint16_t kPressSwitchRanged = 0x400;
constexpr std::uint16_t kPressSwitchThird = 0x800;
constexpr std::uint16_t kPressAttackMask = kAiPressMelee | kAiPressRanged | kAiPressOther;
constexpr std::uint16_t kLocomotionJump = 4;             // 0x2bb688
constexpr std::uint16_t kLocomotionChargeFirst = 163;    // 0x21dfd8: states 163 and 164
constexpr std::uint16_t kLocomotionChargeLast = 164;
constexpr std::uint8_t kSeenBits = 2;                    // 0x21102c: ctrl+0xdc bits 3..4

float distanceBetween(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    const float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return std::sqrt(x * x + y * y + z * z);
}

std::uint16_t headingToward(const std::array<float, 3>& from, const std::array<float, 3>& to) {
    return std::uint16_t(std::int32_t(std::atan2(to[0] - from[0], to[2] - from[2]) * kHeadingUnitsPerRadian));
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// 0x235498(n) is 0x25b2b8() modulo n; a span of 0 (a delay block whose second value is below the first by a multiple of 256) is taken as 1.
std::uint32_t randomBelow(AiConduct& conduct, std::uint32_t n) { return conduct.random() % std::max<std::uint32_t>(n, 1); }

bool invisibleTo(const AiConductEntity& viewer, const AiConductEntity& target) { return !viewer.params.seesInvisible && target.invisible; }

float attackRange(const AiConductEntity& self) {
    return self.params.closeSight && (self.weaponBits & kAiWeaponRanged) ? self.params.fireDistance : self.params.meleeFireDistance;
}

void requestState(AiConduct& conduct, AiConductEntity& self, std::uint8_t state) {
    self.aiState = state;
    AiConductEvent event;
    event.kind = AiConductEventKind::FightState;
    event.character = self.name;
    event.state = state;
    event.target = self.target;
    conduct.push(event);
    if (const AiFightState* record = aiFightState(state); record && record->enter) record->enter(conduct, self);
}

void retryCountdown(AiConduct& conduct, AiConductEntity& self) { self.countdown = randomBelow(conduct, kAiRetrySpan) + kAiRetryBase; }

void sightOrIdle(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest& inside) {
    if (const AiConductEntity* found = conduct.findTarget(self, float(self.params.viewDistance), -1.0f, inside))
        conduct.sighted(self, *found, distanceBetween(found->position, self.position));
    else
        conduct.requestIdle(self);
}

// 0x1f6e40: the weapon switch presses (ctrl+0xe bits 9..11) by `AICombatBehaviour`.
void weaponSwitchPress(AiConductEntity& self, const AiConductEntity* target) {
    if (!target) return;
    const AiConductParams& p = self.params;
    const std::uint8_t mode = aiWeaponMode(p.combatBehaviour, p.melee, p.closeSight, p.meleeFireDistance, p.fireDistance, distanceBetween(target->position, self.position));
    const bool ranged = self.weaponBits & kAiWeaponRanged, melee = self.weaponBits & kAiWeaponMelee, third = self.weaponBits & kAiWeaponThird;
    switch (p.combatBehaviour) {
    case 0:
        if (mode == 1 && !ranged) self.pressBits |= kPressSwitchRanged;
        if (mode == 2 && !melee) self.pressBits |= kPressSwitchMelee;
        if (mode == 4 && !third) self.pressBits |= kPressSwitchThird;
        break;
    case 1:
        if (mode == 1 && !ranged) self.pressBits |= kPressSwitchRanged;
        break;
    case 2:
        if (mode == 2 && !melee) self.pressBits |= kPressSwitchMelee;
        break;
    case 4:
        if (!third) self.pressBits |= kPressSwitchThird;
        break;
    case 5:
        if (mode == 2 && !third) self.pressBits |= kPressSwitchMelee;
        break;
    default: break;
    }
}

// 0x1f5114(entity, target, reach): the strike of AI state 31 can start. STAND-IN SCRIPT-S342: the two points are the node positions and the
// target's radius is 0 (the original measures between the entity offsets +0x60 and takes off the target's +0x5c), and the tests 0x219ba0,
// 0x20bf3c and 0x219990 pass.
bool strikeReachable(const AiConductEntity& self, const AiConductEntity& target, float reach) {
    if (!target.player || !self.landed || !target.landed) return false;
    return !(distanceBetween(self.position, target.position) > reach);
}

void press(AiConduct& conduct, AiConductEntity& self, std::uint16_t bit) {
    self.pressBits |= bit;
    AiConductEvent event;
    event.kind = AiConductEventKind::AttackPress;
    event.character = self.name;
    event.target = self.target;
    event.press = bit;
    conduct.push(event);
}

// 0x2896d4(entity, mode): 0 nothing to do, 1 the target is out of range, 2 no target to attack, 3 an attack was pressed, 4 the same side.
int attack(AiConduct& conduct, AiConductEntity& self, const AiConductEntity* target, std::uint8_t mode) {
    if (!target || !target->present) return 2;
    if (target->params.badGuy == self.params.badGuy) return 4;
    switch (target->params.targetMode) {
    case 0: break;
    case 2:
        if (target->target != self.name) return 2;
        break;
    case 3:
        if (!self.player) return 2;
        break;
    default: return 2;
    }
    if (self.locomotionState >= kLocomotionChargeFirst && self.locomotionState <= kLocomotionChargeLast) return 0;
    const std::optional<std::uint16_t> saved = self.heading;
    if (!self.params.dontFaceTarget) self.heading = headingToward(self.position, target->position);
    if (self.params.lineOfSight && kAiSightRayBlockedStandIn) return 1;
    bool attacks = false;
    if (std::int32_t(self.countdown) <= 0) {
        attacks = self.params.boss;
        if (std::uint16_t& cooldown = conduct.sideCooldown(self.params.badGuy); !attacks && cooldown == 0) {
            cooldown = kAiSideCooldown;
            attacks = true;
        }
    }
    if (attacks) {
        switch (mode) {
        case 1:
            self.countdown = self.params.rangedDelay + randomBelow(conduct, self.params.rangedDelaySpan);
            press(conduct, self, kAiPressRanged);
            break;
        case 2:
        case 5:
            self.countdown = self.params.meleeDelay + randomBelow(conduct, self.params.meleeDelaySpan);
            if (self.params.meleeFireDistance < distanceBetween(self.position, target->position)) {
                self.heading = saved;
                return 1;
            }
            press(conduct, self, kAiPressMelee);
            break;
        case 3:
            self.heading = saved;
            return 0;
        case 4:
            self.countdown = self.params.rangedDelay + randomBelow(conduct, self.params.rangedDelaySpan);
            press(conduct, self, kAiPressOther);
            break;
        default: self.countdown = randomBelow(conduct, kAiIdleAttackSpan) + kAiIdleAttackBase; break;
        }
    }
    const float range = self.weaponBits & kAiWeaponRanged ? self.params.fireDistance : self.params.meleeFireDistance;
    if (distanceBetween(target->position, self.position) > range) {
        self.heading = saved;
        self.pressBits &= std::uint16_t(~kPressAttackMask);
        return 1;
    }
    return self.pressBits & kPressAttackMask ? 3 : 0;
}

// 0x2c4708 and 0x2bb558.
void pursueDecision(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest& inside) {
    if (const AiConductEntity* current = conduct.named(self.target); current && current->arrestTarget) {
        conduct.requestIdle(self);
        return;
    }
    const AiConductEntity* target = conduct.targetOf(self);
    weaponSwitchPress(self, target);
    if (!target) {
        target = conduct.findTarget(self, float(self.params.viewDistance), -1.0f, inside);
        self.target = target ? target->name : std::string();
        if (!target) {
            conduct.release(self);
            return;
        }
    }
    if (!conduct.sees(self, *target, -1.0f, -1.0f, inside)) {
        if (self.params.millAbout) conduct.requestMillAbout(self);
        else conduct.requestIdle(self);
        return;
    }
    self.heading = headingToward(self.position, target->position);
    if (self.locomotionState == kLocomotionJump) {
        self.moveWanted = true;
        return;
    }
    const float range = attackRange(self);
    const float distance = distanceBetween(target->position, self.position);
    const bool ticket = aiAttackTicket(conduct, self);
    bool attacking = range > distance;
    if (!attacking && self.countdown == 0 && ticket) attacking = !self.rangeBound || !inside || inside(*self.rangeBound, target->position);
    if (attacking) {
        aiAttackTarget(conduct, self, *target);
    } else {
        bool taunting = false;
        if (self.countdown == 0) {
            const bool drawn = randomBelow(conduct, kAiTauntChance) == 0;
            taunting = drawn && kAiTauntBitStandIn && !invisibleTo(self, *target) && self.locomotionState >= 1 && self.locomotionState <= 3;
        }
        if (taunting) {
            AiConductEvent event;
            event.kind = AiConductEventKind::Taunt;
            event.character = self.name;
            event.target = self.target;
            conduct.push(event);
            requestState(conduct, self, kAiTauntState);
            retryCountdown(conduct, self);
        } else if (self.params.dontChase) {
            --self.countdown;
        } else if (self.countdown != 0) {
            --self.countdown;
        } else {
            const float hold = self.params.meleeFireDistance * kAiHoldFactor;
            if (!(distance <= hold && !ticket)) {
                const float scale = ticket ? range / (kAiTicketApproachDivisor * distance) : hold / distance;
                std::array<float, 3> destination;
                for (int i = 0; i < 3; ++i) destination[i] = (self.position[i] - target->position[i]) * scale + target->position[i];
                aiMoveTo(conduct, self, destination, false, self.params.dontRun, inside);
            }
            retryCountdown(conduct, self);
        }
    }
    if (self.aiState == kAiPursueState && self.countdown == 0) {
        if (self.hidden) {
            conduct.requestIdle(self);
            retryCountdown(conduct, self);
        } else if (self.aggroCounter == 0) {
            self.target.clear();
            const AiConductEntity* found = conduct.findTarget(self, float(self.params.viewDistance), -1.0f, inside);
            if (!found) {
                conduct.requestIdle(self);
                return;
            }
            self.target = found->name;
        }
    }
    aiKeepInBound(conduct, self, inside);
    if (self.aggroCounter != 0) --self.aggroCounter;
}

// 0x2b82ec and 0x2a92a4. Not ported: the charge of ability 63 (AI state 33, 0x2b8320) and the test of sub +0x12e bit 5 against a player in the
// air (0x2b8434); the aim 0x205fbc of a ranged weapon.
void attackDecision(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest& inside) {
    const AiConductEntity* target = conduct.targetOf(self);
    if (self.countdown == 0 && target && self.params.strike && aiAttackTicket(conduct, self) && strikeReachable(self, *target, kAiStrikeReach)) {
        requestState(conduct, self, kAiWindUpState);
        return;
    }
    const AiConductEntity* current = conduct.named(self.target);
    if (current && current->arrestTarget) {
        conduct.requestIdle(self);
        return;
    }
    weaponSwitchPress(self, target);
    if (!current) {
        conduct.requestIdle(self);
        return;
    }
    const float distance = target ? distanceBetween(self.position, target->position) : FLT_MAX;
    if (std::int32_t(self.countdown) > 0) {
        --self.countdown;
    } else {
        const float range = attackRange(self);
        int result = 2;
        if (current->present) {
            const AiConductParams& p = self.params;
            const std::uint8_t mode = aiWeaponMode(p.combatBehaviour, p.melee, p.closeSight, p.meleeFireDistance, p.fireDistance, distanceBetween(self.position, current->position));
            result = attack(conduct, self, current, mode);
        }
        if (result == 3) {
            if (!self.params.ignorePlayer && self.aggroCounter == 0 && (distance > range || self.params.boss)) {
                if (const AiConductEntity* found = conduct.findTarget(self, float(self.params.viewDistance), -1.0f, inside)) self.target = found->name;
                else conduct.requestIdle(self);
            }
        } else if (result != 0) {
            bool chasing = false;
            if (result == 1 && !self.params.dontChase && target && aiChaseTarget(conduct, self, *target, range, self.params.dontRun, inside)) {
                self.countdown = self.params.targetDelay;
                requestState(conduct, self, kAiChaseState);
                chasing = true;
            }
            if (!chasing) {
                if (result == 1) retryCountdown(conduct, self);
                const AiConductEntity* found = conduct.findTarget(self, float(self.params.viewDistance), -1.0f, inside);
                if (found && (found->name != self.target || result == 1)) conduct.sighted(self, *found, distanceBetween(found->position, self.position));
                else if (self.params.millAbout) conduct.requestMillAbout(self);
                else conduct.requestIdle(self);
            }
        }
    }
    const AiConductEntity* now = conduct.named(self.target);
    if (!aiAttackTicket(conduct, self)) {
        if (now) conduct.sighted(self, *now, distanceBetween(now->position, self.position));
        else self.seenBits = kSeenBits;
    } else if (now && invisibleTo(self, *now)) {
        conduct.sighted(self, *now, distanceBetween(now->position, self.position));
    }
    aiKeepInBound(conduct, self, inside);
    if (self.aggroCounter != 0) --self.aggroCounter;
}

// 0x2c5d3c for a walk the conduct started. Not ported: the low bits of ctrl+0xd8 that skip the chase part (taken as clear), 0x23d4c0 and the
// message 49 of a blocked walk (0x244fc8), the next point of a simple path (ctrl+0xa8, 0x2c6218).
void moveDecision(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest& inside) {
    const std::uint8_t state = self.aiState;
    weaponSwitchPress(self, conduct.targetOf(self));
    if (self.aggroCounter != 0) --self.aggroCounter;
    const bool chase = state == kAiChaseState;
    const bool ticket = chase || aiAttackTicket(conduct, self);
    if (chase || (!self.params.ignorePlayer && self.params.badGuy && !self.avoidMove)) {
        const AiConductEntity* target = conduct.targetOf(self);
        if (!target) {
            target = conduct.findTarget(self, float(self.params.viewDistance), -1.0f, inside);
            self.target = target ? target->name : std::string();
        }
        if (target) {
            if (ticket && invisibleTo(self, *target)) {
                AiConductEvent event;
                event.kind = AiConductEventKind::LostTarget;
                event.character = self.name;
                event.target = self.target;
                conduct.push(event);
                return;
            }
            const float distance = distanceBetween(target->position, self.position);
            if (self.params.friendly && !self.engaged && distance > float(self.params.viewDistance)) {
                if (self.params.millAbout) conduct.requestMillAbout(self);
                else conduct.requestIdle(self);
                return;
            }
            const float range = attackRange(self);
            if (distance < range) {
                if (!(self.params.lineOfSight && kAiSightRayBlockedStandIn)) {
                    if (ticket) aiAttackTarget(conduct, self, *target);
                    else conduct.sighted(self, *target, distance);
                    return;
                }
            } else if (distanceBetween(target->position, self.destination) > std::max(range, kAiRepathDistance)) {
                if (self.rangeBound && inside && !inside(*self.rangeBound, target->position)) {
                    conduct.requestIdle(self);
                    return;
                }
                if (aiChaseTarget(conduct, self, *target, range, self.walk, inside) && state != kAiMoveState) requestState(conduct, self, state);
            }
        }
    }
    if (self.aiState != state) return;
    const int code = conduct.walker().follow ? conduct.walker().follow(self) : 1;
    switch (aiFollowOutcome(code)) {
    case AiFollowOutcome::Arrived:
        if (const AiConductEntity* target = conduct.named(self.target); chase && target) {
            aiAttackTarget(conduct, self, *target);
        } else {
            AiConductEvent event;
            event.kind = AiConductEventKind::MoveEnded;
            event.character = self.name;
            conduct.push(event);
            sightOrIdle(conduct, self, inside);
        }
        break;
    case AiFollowOutcome::Blocked:
        if (std::int32_t(self.countdown) > 0) {
            --self.countdown;
            break;
        }
        if (self.avoidMove && conduct.walker().start && conduct.walker().start(self, self.destination)) break;
        {
            AiConductEvent event;
            event.kind = AiConductEventKind::CantGetThere;
            event.character = self.name;
            conduct.push(event);
        }
        sightOrIdle(conduct, self, inside);
        retryCountdown(conduct, self);
        break;
    case AiFollowOutcome::Moving: break;
    }
    if (!self.avoidMove) aiKeepInBound(conduct, self, inside);
}

// 0x2c1ad4: ctrl+0x140 = the target and 0x206ad0(ctrl, 1, 1), which marks the melee weapon as out.
void windUpEnter(AiConduct&, AiConductEntity& self) { self.weaponBits |= kAiWeaponMelee; }

// 0x2c9d24. STAND-IN SCRIPT-S343: the wind-up runs in the locomotion states 304 to 306, which the held button bit 7 of ctrl+0xc starts and which are
// not ported. The character holds and faces its target (the branch of every other locomotion state) for as long as the test of states 304 and 305
// passes, 0x1f5114 with reach 4.5, and leaves like a wind-up that fails (0x2c9eac): idle, with the melee delay as the countdown. It never strikes.
void windUpDecision(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest&) {
    if (const AiConductEntity* target = conduct.targetOf(self); target && strikeReachable(self, *target, kAiWindUpReach)) {
        self.heading = headingToward(self.position, target->position);
        return;
    }
    conduct.requestIdle(self);
    self.countdown = self.params.meleeDelay + randomBelow(conduct, self.params.meleeDelaySpan);
}

constexpr AiFightState kFightStates[] = {
    {kAiPursueState, pursueDecision, nullptr, "pursue"},
    {kAiAttackState, attackDecision, nullptr, "attack"},
    {kAiMoveState, moveDecision, nullptr, "move"},
    {kAiChaseState, moveDecision, nullptr, "chase"},
    {kAiWindUpState, windUpDecision, windUpEnter, "strike wind-up"},
};

}

std::uint8_t aiCombatBehaviour(std::uint8_t raw, bool melee, bool ranged) {
    const std::uint8_t behaviour = raw & 7;
    switch (behaviour) {
    case 0:
        if (!melee && !ranged) return 3;
        if (melee && !ranged) return 2;
        if (!melee) return 1;
        return 0;
    case 1: return ranged ? 1 : 3;
    case 2: return melee ? 2 : 3;
    default: return behaviour;
    }
}

std::uint8_t aiWeaponMode(std::uint8_t behaviour, bool melee, bool ranged, float meleeDistance, float fireDistance, float distance) {
    if (behaviour == 3) return 3;
    if (melee && (behaviour == 0 || behaviour == 2 || behaviour == 5) && meleeDistance * kAiMeleeModeFactor >= distance) return 2;
    if (behaviour == 4) return 4;
    if (!ranged || behaviour >= 2) return 3;
    if (!melee) return fireDistance >= distance ? 1 : 3;
    if (!(distance > meleeDistance * kAiMeleeModeFactor)) return 3;
    return distance > fireDistance ? 3 : 1;
}

AiCombatFacts aiCombatFacts(const assets::Table& characters, const assets::Table& weapons, unsigned characterRow) {
    AiCombatFacts facts;
    if (characters.sheets.empty()) return facts;
    const assets::TableSheet& sheet = characters.sheets[0];
    if (characterRow >= sheet.rows || sheet.stride <= kCharacterRowRangedWeapon) return facts;
    const std::uint8_t* row = sheet.rowData.data() + std::size_t(characterRow) * sheet.stride;
    const auto has = [row](int ability) { return ((row[kCharacterAbilityMaskOffset + (std::size_t(ability) >> 3)] >> (ability & 7)) & 1) != 0; };
    facts.melee = has(kAiAbilityMelee) && row[kCharacterRowMeleeWeapon] != 0;
    facts.strike = has(kAiAbilityStrike);
    facts.charge = has(kAiAbilityCharge);
    const std::int8_t rangedRow = std::int8_t(row[kCharacterRowRangedWeapon]);
    if (has(kAiAbilityRanged) && rangedRow != 0 && !weapons.sheets.empty()) {
        const assets::TableSheet& weaponSheet = weapons.sheets[0];
        if (rangedRow > 0 && unsigned(rangedRow) < weaponSheet.rows && weaponSheet.stride >= kWeaponRowAbility + 2) {
            const std::uint8_t* weapon = weaponSheet.rowData.data() + std::size_t(rangedRow) * weaponSheet.stride;
            const unsigned ability = weapon[kWeaponRowAbility] | unsigned(weapon[kWeaponRowAbility + 1]) << 8;
            facts.ranged = ability == 0 || (ability < kCharacterAbilityMaskBytes * 8 && has(int(ability)));
        }
    }
    return facts;
}

void applyAiCombatFacts(AiConductParams& params, const AiCombatFacts& facts) {
    params.melee = facts.melee;
    params.closeSight = facts.ranged;
    params.strike = facts.strike;
    params.charge = facts.charge;
    params.combatBehaviour = aiCombatBehaviour(params.combatBehaviour, facts.melee, facts.ranged);
}

std::array<float, 3> aiPointNear(const std::array<float, 3>& target, const std::array<float, 3>& from, float reach) {
    std::array<float, 3> v{from[0] - target[0], from[1] - target[1], from[2] - target[2]};
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (bitsOf(length) > kTinyLengthBits) {
        const float inverse = 1.0f / length;
        for (float& component : v) component *= inverse;
    } else {
        v = {0.0f, 0.0f, 1.0f};
    }
    return {v[0] * reach + target[0], v[1] * reach + target[1], v[2] * reach + target[2]};
}

const AiFightState* aiFightState(std::uint8_t state) {
    for (const AiFightState& record : kFightStates)
        if (record.state == state) return &record;
    return nullptr;
}

const char* aiFightStateName(std::uint8_t state) {
    if (state == kAiTauntState) return "taunt";
    const AiFightState* record = aiFightState(state);
    return record ? record->name : "";
}

bool aiAttackTicket(const AiConduct& conduct, const AiConductEntity& self) {
    if (self.params.boss) return true;
    const AiConductEntity* target = conduct.targetOf(self);
    const auto countAttackers = [&](std::uint8_t weapon, unsigned limit) {
        unsigned count = 0;
        for (const std::string& name : conduct.badSide()) {
            const AiConductEntity* member = conduct.named(name);
            if (!member || member == &self) continue;
            const std::uint8_t s = member->aiState;
            const bool attacks = s == 3 || s == 5 || s == 28 || (s >= 31 && s <= 34);
            const AiConductEntity* memberTarget = conduct.targetOf(*member);
            if (!attacks || !memberTarget || memberTarget != target) continue;
            if (member->weaponBits & weapon) ++count;
            if (count >= limit) return false;
        }
        return true;
    };
    if (!countAttackers(kAiWeaponMelee, kAiMeleeAttackersLimit)) return false;
    if (!(self.weaponBits & kAiWeaponRanged)) return true;
    return countAttackers(kAiWeaponRanged, kAiRangedAttackersLimit);
}

void aiAttackTarget(AiConduct& conduct, AiConductEntity& self, const AiConductEntity& target) {
    self.destination = self.position;
    self.target = target.name;
    self.cantGetThere = false;
    requestState(conduct, self, kAiAttackState);
}

bool aiMoveTo(AiConduct& conduct, AiConductEntity& self, const std::array<float, 3>& destination, bool avoid, bool walk, const AiConductBoundTest& inside) {
    if (!kAiGlobalMillAboutModeStandIn && self.rangeBound && inside && !inside(*self.rangeBound, destination) && !inside(*self.rangeBound, self.position)) return false;
    if (!self.engaged) conduct.engage(self);
    if (!self.engaged) return false;
    const std::uint8_t before = self.aiState;
    const bool walkBefore = self.walk;
    self.aiState = kAiMoveState;
    self.walk = walk;
    if (conduct.walker().start && !conduct.walker().start(self, destination)) {
        self.walk = walkBefore;
        AiConductEvent failed;
        failed.kind = AiConductEventKind::CantGetThere;
        failed.character = self.name;
        conduct.push(failed);
        requestState(conduct, self, before);
        return false;
    }
    self.destination = destination;
    self.avoidMove = avoid;
    self.countdown = kAiMoveCountdown;
    AiConductEvent event;
    event.kind = AiConductEventKind::RunTo;
    event.character = self.name;
    event.destination = destination;
    conduct.push(event);
    requestState(conduct, self, kAiMoveState);
    return true;
}

// STAND-IN SCRIPT-S342: the point the original measures from is the entity offset +0x60 moved to the target's node, with the height of the
// target's node (ctrl+0x240 for a target of kind 27 or 149); the port takes the target's node position.
bool aiChaseTarget(AiConduct& conduct, AiConductEntity& self, const AiConductEntity& target, float range, bool walk, const AiConductBoundTest& inside) {
    const float reach = range * kAiChaseFactor;
    if (!(reach < distanceBetween(self.position, target.position))) return false;
    if (aiMoveTo(conduct, self, aiPointNear(target.position, self.position, reach), false, walk, inside)) return true;
    if (!(kAiChaseFallbackRange < range)) return false;
    return aiMoveTo(conduct, self, target.position, false, walk, inside);
}

// STAND-IN SCRIPT-S342: only the test of the character's own position is ported. A character outside its bound gets the move bit ctrl+0xc bit 0
// (reported once); the walk that bit asks for, the turn to the bound's centre, the boss's AISuperJump back (0x1fab6c), the test of the next step
// of a moving character and the path reset of states 4 and 5 are not applied.
void aiKeepInBound(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest& inside) {
    const bool outside = self.rangeBound && inside && !inside(*self.rangeBound, self.position);
    if (outside && !self.moveWanted) {
        AiConductEvent event;
        event.kind = AiConductEventKind::OutsideBound;
        event.character = self.name;
        conduct.push(event);
    }
    self.moveWanted = outside;
}

void aiSideCooldownStep(std::array<std::uint16_t, 2>& cooldowns) {
    for (std::uint16_t& cooldown : cooldowns)
        if (cooldown != 0) --cooldown;
}

}

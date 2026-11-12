#include "runtime/goon_conduct.hpp"

#include "runtime/ai_pursuit.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

namespace chase::runtime {

namespace {

constexpr std::uint8_t kAiPatrolState = 14;
constexpr std::uint8_t kAiMillAboutState = 9;
constexpr std::uint8_t kAiNoState = 1;
constexpr std::uint32_t kAiMillAboutCountdown = 3;       // 0x224784: ctrl+0x84 = 3
constexpr std::uint8_t kAiNoStateYet = 255;
constexpr std::uint32_t kAiTinyLengthBits = 0x34000000;  // 0x257ec8: a shorter vector normalises to (0, 0, 1)
constexpr std::size_t kAiPlayerSlot = kAiSideSlots;      // 0x58dee0, entry 6 of the list 0x58dec8

std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type == 5 || param->type == 7) return std::int32_t(param->value);
    if (param->type == 0) {
        float value;
        std::memcpy(&value, &param->value, 4);
        return std::int32_t(value);
    }
    return fallback;
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 0) return fallback;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

std::string stringParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 1 || param->blocks.empty()) return {};
    const assets::Bytes& block = param->blocks[0];
    return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
}

std::uint32_t referenceParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    return param ? assets::objectReferenceHash(*param) : 0;
}

// 0x241624 with the two-value block of `AIMeleeFireDelay` / `AIRangedFireDelay` (0x2bcacc): the first value, and the second less the first (1 when equal).
void delayParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::uint8_t& first, std::uint8_t& span) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->blocks.empty() || param->blocks[0].size() < 8) return;
    std::uint32_t low, high;
    std::memcpy(&low, param->blocks[0].data(), 4);
    std::memcpy(&high, param->blocks[0].data() + 4, 4);
    first = std::uint8_t(low);
    span = std::uint8_t(high == low ? 1 : high - low);
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// 0x1fbb54(candidate, target): the candidate's own target (0x1f522c) is `target` and its AI state is one of 3, 5, 28 and 31 .. 34.
bool attacksSameTarget(const AiConductEntity& candidate, const AiConductEntity* candidateTarget, const AiConductEntity* target) {
    if (!candidateTarget || candidateTarget != target) return false;
    const std::uint8_t s = candidate.aiState;
    return s == 3 || s == 5 || s == 28 || s == 31 || s == 32 || s == 33 || s == 34;
}

}

AiConductParams aiConductParamsOf(const assets::LevelObject& object, const assets::Level& cls) {
    AiConductParams params;
    params.viewDistance = std::uint16_t(intParam(object, cls, "AIViewDistance", kAiViewDistanceDefault));
    params.meleeFireDistance = floatParam(object, cls, "AIMeleeFireDistance", kAiMeleeFireDistanceDefault);
    params.fireDistance = floatParam(object, cls, "AIFireDistance", kAiFireDistanceDefault);
    params.rangeBound = stringParam(object, cls, "AIRangeBound");
    params.targetMode = std::uint8_t(intParam(object, cls, "AIDontTarget", 0) & 7);
    params.ignorePlayer = (intParam(object, cls, "AIIgnorePlayer", 0) & 1) != 0;
    params.seesInvisible = (intParam(object, cls, "AISeesInvisibleChars", 0) & 1) != 0;
    params.friendly = (intParam(object, cls, "AIFriendly", 0) & 1) != 0;
    params.runSpeed = floatParam(object, cls, "AIRunSpeed", kAiRunSpeedDefault) * kAiRunSpeedScale;
    params.returnToSpawnPoint = intParam(object, cls, "ReturnToSpawnPoint", 1) != 0;
    params.millAbout = intParam(object, cls, "AIMillAbout", 0) != 0 || kAiConductGlobalMillAboutStandIn;
    params.patrolPathHash = referenceParam(object, cls, "AIPatrolPath");
    params.patrolPath = params.patrolPathHash != 0;
    params.dontRun = (intParam(object, cls, "AIDontRun", 0) & 1) != 0;
    params.patrolAlertOnLos = (intParam(object, cls, "AIPatrolAlertOnLOS", 0) & 1) != 0;
    params.idleReturn.sleepAnim = stringParam(object, cls, "AISleepAnim");
    params.idleReturn.sleepAnimWake = stringParam(object, cls, "AISleepAnimWake");
    params.idleReturn.waitTime = intParam(object, cls, "Wait_Time", kAiReturnWaitDefault);
    params.badGuy = intParam(object, cls, "AIBadGuy", 0) != 0;
    params.boss = intParam(object, cls, "Boss_IsBoss", 0) != 0;
    params.characterRow = std::uint8_t(intParam(object, cls, "CharacterEnum", kAiCharacterRowDefault));
    params.combatBehaviour = std::uint8_t(intParam(object, cls, "AICombatBehaviour", kAiCombatBehaviourDefault) & 7);
    params.dontChase = (intParam(object, cls, "AIDontChase", 0) & 1) != 0;
    params.lineOfSight = (intParam(object, cls, "AILineOfSight", 0) & 1) != 0;
    params.dontFaceTarget = (intParam(object, cls, "AIDontFaceTarget", 0) & 1) != 0;
    delayParam(object, cls, "AIMeleeFireDelay", params.meleeDelay, params.meleeDelaySpan);
    delayParam(object, cls, "AIRangedFireDelay", params.rangedDelay, params.rangedDelaySpan);
    params.targetDelay = std::uint8_t(intParam(object, cls, "AITargetDelay", kAiTargetDelayDefault));
    return params;
}

std::uint8_t aiStartRequest(const AiConductParams& params) {
    if (params.patrolPath) return kAiPatrolState;
    if (params.friendly && params.millAbout) return kAiMillAboutState;
    return 0;
}

void AiConduct::reset(EngagedTable& table, std::vector<AiConductEntity> entities) {
    table_ = &table;
    *table_ = EngagedTable{};
    entities_ = std::move(entities);
    lastState_.assign(entities_.size(), kAiNoStateYet);
    events_.clear();
}

AiConductEntity* AiConduct::find(const std::string& name) { return entityNamed(name); }

AiConductEntity* AiConduct::entityNamed(const std::string& name) {
    if (name.empty()) return nullptr;
    for (AiConductEntity& entity : entities_)
        if (entity.name == name) return &entity;
    return nullptr;
}

const AiConductEntity* AiConduct::entityNamed(const std::string& name) const {
    if (name.empty()) return nullptr;
    for (const AiConductEntity& entity : entities_)
        if (entity.name == name) return &entity;
    return nullptr;
}

const AiConductEntity* AiConduct::player() const {
    for (const AiConductEntity& entity : entities_)
        if (entity.player) return &entity;
    return nullptr;
}

const AiConductEntity* AiConduct::validTarget(const AiConductEntity& self) const { return entityNamed(self.target); }

void AiConduct::engage(AiConductEntity& self) {
    const EngageResult result = table_->engage(self.name, self.params.badGuy);
    if (result.counterCleared) self.aggroCounter = 0;
    self.engaged = table_->engaged(self.name);
    if (!result.joined) return;
    const std::array<std::string, kAiSideSlots>& side = table_->list(self.params.badGuy);
    AiConductEvent event;
    event.kind = AiConductEventKind::Engaged;
    event.character = self.name;
    event.badSide = self.params.badGuy;
    event.slot = std::size_t(std::find(side.begin(), side.end(), self.name) - side.begin());
    events_.push_back(event);
}

void AiConduct::release(AiConductEntity& self) {
    const std::array<std::string, kAiSideSlots>& side = table_->list(self.params.badGuy);
    const std::size_t slot = std::size_t(std::find(side.begin(), side.end(), self.name) - side.begin());
    if (!table_->release(self.name, self.params.badGuy, self.params.boss, self.aiState == kAiNoState)) return;
    self.engaged = false;
    AiConductEvent event;
    event.kind = AiConductEventKind::Released;
    event.character = self.name;
    event.badSide = self.params.badGuy;
    event.slot = slot;
    events_.push_back(event);
}

bool AiConduct::sees(const AiConductEntity& viewer, const AiConductEntity& target, float limit, float second, const AiConductBoundTest& inside,
                     float* distance, const float* angle) const {
    if (angle && !(viewer.present && target.present && aiAngleTest(viewer.position, viewer.forward, target.position, *angle))) return false;
    switch (target.params.targetMode) {
    case 0:
    case 4: break;
    case 2:
        if (target.target != viewer.name) return false;
        break;
    case 3:
        if (!viewer.player) return false;
        break;
    default: return false;
    }
    if (!viewer.params.seesInvisible && target.invisible) return false;
    if (!target.present) return false;
    if (limit < 0.0f) limit = float(viewer.params.viewDistance);
    if (second < 0.0f) second = viewer.params.meleeFireDistance;
    const float v[3] = {target.position[0] - viewer.position[0], target.position[1] - viewer.position[1], target.position[2] - viewer.position[2]};
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (distance) *distance = length;
    if (length > limit) return false;
    if (viewer.params.closeSight && !(length > second)) {
        const float y = bitsOf(length) > kAiTinyLengthBits ? v[1] * (1.0f / length) : 0.0f;
        if (std::int32_t(bitsOf(y)) < std::int32_t(kAiFriendlySightSineBits)) return true;
    }
    if (viewer.rangeBound && inside) return inside(*viewer.rangeBound, target.position);
    return true;
}

const AiConductEntity* AiConduct::findTarget(const AiConductEntity& self, float limit, float second, const AiConductBoundTest& inside, bool angleFlag) {
    const float watchAngle = aiWatchAngle(kAiWatchAngleDegrees);
    const float* angle = angleFlag ? &watchAngle : nullptr;
    if (limit < 0.0f) limit = float(self.params.viewDistance);
    if (second < 0.0f) second = self.params.meleeFireDistance;
    if (self.aggroCounter != 0)
        if (const AiConductEntity* current = validTarget(self)) return current;
    const AiConductEntity* thePlayer = player();
    if (kAiConductBossSearchStandIn && self.params.boss) {
        if (!thePlayer || !thePlayer->present) return nullptr;
        if (self.rangeBound && inside && !inside(*self.rangeBound, thePlayer->position)) return nullptr;
        return sees(self, *thePlayer, limit, second, inside, nullptr, angle) ? thePlayer : nullptr;
    }
    const AiConductEntity* playerSlot = !self.params.ignorePlayer && thePlayer && thePlayer->present ? thePlayer : nullptr;
    table_->setPlayerSlot(playerSlot ? playerSlot->name : std::string());
    std::vector<const AiConductEntity*> good, bad;
    for (const std::string& name : table_->list(false)) good.push_back(entityNamed(name));
    good.push_back(playerSlot);
    for (const std::string& name : table_->list(true)) bad.push_back(entityNamed(name));
    const std::vector<const AiConductEntity*>& candidates = self.params.badGuy ? good : bad;
    const std::vector<const AiConductEntity*>& own = self.params.badGuy ? bad : good;
    const std::size_t count = candidates.size();

    int playerIndex = -1;
    for (std::size_t i = 0; i < count; ++i)
        if (candidates[i] && thePlayer && candidates[i] == thePlayer) playerIndex = int(i);
    std::vector<std::uint8_t> claims(count, 0);
    std::uint32_t total = 0;
    int mine = -1;
    for (const AiConductEntity* member : own) {
        if (!member || member == thePlayer) continue;
        const AiConductEntity* memberTarget = validTarget(*member);
        for (std::size_t i = 0; i < count; ++i) {
            if (!candidates[i] || !attacksSameTarget(*candidates[i], validTarget(*candidates[i]), memberTarget)) continue;
            ++claims[i];
            ++total;
            if (member == &self) mine = int(i);
        }
    }
    if (playerIndex >= 0 && thePlayer->present && sees(self, *thePlayer, limit, second, inside, nullptr, angle)) return thePlayer;
    if (mine >= 0 && claims[std::size_t(mine)] <= 1)
        return sees(self, *candidates[std::size_t(mine)], limit, second, inside, nullptr, angle) ? candidates[std::size_t(mine)] : nullptr;

    std::vector<float> distances(count, 0.0f);
    for (std::size_t i = 0; i < count; ++i) {
        const AiConductEntity* candidate = candidates[i];
        if (!candidate || !candidate->present) continue;
        float measured = 0.0f;
        distances[i] = sees(self, *candidate, limit, second, inside, &measured, angle) ? measured : FLT_MAX;
        if (claims[i] == 0 && !(distances[i] > limit)) return candidate;
    }
    for (std::size_t least = 1; least < count && total != 0; ++least)
        for (std::size_t i = 0; i < count && total != 0; ++i) {
            if (claims[i] != least) continue;
            const AiConductEntity* candidate = candidates[i];
            if (candidate->present && distances[i] > 0.0f && !(distances[i] > limit)) return candidate;
            total -= claims[i];
        }
    return nullptr;
}

void AiConduct::sighted(AiConductEntity& self, const AiConductEntity& target, float distance, bool held) {
    if (self.locomotionState == kAiSightRefusedLocomotionState) return;
    self.seenBits = kAiSeenBits;
    if (!held) engage(self);
    AiConductEvent event;
    event.kind = AiConductEventKind::Sighted;
    event.character = self.name;
    event.target = target.name;
    event.distance = distance;
    if (held || self.engaged) {
        self.aiState = kAiCombatState;
        self.target = target.name;
        event.combat = true;
    }
    events_.push_back(event);
}

void AiConduct::thinkIdle(AiConductEntity& self, const AiConductBoundTest& inside) {
    if (!aiIdleGate(clock_)) return;
    bool awake = true;
    if (!self.params.idleReturn.sleepAnim.empty()) {
        const AiSleepStep step = aiSleepStep({self.seenBits, self.locomotionState, self.params.friendly, !self.params.idleReturn.sleepAnimWake.empty(), self.clipEnded}, [&] {
            const AiConductEntity* thePlayer = player();
            return thePlayer && sees(self, *thePlayer, -1.0f, -1.0f, inside);
        });
        self.seenBits = step.bits;
        awake = step.awake;
        if (step.clip != AiSleepClip::None) {
            AiConductEvent event;
            event.kind = AiConductEventKind::SleepClip;
            event.character = self.name;
            event.loop = step.clip == AiSleepClip::Sleep;
            event.clip = aiSleepClipName(event.loop ? self.params.idleReturn.sleepAnim : self.params.idleReturn.sleepAnimWake);
            events_.push_back(event);
        }
    }
    if (awake && !self.cantGetThere && self.countdown == 0) {
        if (const AiConductEntity* target = findTarget(self, float(self.params.viewDistance), -1.0f, inside); target && !self.params.friendly) {
            const float dx = target->position[0] - self.position[0], dy = target->position[1] - self.position[1], dz = target->position[2] - self.position[2];
            sighted(self, *target, std::sqrt(dx * dx + dy * dy + dz * dz));
            return;
        }
    }
    if (self.countdown != 0) --self.countdown;
    if (self.params.millAbout) {
        requestMillAbout(self);
        return;
    }
    if (self.params.friendly && self.spawn) {
        const float dx = self.position[0] - (*self.spawn)[0], dy = self.position[1] - (*self.spawn)[1], dz = self.position[2] - (*self.spawn)[2];
        if (std::sqrt(dx * dx + dy * dy + dz * dz) > kAiFriendlyHomeFactor * self.params.runSpeed) {
            if (self.params.returnToSpawnPoint) engage(self);
            return;
        }
    }
    if (self.params.boss) return;
    if (!self.engaged) {
        if (!self.hidden) engage(self);
        return;
    }
    if (self.hidden) {
        release(self);
        return;
    }
    if (aiReturnRequested(self.patrolObject != 0, self.params.returnToSpawnPoint, self.position, self.spawn)) requestReturn(self);
}

void AiConduct::requestMillAbout(AiConductEntity& self) {
    if (!self.params.millAbout) return;
    engage(self);
    self.aiState = kAiMillAboutState;
    self.countdown = kAiMillAboutCountdown;
    AiConductEvent event;
    event.kind = AiConductEventKind::MillAbout;
    event.character = self.name;
    events_.push_back(event);
}

void AiConduct::requestIdle(AiConductEntity& self) {
    self.aiState = 0;
    self.target.clear();
    release(self);
    AiConductEvent idle;
    idle.kind = AiConductEventKind::Idle;
    idle.character = self.name;
    events_.push_back(idle);
}

const AiConductEntity* AiConduct::targetOf(const AiConductEntity& self) const {
    const AiConductEntity* target = entityNamed(self.target);
    return target && target->present ? target : nullptr;
}

void AiConduct::requestReturn(AiConductEntity& self) {
    self.aiState = kAiReturnToSpawnState;
    AiConductEvent event;
    event.kind = AiConductEventKind::ReturnToSpawn;
    event.character = self.name;
    events_.push_back(event);
    enterReturn(self);
}

void AiConduct::enterReturn(AiConductEntity& self) {
    release(self);
    self.countdown = aiReturnCountdown(self.patrolObject != 0, self.params.idleReturn);
}

void AiConduct::thinkReturn(AiConductEntity& self, const AiConductBoundTest& inside) {
    if (aiReturnWaitStep(self.patrolObject != 0, self.params.returnToSpawnPoint, self.countdown)) requestPatrol(self);
    watch(self, inside);
}

void AiConduct::watch(AiConductEntity& self, const AiConductBoundTest& inside) {
    const AiConductEntity* target = findTarget(self, float(self.params.viewDistance), -1.0f, inside, self.params.patrolAlertOnLos);
    if (!target) return;
    AiConductEvent caught;
    caught.kind = AiConductEventKind::PlayerCaught;
    caught.character = self.name;
    caught.target = self.target;
    events_.push_back(caught);
    const float dx = target->position[0] - self.position[0], dy = target->position[1] - self.position[1], dz = target->position[2] - self.position[2];
    sighted(self, *target, std::sqrt(dx * dx + dy * dy + dz * dz), true);
}

void AiConduct::requestPatrol(AiConductEntity& self) {
    self.aiState = kAiReturnedState;
    AiConductEvent event;
    event.kind = AiConductEventKind::Returned;
    event.character = self.name;
    events_.push_back(event);
    enterPatrol(self);
}

void AiConduct::enterPatrol(AiConductEntity& self) {
    AiPatrolEnterInput input;
    input.patrolObject = self.patrolObject;
    input.pathParameter = self.params.patrolPathHash;
    input.returnToSpawnPoint = self.params.returnToSpawnPoint;
    input.dontRun = self.params.dontRun;
    input.walk = self.walk;
    const AiPatrolEnterStep step = aiPatrolEnter(input);
    self.patrolObject = step.patrolObject;
    self.walk = step.walk;
    if (!self.engaged) engage(self);
    std::optional<std::array<float, 3>> destination;
    if (step.source == AiPatrolSource::PatrolObject) {
        if (const std::optional<AiPatrolNode> node = patrolNodes_ ? patrolNodes_(step.patrolObject) : std::nullopt) destination = node->position;
    } else if (step.source == AiPatrolSource::SpawnPoint) {
        destination = self.spawn;
    }
    if (!destination) return;
    AiConductEvent event;
    event.kind = AiConductEventKind::PatrolWalk;
    event.character = self.name;
    event.source = step.source;
    event.patrolling = step.patrolling;
    events_.push_back(event);
    if (walker_.start && !walker_.start(self, *destination)) {
        AiConductEvent failed;
        failed.kind = AiConductEventKind::CantGetThere;
        failed.character = self.name;
        events_.push_back(failed);
    }
}

void AiConduct::thinkPatrol(AiConductEntity& self, const AiConductBoundTest& inside) {
    const int code = walker_.follow ? walker_.follow(self) : 1;
    switch (aiFollowOutcome(code)) {
    case AiFollowOutcome::Arrived: {
        std::uint32_t next = 0;
        if (self.patrolObject != 0 && patrolNodes_)
            if (const std::optional<AiPatrolNode> node = patrolNodes_(self.patrolObject)) next = node->next;
        const AiPatrolArrival arrival = aiPatrolArrival(self.patrolObject, self.params.returnToSpawnPoint, next);
        AiConductEvent event;
        event.kind = AiConductEventKind::WalkEnded;
        event.character = self.name;
        event.toWait = !arrival.idle;
        events_.push_back(event);
        if (arrival.idle) {
            self.aiState = 0;
            release(self);
            AiConductEvent idle;
            idle.kind = AiConductEventKind::Idle;
            idle.character = self.name;
            events_.push_back(idle);
            return;
        }
        self.patrolObject = arrival.patrolObject;
        requestReturn(self);
        return;
    }
    case AiFollowOutcome::Blocked:
        if (!aiPatrolBlockedStep(self.countdown, random_.next())) return;
        if (const AiConductEntity* target = findTarget(self, float(self.params.viewDistance), -1.0f, inside)) {
            const float dx = target->position[0] - self.position[0], dy = target->position[1] - self.position[1], dz = target->position[2] - self.position[2];
            sighted(self, *target, std::sqrt(dx * dx + dy * dy + dz * dz));
            return;
        }
        self.aiState = 0;
        self.target.clear();
        release(self);
        {
            AiConductEvent idle;
            idle.kind = AiConductEventKind::Idle;
            idle.character = self.name;
            events_.push_back(idle);
        }
        return;
    case AiFollowOutcome::Moving: watch(self, inside); return;
    }
}

void AiConduct::update(const AiConductBoundTest& inside) {
    for (AiConductEntity& entity : entities_) {
        entity.engaged = !entity.player && table_->engaged(entity.name);
        if (!entity.player && entity.present && !entity.spawn) entity.spawn = entity.position;
    }
    for (std::size_t i = 0; i < entities_.size(); ++i) {
        AiConductEntity& entity = entities_[i];
        if (entity.player || lastState_[i] == entity.aiState) continue;
        if (entity.aiState == 0 || entity.aiState == kAiNoState) release(entity);
        if (entity.aiState == kAiReturnToSpawnState) enterReturn(entity);
        if (entity.aiState == kAiReturnedState) enterPatrol(entity);
        if (const AiFightState* fight = aiFightState(entity.aiState); fight && fight->enter) fight->enter(*this, entity);
        lastState_[i] = entity.aiState;
    }
    aiSideCooldownStep(sideCooldowns_);
    for (std::size_t i = 0; i < entities_.size(); ++i) {
        AiConductEntity& entity = entities_[i];
        if (entity.player || !entity.present) continue;
        if (!entity.thinks) continue;
        entity.pressBits = 0;
        const AiFightState* fight = aiFightState(entity.aiState);
        if (fight && (entity.aiState == kAiMoveState || entity.aiState == kAiChaseState) && !entity.conductWalk) fight = nullptr;
        if (entity.aiState == 0) {
            thinkIdle(entity, inside);
        } else if (fight) {
            // 0x200c5c: a character in a state above 1 without a slot takes one, and goes idle when its side list is full.
            if (!entity.engaged) engage(entity);
            if (entity.engaged) fight->decision(*this, entity, inside);
            else requestIdle(entity);
        } else if (entity.aiState == kAiReturnToSpawnState) {
            thinkReturn(entity, inside);
        } else if (entity.aiState == kAiReturnedState) {
            thinkPatrol(entity, inside);
        } else {
            continue;
        }
        lastState_[i] = entity.aiState;
    }
}

std::vector<AiConductEvent> AiConduct::takeEvents() {
    std::vector<AiConductEvent> taken;
    taken.swap(events_);
    return taken;
}

}

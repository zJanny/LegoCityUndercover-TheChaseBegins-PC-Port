#include "runtime/ai.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    AiNativeKind kind;
    unsigned arguments;
    unsigned objects;
};

constexpr Entry kEntries[] = {
    {"aienable", AiNativeKind::AIEnable, 2, 1},
    {"aiactive", AiNativeKind::AIActive, 1, 1},
    {"aimove", AiNativeKind::AIMove, 3, 2},
    {"aimovenoavoid", AiNativeKind::AIMoveNoAvoid, 3, 2},
    {"aifacetarget", AiNativeKind::AIFaceTarget, 2, 2},
    {"aiignoreplayer", AiNativeKind::AIIgnorePlayer, 2, 1},
    {"ainostate", AiNativeKind::AINoState, 1, 1},
    {"characterinvulnerable", AiNativeKind::CharacterInvulnerable, 2, 1},
    {"aisuperjump", AiNativeKind::AISuperJump, 3, 2},
    {"airesetstate", AiNativeKind::AIResetState, 1, 1},
    {"aifollowsimplepath", AiNativeKind::AIFollowSimplePath, 5, 2},
    {"aipursuit", AiNativeKind::AIPursuit, 3, 2},
    {"aicantattack", AiNativeKind::AICantAttack, 2, 1},
    {"aistalkplayer", AiNativeKind::AIStalkPlayer, 2, 1},
    {"aimillabout", AiNativeKind::AIMillAbout, 2, 1},
    {"airunaway", AiNativeKind::AIRunAway, 1, 1},
};

constexpr float kHeadingUnitsPerRadian = 10430.378f;
constexpr const char* kPlayerAlias = "player1";

std::uint16_t headingToward(const std::array<float, 3>& from, const std::array<float, 3>& to) {
    const float angle = std::atan2(to[0] - from[0], to[2] - from[2]) * kHeadingUnitsPerRadian;
    return static_cast<std::uint16_t>(static_cast<std::int32_t>(angle));
}

}

std::optional<AiNativeKind> aiNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> aiNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned aiNativeArguments(AiNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

unsigned aiNativeObjects(AiNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.objects;
    return 0;
}

bool aiNativeIsStandIn(AiNativeKind) { return true; }

AiCharacter& AiCore::at(const std::string& name) { return characters_[name]; }

const AiCharacter* AiCore::character(const std::string& name) const {
    const auto found = characters_.find(name);
    return found == characters_.end() ? nullptr : &found->second;
}

bool AiCore::locate(const std::string& name, std::array<float, 3>& out) {
    if (const auto found = characters_.find(name); found != characters_.end() && found->second.position) {
        out = *found->second.position;
        return true;
    }
    float position[3];
    if (lookup_ && lookup_(name, position)) {
        out = {position[0], position[1], position[2]};
        return true;
    }
    if (std::find(unresolved_.begin(), unresolved_.end(), name) == unresolved_.end()) unresolved_.push_back(name);
    return false;
}

void AiCore::setPosition(const std::string& name, const float position[3]) {
    at(name).position = std::array<float, 3>{position[0], position[1], position[2]};
}

bool AiCore::moving(const std::string& name) const {
    const AiCharacter* found = character(name);
    return found && found->move && (found->state == AiState::MoveTo || found->state == AiState::FollowPath);
}

bool AiCore::movedLastUpdate(const std::string& name) const { return std::find(moved_.begin(), moved_.end(), name) != moved_.end(); }

void AiCore::requestIdle(AiCharacter& target) {
    target.state = AiState::Idle;
    target.move.reset();
}

void AiCore::requestMillAbout(AiCharacter& target) {
    if (target.millAbout.value_or(false)) target.state = AiState::MillAbout;
}

void AiCore::startMove(const std::string& name, std::vector<std::array<float, 3>> waypoints, bool run, bool noAvoid, bool path) {
    AiCharacter& target = at(name);
    std::array<float, 3> where{};
    if (!target.position && locate(name, where)) target.position = where;
    AiMoveOrder order;
    order.waypoints = std::move(waypoints);
    order.run = run;
    order.noAvoid = noAvoid;
    order.path = path;
    target.move = std::move(order);
    target.state = path ? AiState::FollowPath : AiState::MoveTo;
    if (!path && navigation_ && target.position && target.move->waypoints.size() == 1) {
        const std::array<float, 3> goal = target.move->waypoints.front();
        if (!startNavMove(name, target, *target.position, goal, noAvoid)) {
            target.move.reset();
            target.state = AiState::Idle;
        }
    }
}

bool AiCore::startNavMove(const std::string& name, AiCharacter& target, const std::array<float, 3>& from, const std::array<float, 3>& to,
                          bool noAvoid) {
    for (std::size_t index = 0; index < navigation_->size(); ++index) {
        auto record = std::make_shared<NavPathRecord>();
        record->avoidFlags = noAvoid ? 1 : 0;
        record->avoidMask = record->gateMask = kAiNavGateMask;
        record->heightTolerance = kAiNavHeightTolerance;
        const int code = navFindPath((*navigation_)[index], *record, from.data(), to.data());
        if (record->startGrid == kNavNone && record->goalGrid == kNavNone) continue;
        if (code == 1) {
            if (std::find(cannotGetThere_.begin(), cannotGetThere_.end(), name) == cannotGetThere_.end()) cannotGetThere_.push_back(name);
            return false;
        }
        if (code == 0 || code == 3) {
            target.move->nav = std::move(record);
            target.move->navWorld = index;
        }
        return true;
    }
    return true;
}

void AiCore::startSimplePath(const std::string& name, SimplePathFollow follow, bool run) {
    AiCharacter& target = at(name);
    std::array<float, 3> where{};
    if (!target.position && locate(name, where)) target.position = where;
    AiMoveOrder order;
    order.run = run;
    order.path = true;
    order.follow = std::move(follow);
    target.move = std::move(order);
    target.state = AiState::FollowPath;
}

void AiCore::run(AiNativeKind kind, const std::vector<std::string>& names, const std::vector<float>& numbers) {
    if (names.empty()) return;
    const std::string& name = names[0];
    const float number = numbers.empty() ? 0.0f : numbers[0];
    AiCharacter& target = at(name);
    std::array<float, 3> where{};
    switch (kind) {
    case AiNativeKind::AIEnable:
        target.enabled = number != 0.0f;
        if (locate(name, where)) target.homePosition = where;
        target.state = AiState::NoState;
        target.move.reset();
        break;
    case AiNativeKind::AINoState:
        target.state = AiState::NoState;
        target.move.reset();
        break;
    case AiNativeKind::AIActive:
        if (target.millAbout.value_or(false)) requestMillAbout(target);
        else requestIdle(target);
        break;
    case AiNativeKind::AIMillAbout:
        target.millAbout = number > 0.0f;
        if (*target.millAbout) requestMillAbout(target);
        else requestIdle(target);
        break;
    case AiNativeKind::AIIgnorePlayer: target.ignorePlayer = number != 0.0f; break;
    case AiNativeKind::CharacterInvulnerable: target.invulnerable = (static_cast<std::int32_t>(number) & 1) != 0; break;
    case AiNativeKind::AICantAttack: target.canAttack = number == 0.0f; break;
    case AiNativeKind::AIMove:
    case AiNativeKind::AIMoveNoAvoid:
        if (names.size() >= 2 && locate(names[1], where)) startMove(name, {where}, number != 0.0f, kind == AiNativeKind::AIMoveNoAvoid, false);
        break;
    case AiNativeKind::AIFaceTarget: {
        std::array<float, 3> self{};
        const bool facing = names.size() >= 2 && locate(names[1], where);
        if (!facing) break;
        if (target.enabled.value_or(false) || name != kPlayerAlias) {
            target.state = AiState::FaceTarget;
            target.faceTarget = names[1];
            if (locate(name, self)) {
                target.homePosition = self;
                if (!target.position) target.position = self;
            }
        } else if (locate(name, self)) {
            target.heading = headingToward(self, where);
        }
        break;
    }
    case AiNativeKind::AIResetState:
        requestIdle(target);
        ++target.stateResets;
        break;
    case AiNativeKind::AISuperJump:
        if (names.size() >= 2 && locate(names[1], where)) {
            std::array<float, 3> self{};
            if (target.position) self = *target.position;
            else if (!locate(name, self)) break;
            target.position = self;
            AiSuperJump jump;
            jump.target = where;
            jump.height = number;
            jump.plan = planSuperJump(self, where, kSuperJumpNativeFrames, number);
            jump.start = self;
            jump.horizontalDistance = std::sqrt((where[0] - self[0]) * (where[0] - self[0]) + (where[2] - self[2]) * (where[2] - self[2]));
            target.heading = jump.plan.heading;
            target.move.reset();
            target.superJump = jump;
            target.state = AiState::GoToTarget;
        }
        break;
    case AiNativeKind::AIPursuit:
        if (names.size() >= 2) {
            target.pursuit = AiPursuit{names[1], number != 0.0f};
            target.state = number != 0.0f ? AiState::Takedown : AiState::Pursuit;
        }
        break;
    case AiNativeKind::AIStalkPlayer:
        target.stalkDistance = number;
        target.state = AiState::Stalk;
        target.move.reset();
        break;
    case AiNativeKind::AIRunAway:
        target.runningAway = true;
        target.state = AiState::RunAway;
        target.move.reset();
        break;
    case AiNativeKind::AIFollowSimplePath: {
        SimplePathFollow follow;
        if (names.size() < 2 || !pathLookup_ || !pathLookup_(names[1], follow) || follow.nodes.empty()) break;
        const std::int32_t count = static_cast<std::int32_t>(follow.nodes.size());
        std::int32_t first = numbers.size() > 0 ? static_cast<std::int32_t>(numbers[0]) : 0;
        if (numbers.size() > 0 && numbers[0] < 0.0f) first += count - 1;
        follow.index = static_cast<std::uint16_t>(std::clamp<std::int32_t>(first, 0, count - 1));
        follow.reverse = numbers.size() > 1 && numbers[1] != 0.0f;
        startSimplePath(name, std::move(follow), numbers.size() > 2 && numbers[2] != 0.0f);
        break;
    }
    }
}

void AiCore::advanceStraightMove(const std::string& name, AiCharacter& target, float dt) {
    AiMoveOrder& order = *target.move;
    float budget = (order.run ? kAiRunSpeed : kAiWalkSpeed) * dt;
    std::array<float, 3>& position = *target.position;
    while (budget > 0.0f && !order.waypoints.empty()) {
        const std::array<float, 3>& goal = order.waypoints.front();
        const float dx = goal[0] - position[0], dz = goal[2] - position[2];
        const float distance = std::sqrt(dx * dx + dz * dz);
        target.heading = headingToward(position, goal);
        if (distance <= budget) {
            position = goal;
            budget -= distance;
            order.waypoints.erase(order.waypoints.begin());
        } else {
            position[0] += dx / distance * budget;
            position[2] += dz / distance * budget;
            budget = 0.0f;
        }
    }
    if (order.waypoints.empty()) {
        target.move.reset();
        target.state = AiState::Idle;
        ended_.push_back({name, 0});
    }
}

void AiCore::advanceNavMove(const std::string& name, AiCharacter& target, float dt) {
    AiMoveOrder& order = *target.move;
    std::array<float, 3>& position = *target.position;
    const NavFollowResult result = navFollow((*navigation_)[order.navWorld], *order.nav, position.data());
    if (result.code == 0) {
        target.move.reset();
        target.state = AiState::Idle;
        ended_.push_back({name, 0});
        return;
    }
    if (result.code == 1 || result.code == 2) {
        order.nav.reset();
        return;
    }
    if (result.code > 5) {
        order.nav.reset();
        return;
    }
    if (result.code != 4 && result.code != 5) return;
    const float dx = result.target[0] - position[0], dz = result.target[2] - position[2];
    const float distance = std::sqrt(dx * dx + dz * dz);
    target.heading = headingToward(position, result.target);
    if (!order.navHeightOffset) order.navHeightOffset = position[1] - result.target[1];
    position[1] = result.target[1] + *order.navHeightOffset;
    if (distance <= 0.0f) return;
    const float step = std::min(distance, (order.run ? kAiRunSpeed : kAiWalkSpeed) * dt);
    position[0] += dx / distance * step;
    position[2] += dz / distance * step;
}

namespace {
// STAND-IN SCRIPT-S103: the height follows a parabola through the apex `plan.climb` above the start to the target height by the fraction of the
// horizontal distance flown; a released flight goes on with the fraction above 1 (STAND-IN SCRIPT-S318).
float superJumpHeight(const AiSuperJump& jump, float progress) {
    const float rise = jump.target[1] - jump.start[1];
    const float climb = jump.plan.climb;
    if (climb > 0.0f && climb >= rise) {
        const float apexToLanding = std::sqrt((climb - rise) / climb);
        const float apex = 1.0f / (1.0f + apexToLanding);
        const float k = climb / (apex * apex);
        return jump.start[1] + climb - k * (progress - apex) * (progress - apex);
    }
    return jump.start[1] + rise * progress;
}
}

void AiCore::advanceSuperJump(const std::string& name, AiCharacter& target, float dt) {
    AiSuperJump& jump = *target.superJump;
    std::array<float, 3>& position = *target.position;
    GoToTarget order;
    order.target = jump.target;
    order.minDistance = kAiGoToMinDistance;
    std::uint16_t heading = 0;
    if (stepGoToTarget(order, position, heading) == GoToTargetResult::Arrived) {
        target.state = AiState::Idle;
        target.superJump.reset();
        ended_.push_back({name, 0});
        return;
    }
    target.heading = heading;
    SuperJumpStep step = stepSuperJumpHorizontal(jump.plan, heading, dt);
    const float length = std::sqrt(step.x * step.x + step.z * step.z);
    const float remaining = std::sqrt((jump.target[0] - position[0]) * (jump.target[0] - position[0]) + (jump.target[2] - position[2]) * (jump.target[2] - position[2]));
    // STAND-IN SCRIPT-S103: the original ends the flight by its landing (0x28e4cc); without that model a step longer than the distance left is
    // cut at the target, so state 11 arrives in the next update instead of passing the target again and again.
    if (length > remaining && length > 0.0f) {
        step.x *= remaining / length;
        step.z *= remaining / length;
    }
    position[0] += step.x;
    position[2] += step.z;
    jump.traveled += std::min(length, remaining);
    position[1] = superJumpHeight(jump, jump.horizontalDistance > 0.0f ? std::min(1.0f, jump.traveled / jump.horizontalDistance) : 1.0f);
}

bool AiCore::advanceReleasedFlight(AiCharacter& target, float dt) {
    AiSuperJump& jump = *target.superJump;
    jump.released = true;
    if (jump.horizontalDistance <= 0.0f) return false;
    const SuperJumpStep step = stepSuperJumpHorizontal(jump.plan, jump.plan.heading, dt);
    jump.traveled += std::sqrt(step.x * step.x + step.z * step.z);
    (*target.position)[1] = superJumpHeight(jump, jump.traveled / jump.horizontalDistance);
    return true;
}

void AiCore::land(const std::string& name, float height) {
    AiCharacter& target = at(name);
    if (!target.superJump || !target.superJump->released || !target.position) return;
    (*target.position)[1] = height;
    target.superJump.reset();
}

void AiCore::advanceFaceTarget(const std::string&, AiCharacter& target) {
    std::array<float, 3> where{};
    const bool located = !target.faceTarget.empty() && locate(target.faceTarget, where);
    const FaceTargetResult result = stepFaceTarget(*target.position, target.homePosition.value_or(*target.position), located ? &where : nullptr,
                                                   target.heading.value_or(0));
    *target.position = result.position;
    if (result.facing) target.heading = result.heading;
}

void AiCore::advanceSimplePath(const std::string& name, AiCharacter& target, float dt) {
    AiMoveOrder& order = *target.move;
    std::array<float, 3>& position = *target.position;
    std::uint16_t heading = 0;
    const bool nodeInRange = order.follow->index < order.follow->nodes.size();
    const SimplePathResult result = stepSimplePath(*order.follow, position, heading);
    if (nodeInRange) target.heading = heading;
    if (result == SimplePathResult::Ended) {
        target.move.reset();
        target.state = AiState::Idle;
        ended_.push_back({name, kSimplePathEndEventArgument});
        return;
    }
    if (result == SimplePathResult::Advanced) return;
    const std::array<float, 3>& node = order.follow->nodes[order.follow->index];
    const float dx = node[0] - position[0], dz = node[2] - position[2];
    const float distance = std::sqrt(dx * dx + dz * dz);
    const float step = std::min(distance, (order.run ? kAiRunSpeed : kAiWalkSpeed) * dt);
    position[0] += dx / distance * step;
    position[2] += dz / distance * step;
}

void AiCore::update(float dt) {
    moved_.clear();
    navClock_ += dt;
    if (navigation_)
        for (NavWorld& world : *navigation_) world.now = navClock_;
    for (auto& [name, target] : characters_) {
        if (!target.position) continue;
        if (updateGate_ && !updateGate_(name)) continue;
        if (target.state == AiState::GoToTarget && target.superJump) {
            moved_.push_back(name);
            advanceSuperJump(name, target, dt);
            continue;
        }
        if (target.superJump && advanceReleasedFlight(target, dt)) moved_.push_back(name);
        if (target.state == AiState::FaceTarget) {
            advanceFaceTarget(name, target);
            continue;
        }
        if (!target.move) continue;
        if (target.state != AiState::MoveTo && target.state != AiState::FollowPath) continue;
        moved_.push_back(name);
        if (target.move->follow) advanceSimplePath(name, target, dt);
        else if (target.move->nav) advanceNavMove(name, target, dt);
        else advanceStraightMove(name, target, dt);
    }
}

std::vector<AiMoveEnd> AiCore::takeMovesEnded() {
    std::vector<AiMoveEnd> taken;
    taken.swap(ended_);
    return taken;
}

}

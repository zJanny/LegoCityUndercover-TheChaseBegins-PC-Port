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
            if (locate(name, self)) target.homePosition = self;
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
        if (names.size() >= 2 && locate(names[1], where)) target.superJump = AiSuperJump{where, number};
        break;
    case AiNativeKind::AIPursuit:
        if (names.size() >= 2) {
            target.pursuit = AiPursuit{names[1], number != 0.0f};
            target.state = number != 0.0f ? AiState::Takedown : AiState::Pursuit;
        }
        break;
    case AiNativeKind::AIStalkPlayer: target.stalkDistance = number; break;
    case AiNativeKind::AIRunAway: target.runningAway = true; break;
    case AiNativeKind::AIFollowSimplePath: {
        const std::vector<std::array<float, 3>>* points = names.size() >= 2 && pathLookup_ ? pathLookup_(names[1]) : nullptr;
        if (!points || points->empty()) break;
        const std::int32_t count = static_cast<std::int32_t>(points->size());
        std::int32_t first = numbers.size() > 0 ? static_cast<std::int32_t>(numbers[0]) : 0;
        if (numbers.size() > 0 && numbers[0] < 0.0f) first += count - 1;
        first = std::clamp<std::int32_t>(first, 0, count - 1);
        const bool reverse = numbers.size() > 1 && numbers[1] != 0.0f;
        const bool running = numbers.size() > 2 && numbers[2] != 0.0f;
        std::vector<std::array<float, 3>> waypoints;
        for (std::int32_t i = first; reverse ? i >= 0 : i < count; i += reverse ? -1 : 1) waypoints.push_back((*points)[std::size_t(i)]);
        startMove(name, std::move(waypoints), running, false, true);
        break;
    }
    }
}

void AiCore::update(float dt) {
    moved_.clear();
    for (auto& [name, target] : characters_) {
        if (!target.move || !target.position) continue;
        if (target.state != AiState::MoveTo && target.state != AiState::FollowPath) continue;
        AiMoveOrder& order = *target.move;
        moved_.push_back(name);
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
            ended_.push_back(name);
        }
    }
}

std::vector<std::string> AiCore::takeMovesEnded() {
    std::vector<std::string> taken;
    taken.swap(ended_);
    return taken;
}

}

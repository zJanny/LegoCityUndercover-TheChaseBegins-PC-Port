#include "runtime/ai_patrol_walk.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {

AiFollowOutcome aiFollowOutcome(int code) {
    const int low = code & 0xff;
    if (low == 0) return AiFollowOutcome::Arrived;
    if (low == 1 || low == 2) return AiFollowOutcome::Blocked;
    return AiFollowOutcome::Moving;
}

AiPatrolEnterStep aiPatrolEnter(const AiPatrolEnterInput& input) {
    AiPatrolEnterStep step;
    step.patrolObject = input.patrolObject;
    step.walk = input.walk;
    if (input.patrolObject == 0) {
        step.patrolObject = input.pathParameter;
        if (step.patrolObject != 0) {
            step.patrolling = true;
            step.walk = true;
        } else if (input.returnToSpawnPoint) {
            step.walk = input.dontRun;
        }
    }
    if (step.patrolObject != 0) step.source = AiPatrolSource::PatrolObject;
    else if (input.returnToSpawnPoint) step.source = AiPatrolSource::SpawnPoint;
    return step;
}

AiPatrolArrival aiPatrolArrival(std::uint32_t patrolObject, bool returnToSpawnPoint, std::uint32_t nextNode) {
    AiPatrolArrival arrival;
    if (patrolObject == 0 && returnToSpawnPoint) {
        arrival.idle = true;
        return arrival;
    }
    arrival.patrolObject = patrolObject == 0 ? 0 : nextNode;
    return arrival;
}

bool aiPatrolBlockedStep(std::uint32_t& countdown, std::uint32_t random) {
    if (std::int32_t(countdown) > 0) {
        --countdown;
        return false;
    }
    countdown = random % kAiPatrolRetrySpan + kAiPatrolRetryBase;
    return true;
}

float aiWatchAngle(float degrees) { return degrees * kAiPiSingle * kAiRadiansPerDegreeStep; }

bool aiAngleTest(const std::array<float, 3>& viewer, const std::array<float, 3>& forward, const std::array<float, 3>& target, float angle) {
    const float v[3] = {target[0] - viewer[0], target[1] - viewer[1], target[2] - viewer[2]};
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    std::uint32_t bits;
    std::memcpy(&bits, &length, 4);
    float direction[3] = {0.0f, 0.0f, 1.0f};
    if (bits > kAiAngleTinyLengthBits) {
        const float scale = 1.0f / length;
        for (int i = 0; i < 3; ++i) direction[i] = v[i] * scale;
    }
    const float dot = direction[0] * forward[0] + direction[1] * forward[1] + direction[2] * forward[2];
    return dot > std::cos(angle);
}

}

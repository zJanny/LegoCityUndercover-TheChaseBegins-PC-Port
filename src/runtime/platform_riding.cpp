#include "runtime/platform_riding.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::int32_t kSpeedMinimumBits = 0x34000000;
constexpr std::uint8_t kLastResetToStoppedType = 6;

}

void setPlatformSpeed(PlatformRecord& record, float speed) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &speed, sizeof bits);
    record.speed = bits < kSpeedMinimumBits ? kPlatformSpeedMinimum : speed;
}

void setPlatformOrigin(PlatformRecord& record, const PlatformVector& translation) { record.motion.origin = translation; }

void setPlatformTarget(PlatformRecord& record, const PlatformVector& translation) { record.motion.target = translation; }

void movePlatform(PlatformRecord& record, float progress, bool backwards) { platformRequest(record, backwards, progress); }

PlatformVector platformReset(PlatformRecord& record) {
    if (record.motion.type == 0) record.requested = kPlatformMovingState;
    else if (record.motion.type <= kLastResetToStoppedType) record.requested = kPlatformStoppedState;
    record.value = 0.0f;
    return record.motion.origin;
}

void groundEntityContact(GroundEntityLink& link, const std::string& lowerPlatform, const PlatformVector& translation) {
    link.owner = lowerPlatform;
    link.oriented = false;
    link.remembered = translation;
}

namespace {
PlatformVector intoFrame(const PathRotation& rows, const PlatformVector& v) {
    PlatformVector out{};
    for (std::size_t k = 0; k < 3; ++k) out[k] = v[0] * rows[3 * k] + v[1] * rows[3 * k + 1] + v[2] * rows[3 * k + 2];
    return out;
}
}

void groundEntityContactOriented(GroundEntityLink& link, const std::string& lowerPlatform, const PlatformFrame& frame, const PlatformVector& position,
                                 const PlatformVector& forward) {
    link.owner = lowerPlatform;
    link.oriented = true;
    link.remembered = intoFrame(frame.rows, {position[0] - frame.translation[0], position[1] - frame.translation[1], position[2] - frame.translation[2]});
    link.localForward = intoFrame(frame.rows, forward);
}

GroundEntityTurn groundEntityRideOriented(const GroundEntityLink& link, const PlatformFrame& frame, const PlatformVector& position, const PlatformVector& forward) {
    GroundEntityTurn ride;
    if (link.owner.empty() || !link.oriented) return ride;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const float world = link.remembered[0] * frame.rows[axis] + link.remembered[1] * frame.rows[3 + axis] + link.remembered[2] * frame.rows[6 + axis] +
                            frame.translation[axis];
        ride.move[axis] = world - position[axis];
    }
    ride.move[1] = 0.0f;
    const PlatformVector now = intoFrame(frame.rows, forward);
    const float turn = (std::atan2(link.localForward[0], link.localForward[2]) - std::atan2(now[0], now[2])) * kRadiansToHeading;
    ride.turn = std::int16_t(std::int32_t(turn));
    return ride;
}

PlatformVector groundEntityRide(const GroundEntityLink& link, const PlatformVector& now) {
    if (link.owner.empty() || link.oriented) return {};
    const float dx = link.remembered[0] - now[0];
    const float dy = link.remembered[1] - now[1];
    const float dz = link.remembered[2] - now[2];
    float squared = dx * dx;
    squared += dy * dy;
    squared += dz * dz;
    if (!(squared > 0.0f) || !(kGroundEntityRideLimitSquared > squared)) return {};
    return {now[0] - link.remembered[0], now[1] - link.remembered[1], now[2] - link.remembered[2]};
}

}

#include "runtime/attack_facts.hpp"
#include "runtime/camera_reset_probe.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>

namespace chase::runtime {
namespace {
using Vec3 = std::array<float, 3>;

Vec3 transformPoint(const assets::Matrix& m, const Vec3& p) {
    return {p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12], p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
            p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]};
}

Vec3 toBoxSpace(const assets::Matrix& m, const Vec3& p) {
    const float d[3] = {p[0] - m[12], p[1] - m[13], p[2] - m[14]};
    Vec3 out{};
    for (std::size_t i = 0; i < 3; ++i) out[i] = d[0] * m[i * 4] + d[1] * m[i * 4 + 1] + d[2] * m[i * 4 + 2];
    return out;
}

float distance(const Vec3& a, const Vec3& b) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool ownerAllowsQuery(const AttackCandidate& candidate) {
    return candidate.collision && (candidate.entity.flags & kEntityFlagCollision) && !(candidate.entity.flags & kEntityFlagsOwnerExcluded);
}

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

bool attackLineClear(const CollisionWorld& world, const AttackBody& attacker, const std::vector<AttackCandidate>& candidates, std::size_t target) {
    if (target >= candidates.size()) return true;
    const Vec3 from = transformPoint(attacker.node, attacker.centre);
    const AttackBody& targetBody = candidates[target].body;
    const Vec3 to = transformPoint(targetBody.node, targetBody.centre);
    const float length = distance(from, to);

    float nearestOther = FLT_MAX;
    const CameraResetProbe probe(world, std::string(), kAttackLineLayer, kAttackLineExcludedSurfaces);
    if (const auto hit = probe.nearestHit(from.data(), to.data())) nearestOther = distance(from, *hit);

    float targetDistance = FLT_MAX;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const AttackCandidate& candidate = candidates[i];
        if (!isAttackCharacter(candidate.entity) || !ownerAllowsQuery(candidate)) continue;
        const AttackBody& body = candidate.body;
        const Vec3 start = toBoxSpace(body.node, from), end = toBoxSpace(body.node, to);
        float fraction = 0;
        if (!segmentMeetsBox(start.data(), end.data(), body.centre.data(), body.half.data(), fraction)) continue;
        const float entry = fraction * length;
        if (i == target) targetDistance = entry;
        else nearestOther = std::min(nearestOther, entry);
    }
    if (nearestOther == FLT_MAX) return true;
    return targetDistance <= nearestOther;
}

void markAttackNearList(std::vector<AttackCandidate>& candidates, const Vec3& playerNode) {
    std::size_t listed = 0;
    for (AttackCandidate& candidate : candidates) {
        candidate.listed = false;
        if (listed >= kAttackNearListMax || !ownerAllowsQuery(candidate)) continue;
        const AttackBody& body = candidate.body;
        Vec3 low{FLT_MAX, FLT_MAX, FLT_MAX}, high{-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (int corner = 0; corner < 8; ++corner) {
            const Vec3 local{body.centre[0] + ((corner & 1) ? body.half[0] : -body.half[0]), body.centre[1] + ((corner & 2) ? body.half[1] : -body.half[1]),
                             body.centre[2] + ((corner & 4) ? body.half[2] : -body.half[2])};
            const Vec3 world = transformPoint(body.node, local);
            for (std::size_t a = 0; a < 3; ++a) {
                low[a] = std::min(low[a], world[a]);
                high[a] = std::max(high[a], world[a]);
            }
        }
        bool meets = true;
        for (std::size_t a = 0; a < 3; ++a)
            if (high[a] < playerNode[a] - kAttackNearRange || low[a] > playerNode[a] + kAttackNearRange) meets = false;
        candidate.listed = meets;
        if (meets) ++listed;
    }
}

EntityBox characterEntityBox(const assets::Level& classFile, bool boundCollision) {
    EntityBox box;
    for (const assets::LevelBound& bound : classFile.bounds) {
        if (lowerCase(bound.name) != "collisionbound") continue;
        for (std::size_t a = 0; a < 3; ++a) {
            box.centre[a] = bound.centre[a];
            box.half[a] = bound.half[a];
        }
        box.collision = boundCollision;
        break;
    }
    return box;
}

}

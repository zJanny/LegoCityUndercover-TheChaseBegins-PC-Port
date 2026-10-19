#include "runtime/use_marker.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {

float distance(const float a[3], const float b[3]) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}

float UseMarkers::Slot::value() const {
    switch (state) {
    case FadeState::Hidden: return 0.0f;
    case FadeState::FadingIn: return std::clamp(elapsed / kUseMarkerFadeInSeconds, 0.0f, 1.0f);
    case FadeState::Shown: return 1.0f;
    case FadeState::FadingOut: return std::clamp(1.0f - elapsed / kUseMarkerFadeOutSeconds, 0.0f, 1.0f);
    }
    return 0.0f;
}

void UseMarkers::show(Slot& slot) {
    switch (slot.state) {
    case FadeState::Hidden:
        slot.state = FadeState::FadingIn;
        slot.elapsed = 0;
        break;
    case FadeState::FadingIn: break;
    case FadeState::Shown: slot.elapsed = 0; break;
    case FadeState::FadingOut: {
        const float shown = slot.value();
        slot.state = FadeState::FadingIn;
        slot.elapsed = shown * kUseMarkerFadeInSeconds;
        break;
    }
    }
    slot.hold = kUseMarkerHoldSeconds;
}

// STAND-IN SCRIPT-S248: the fader manager that counts the hold (slot +0x1c) down and starts the fade out (state 3) was not
// found; the hold counts down by the update time and the fade out starts when it runs out.
void UseMarkers::step(Slot& slot, float dt) {
    slot.elapsed += dt;
    if (slot.state == FadeState::FadingIn && slot.elapsed >= kUseMarkerFadeInSeconds) slot.state = FadeState::Shown;
    if (slot.state == FadeState::FadingIn || slot.state == FadeState::Shown) {
        slot.hold -= dt;
        if (slot.hold <= 0) {
            const float shown = slot.value();
            slot.state = FadeState::FadingOut;
            slot.elapsed = (1.0f - shown) * kUseMarkerFadeOutSeconds;
        }
    }
    if (slot.state == FadeState::FadingOut && int(slot.value() * 255.0f) < kUseMarkerReleaseAlpha) slot = Slot{};
}

void UseMarkers::update(const std::vector<UseMarkerCandidate>& candidates, const float camera[3], const float player[3], float dt) {
    clock_ += dt * kUseMarkerClockUnitsPerSecondStandIn;
    for (Slot& slot : slots_)
        if (slot.used) step(slot, dt);
    for (const UseMarkerCandidate& candidate : candidates) {
        if (!candidate.usable) continue;
        const float position[3] = {candidate.matrix[12], candidate.matrix[13], candidate.matrix[14]};
        if (!(distance(camera, position) < kUseMarkerRange) && !(distance(player, position) < kUseMarkerRange)) continue;
        auto slot = std::find_if(slots_.begin(), slots_.end(), [&](const Slot& s) { return s.used && s.marker.instance == candidate.instance; });
        if (slot == slots_.end()) slot = std::find_if(slots_.begin(), slots_.end(), [](const Slot& s) { return !s.used; });
        if (slot == slots_.end()) continue;
        slot->used = true;
        slot->marker = candidate;
        show(*slot);
    }
}

std::vector<UseMarkerQuad> UseMarkers::quads() const {
    std::vector<UseMarkerQuad> quads;
    const float angle = clock_ / kUseMarkerSpinDivisor, c = std::cos(angle), s = std::sin(angle);
    for (const Slot& slot : slots_) {
        if (!slot.used) continue;
        const std::array<float, 16>& m = slot.marker.matrix;
        float right[3], forward[3];
        for (int a = 0; a < 3; ++a) {
            right[a] = c * m[a] - s * m[8 + a];
            forward[a] = s * m[a] + c * m[8 + a];
        }
        UseMarkerQuad quad;
        quad.texture = slot.marker.texture;
        quad.alpha = slot.value();
        const float corners[4][4] = {{-kUseMarkerHalfSize, -kUseMarkerHalfSize, 1, 0},
                                     {kUseMarkerHalfSize, -kUseMarkerHalfSize, 0, 0},
                                     {kUseMarkerHalfSize, kUseMarkerHalfSize, 0, 1},
                                     {-kUseMarkerHalfSize, kUseMarkerHalfSize, 1, 1}};
        for (int v = 0; v < 4; ++v) {
            for (int a = 0; a < 3; ++a)
                quad.vertices[v].position[a] = slot.marker.ground[a] + corners[v][0] * right[a] + kUseMarkerLift * m[4 + a] + corners[v][1] * forward[a];
            quad.vertices[v].uv[0] = corners[v][2];
            quad.vertices[v].uv[1] = corners[v][3];
        }
        quads.push_back(quad);
    }
    return quads;
}

float UseMarkers::alpha(const std::string& instance) const {
    for (const Slot& slot : slots_)
        if (slot.used && slot.marker.instance == instance) return slot.value();
    return 0.0f;
}

bool UseMarkers::slotted(const std::string& instance) const {
    for (const Slot& slot : slots_)
        if (slot.used && slot.marker.instance == instance) return true;
    return false;
}

}

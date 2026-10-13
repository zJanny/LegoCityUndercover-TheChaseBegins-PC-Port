#include "runtime/sound_events.hpp"

namespace chase::runtime {

float footstepTrackValue(const assets::Animation& animation, float normalisedTime) {
    for (const auto& sub : animation.subAnimations)
        for (const assets::AnimTrack& track : sub)
            if (track.hash == kFootstepTrackHash) {
                float value = 0.0f;
                for (const assets::CurveKey& key : track.keys)
                    if (key.time <= normalisedTime) value = key.value;
                return value;
            }
    return 0.0f;
}

bool FootstepDetector::update(float value, float weight) {
    const bool rose = value * weight - previousValue_ * previousWeight_ > kFootstepRise;
    previousValue_ = value;
    previousWeight_ = weight;
    return rose;
}

void FootstepDetector::restart(float value, float weight) {
    previousValue_ = value;
    previousWeight_ = weight;
}

std::vector<SoundCall> CharacterSounds::landing(float fallDistance, bool water, bool objectType11,
                                                const std::array<float, 3>& position) const {
    std::vector<SoundCall> calls;
    if (fallDistance > kLandingImpactDistance) calls.push_back({id(kSlotLandFromHeight), position});
    calls.push_back({id(water ? kSlotLandWater : (objectType11 ? kSlotLandOnObject11 : kSlotFootstep)), position});
    return calls;
}

SoundCall studCollectCall(const StudCollected& collected, const std::array<float, 3>& centre) {
    return {collected.sound, centre};
}

} // namespace chase::runtime

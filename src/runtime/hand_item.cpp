#include "runtime/hand_item.hpp"

namespace chase::runtime {

namespace {
constexpr std::uint32_t kObjectNameOffset = 0, kBoneNameOffset = 4;

std::optional<std::size_t> trackSub(const assets::Animation& animation) {
    for (std::size_t sub = 0; sub < animation.subAnimations.size(); ++sub)
        for (const assets::AnimTrack& track : animation.subAnimations[sub])
            if (track.hash == kHandItemTrackHash) return sub;
    return std::nullopt;
}
}

std::optional<HandItem> handItem(const assets::Animation& animation) {
    const std::optional<std::size_t> sub = trackSub(animation);
    if (!sub || *sub >= animation.subAnimationTargets.size()) return std::nullopt;
    HandItem item;
    bool object = false, bone = false;
    for (const assets::AnimationTarget& target : animation.subAnimationTargets[*sub]) {
        if (target.offset == kObjectNameOffset) item.object = target.text, object = true;
        if (target.offset == kBoneNameOffset) item.bone = target.text, bone = true;
    }
    if (!object || !bone) return std::nullopt;
    return item;
}

float handItemTrackValue(const assets::Animation& animation, float normalisedTime) {
    for (const auto& sub : animation.subAnimations)
        for (const assets::AnimTrack& track : sub)
            if (track.hash == kHandItemTrackHash) {
                float value = 0.0f;
                for (const assets::CurveKey& key : track.keys)
                    if (key.time <= normalisedTime) value = key.value;
                return value;
            }
    return 0.0f;
}

std::optional<HandItemEvent> HandItemTrack::update(const assets::Animation& animation, float normalisedTime, bool restarted) {
    const std::optional<HandItem> item = handItem(animation);
    const float value = item ? handItemTrackValue(animation, normalisedTime) : 0.0f;
    const float change = value - previous_;
    previous_ = value;
    if (!item || restarted || change == 0.0f) return std::nullopt;
    return HandItemEvent{change > 0.0f, *item};
}

}

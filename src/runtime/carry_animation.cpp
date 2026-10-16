#include "runtime/carry_animation.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::runtime {
namespace {
constexpr int kNoClip = 0x271;
constexpr std::size_t kNamesSheet = 1, kRolesSheet = 2;
constexpr int kVariantSmall = 3, kVariantBig = 5;
constexpr float kRiseThreshold = 0.5f;
constexpr float kUpdatesPerSecond = 30.0f;

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

CarryAnimations::CarryAnimations(const assets::Table& animations) {
    if (animations.sheets.size() <= kRolesSheet) return;
    const assets::TableSheet& names = animations.sheets[kNamesSheet];
    for (unsigned row = 0; row < names.rows; ++row) names_.push_back(assets::tableValue(names, row, 1).text);
    const assets::TableSheet& roles = animations.sheets[kRolesSheet];
    for (unsigned row = 0; row < roles.rows; ++row) {
        std::array<int, 7> record{};
        for (std::size_t column = 0; column < 7; ++column) record[column] = int(std::int16_t(assets::tableValue(roles, row, column + 1).bits));
        roles_.push_back(record);
    }
}

int CarryAnimations::clip(int id, int carryingMode) const {
    if (carryingMode == 0) return id;
    for (const std::array<int, 7>& role : roles_) {
        if (role[0] != id) continue;
        const int variant = role[std::size_t(carryingMode == 2 ? kVariantBig : kVariantSmall)];
        return variant == kNoClip ? id : variant;
    }
    return id;
}

std::string CarryAnimations::name(int id) const {
    return id >= 0 && std::size_t(id) < names_.size() ? names_[std::size_t(id)] : std::string();
}

std::vector<int> CarryAnimations::carriedClips() const {
    std::vector<int> clips;
    for (const std::array<int, 7>& role : roles_)
        for (const int variant : {role[std::size_t(kVariantSmall)], role[std::size_t(kVariantBig)]})
            if (variant != kNoClip) clips.push_back(variant);
    return clips;
}

int carryEventUpdate(const assets::Animation& animation, std::uint32_t hash) {
    const float seconds = assets::animationDuration(animation);
    for (const auto& sub : animation.subAnimations)
        for (const assets::AnimTrack& track : sub) {
            if (track.hash != hash) continue;
            float previous = 0.0f;
            for (const assets::CurveKey& key : track.keys) {
                if (key.value - previous > kRiseThreshold) return int(std::lround(key.time * seconds * kUpdatesPerSecond));
                previous = key.value;
            }
        }
    return -1;
}

CarryTiming carryTiming(const assets::Animation& pickup, const assets::Animation& putdown) {
    CarryTiming timing;
    if (const int attach = carryEventUpdate(pickup, kCarryEventTrackHash); attach >= 0) {
        timing.pickupAttach = unsigned(attach);
        timing.pickupEnd = unsigned(attach) + kCarryPickupEndAfterAttach;
    }
    if (const int release = carryEventUpdate(putdown, kCarryEventTrackHash); release >= 0) timing.placeRelease = unsigned(release);
    return timing;
}

std::array<float, 16> carryHandOffset(const assets::Model& model) {
    std::array<float, 16> offset = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    for (std::size_t node = 0; node < model.nodeNames.size() && node < model.nodeMatrices.size(); ++node) {
        if (lowerCase(model.nodeNames[node]) != kCarryHandleNode) continue;
        const std::array<float, 16>& m = model.nodeMatrices[node];
        for (std::size_t row = 0; row < 3; ++row)
            for (std::size_t column = 0; column < 3; ++column) offset[row * 4 + column] = m[column * 4 + row];
        for (std::size_t column = 0; column < 3; ++column)
            offset[12 + column] = -(m[12] * m[column * 4] + m[13] * m[column * 4 + 1] + m[14] * m[column * 4 + 2]);
        break;
    }
    return offset;
}

}

#include "runtime/ai_start_state.hpp"

#include "runtime/ai.hpp"
#include "runtime/level_start.hpp"
#include "runtime/switch_base.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::array<AiStartStateEntry, kAiStartStateEntries> kEntries = {{
    {0x2cc73c, 0x2a07b4, 0x2bae68, 0x2cb968},
    {0x2a8bec, 0x2a07b4, 0x2bae68, 0x2a8b60},
    {0x2a8bec, 0x2a07b4, 0x2bae68, 0x295538},
    {0, 0, 0, 0x2bae6c},
    {0, 0, 0, 0x2b7fe0},
    {0, 0, 0, 0x2aaf40},
    {0x2c0af4, 0x2a07b4, 0x2bae68, 0x2bae74},
    {0, 0, 0, 0x2afc90},
    {0x2b7fd0, 0x2b0dc4, 0x2bae68, 0x2c9ca8},
    {0x2b7fd0, 0x2b0dc4, 0x2bae68, 0x298bd4},
}};

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 0) return fallback;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

std::string stringParam(const assets::LevelObject& object, const assets::Level& cls, const std::string& name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 1 || param->blocks.empty()) return {};
    const assets::Bytes& block = param->blocks[0];
    return lower(std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0))));
}

std::string referenceParam(const assets::LevelObject& object, const assets::Level& cls, const std::string& name,
                           const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

}

std::optional<AiStartStateEntry> aiStartStateEntry(std::uint16_t index) {
    if (index >= kAiStartStateEntries) return std::nullopt;
    return kEntries[index];
}

AiStartTargetKind aiStartTargetKind(std::uint16_t index) {
    const std::optional<AiStartStateEntry> entry = aiStartStateEntry(index);
    if (!entry) return AiStartTargetKind::None;
    if (entry->postLink == 0x2a07b4) return AiStartTargetKind::RunTo;
    if (entry->postLink == 0x2b0dc4) return AiStartTargetKind::FollowPath;
    return AiStartTargetKind::None;
}

const char* aiStartActionName(AiStartAction action) {
    switch (action) {
    case AiStartAction::None: return "none";
    case AiStartAction::Idle: return "idle";
    case AiStartAction::Move: return "move";
    case AiStartAction::RequestState6: return "state 6";
    case AiStartAction::RequestState7: return "state 7";
    case AiStartAction::RequestState8: return "state 8";
    case AiStartAction::MillAbout: return "mill about";
    case AiStartAction::PlaceBelow: return "place below";
    case AiStartAction::PlaceBehind: return "place behind";
    case AiStartAction::SuperJump: return "super jump";
    case AiStartAction::NoState: return "no state";
    case AiStartAction::FollowPath: return "follow path";
    case AiStartAction::PathLocomotion: return "path locomotion";
    }
    return "?";
}

AiStartState aiStartStateOf(std::uint16_t index, const assets::LevelObject& object, const assets::Level& cls,
                            const std::map<std::uint32_t, std::string>& names) {
    AiStartState state;
    state.index = index;
    if (index == 0) state.forceMove = (switchIntParam(object, cls, "ForceMove", 0) & 1) != 0;
    if (index == 6) state.superJumpHeight = floatParam(object, cls, "SuperjumpHeight", kAiStartSuperJumpHeight);
    const AiStartTargetKind kind = aiStartTargetKind(index);
    for (std::size_t i = 0; i < kAiStartStateTargets; ++i) {
        if (kind == AiStartTargetKind::RunTo) state.targets[i] = referenceParam(object, cls, "Runto" + std::to_string(i + 1), names);
        if (kind == AiStartTargetKind::FollowPath) state.targets[i] = stringParam(object, cls, "FollowPath" + std::to_string(i + 1));
    }
    return state;
}

AiStartOrder aiStartOrder(const AiStartState& state, std::size_t slot) {
    AiStartOrder order;
    if (state.index >= kAiStartStateEntries) return order;
    const std::string target = slot < kAiStartStateTargets ? state.targets[slot] : std::string();
    order.target = target;
    switch (state.index) {
    case 0:
        order.action = target.empty() ? AiStartAction::Idle : AiStartAction::Move;
        order.avoid = state.forceMove;
        break;
    case 1: order.action = target.empty() ? AiStartAction::RequestState7 : AiStartAction::RequestState6; break;
    case 2:
        if (!target.empty()) order.action = AiStartAction::RequestState8;
        break;
    case 3: order.action = AiStartAction::MillAbout; break;
    case 4: order.action = AiStartAction::PlaceBelow; break;
    case 5: order.action = AiStartAction::PlaceBehind; break;
    case 6:
        if (target.empty()) break;
        order.action = AiStartAction::SuperJump;
        order.height = state.superJumpHeight;
        order.frames = kAiStartSuperJumpFrames;
        break;
    case 7: order.action = AiStartAction::NoState; break;
    case 8: order.action = AiStartAction::FollowPath; break;
    case 9: order.action = AiStartAction::PathLocomotion; break;
    }
    return order;
}

AiStartApplied applyAiStartOrder(const AiStartOrder& order, const std::string& character, const std::array<float, 3>& position, AiCore& ai) {
    ai.setPosition(character, position.data());
    const auto unresolved = [&] {
        const std::vector<std::string>& names = ai.unresolvedTargets();
        return std::find(names.begin(), names.end(), order.target) != names.end() ? AiStartApplied::TargetNotFound : AiStartApplied::Applied;
    };
    switch (order.action) {
    case AiStartAction::Idle: ai.startIdle(character); return AiStartApplied::Applied;
    case AiStartAction::Move: ai.run(AiNativeKind::AIMove, {character, order.target}, {1.0f}); return unresolved();
    case AiStartAction::MillAbout: ai.startMillAbout(character); return AiStartApplied::Applied;
    case AiStartAction::SuperJump: ai.run(AiNativeKind::AISuperJump, {character, order.target}, {order.height}); return unresolved();
    case AiStartAction::NoState: ai.run(AiNativeKind::AINoState, {character}, {}); return AiStartApplied::Applied;
    default: return AiStartApplied::NotPorted;
    }
}

}

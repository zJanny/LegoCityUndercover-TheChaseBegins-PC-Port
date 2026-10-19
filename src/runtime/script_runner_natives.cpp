#include "runtime/script_runner.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::string withoutQuotes(const std::string& text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') return text.substr(1, text.size() - 2);
    return text;
}

}

void ScriptRunner::destroyNative(const std::string& lowerInstance) { destroyEntity(lowerInstance); }

void ScriptRunner::destroyEntity(const std::string& lowerInstance) {
    if (state_.isDisabled(lowerInstance)) return;
    fireObjectEvent("dead", lowerInstance);
    const std::optional<ObjectNativeState::DestroyResult> result = objectNatives_.destroy(lowerInstance, false, classMessage_);
    if (!result) return;
    if (!result->triggerObject.empty()) triggerObjectEnabling(result->triggerObject);
    state_.destroyed.insert(lowerInstance);
    if (result->reaction == DestroyReaction::HidesEntity) state_.disabled.insert(lowerInstance);
}

void ScriptRunner::effectNative(const assets::ScriptCall& call) {
    const auto kind = effectNativeKind(lowerCase(call.function));
    const auto& args = call.arguments;
    if (!kind || args.size() < effectNativeArguments(*kind)) return;
    switch (*kind) {
    case EffectNativeKind::EffectFadeObject:
        effectNatives_.fadeObject(lowerCase(args[0]), read(args[1]), read(args[2]), read(args[3]), static_cast<std::uint8_t>(static_cast<int>(read(args[4])) & 0xff));
        break;
    case EffectNativeKind::EnableDistanceFade:
        effectNatives_.enableDistanceFade(lowerCase(args[0]), read(args[1]));
        break;
    case EffectNativeKind::AddToSemiTransNearCameraList:
        effectNatives_.addSemiTransNearCamera(lowerCase(withoutQuotes(args[0])), withoutQuotes(args[1]));
        break;
    }
}

ScriptRunner::Result ScriptRunner::worldNative(const assets::ScriptCall& call) {
    const auto kind = worldNativeKind(lowerCase(call.function));
    const auto& args = call.arguments;
    if (!kind || args.size() < worldNativeArguments(*kind)) return Result::Continue;
    switch (*kind) {
    case WorldNativeKind::SetPartyCharacter: {
        const std::string object = lowerCase(args[0]);
        if (object != "player1" && object != "player2") break;
        partyRequests_.push_back({object == "player1" ? 0u : 1u, static_cast<std::uint16_t>(static_cast<std::uint32_t>(read(args[1])) & 0xffff), static_cast<int>(read(args[2])) != 0});
        break;
    }
    case WorldNativeKind::StreamRoomIn: {
        const std::string room = lowerCase(withoutQuotes(args[0]));
        if (!roomKnown(room)) break;
        roomRequests_.push_back(room);
        if (!allRoomsLoaded_ && loadedRooms_.count(room) == 0) return Result::Block;
        break;
    }
    }
    return Result::Continue;
}

bool ScriptRunner::roomKnown(const std::string& room) const { return knownRooms_.count(lowerCase(room)) != 0; }

std::vector<PartyRequest> ScriptRunner::takePartyRequests() {
    std::vector<PartyRequest> taken;
    taken.swap(partyRequests_);
    return taken;
}

std::vector<std::string> ScriptRunner::takeRoomRequests() {
    std::vector<std::string> taken;
    taken.swap(roomRequests_);
    return taken;
}

}

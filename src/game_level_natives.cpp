#include "game_level.hpp"
#include "runtime/natives_objects.hpp"
#include "runtime/scene_start.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::installScriptHooks() {
    levelScripts.setClassMessageHandler([this](const std::string& instance, std::uint8_t message) {
        if (message != runtime::kMessageDestroy) return;
        for (std::size_t i = 0; i < boardingVehicles.size() && i < vehicleDrives.size(); ++i)
            if (game_doors::lowerCase(boardingVehicles[i].instance) == instance) {
                pendingVehicleBreaks.push_back(i);
                return;
            }
        if (!characterDeathMessage(instance)) propDestroyMessage(instance);
    });
}

void LevelSession::applyScriptNatives() {
    for (const runtime::PartyRequest& request : levelScripts.takePartyRequests())
        if (runtime::partyOutcome(game_player::kPlayerCharacterId, request) == runtime::PartyOutcome::NotPorted)
            std::cout << "party character " << request.character << " for Player" << request.slot + 1 << " at update " << updatesForLog
                      << ": swap not ported (stand-in SCRIPT-S234)\n";
    scriptedRooms.clear();
    for (const std::string& room : levelScripts.takeRoomRequests()) scriptedRooms.insert(runtime::roomNumber(room));
    for (const std::size_t vehicle : pendingVehicleBreaks)
        if (!vehicleDrives[vehicle].wreck.active) breakVehicleNode(vehicle);
    pendingVehicleBreaks.clear();
}

void LevelSession::applyObjectFades(std::vector<bool>& drawn) {
    if (!levelScripts.effects().anyFade()) return;
    for (std::size_t i = 0; i < placedObjects.size(); ++i) {
        const auto fade = levelScripts.effects().fade(game_doors::lowerCase(placedObjects[i].instance));
        if (!fade) continue;
        scene.setInstanceFade(placedSceneIndex[i], fade->multiplier);
        for (const auto& [placedIndex, wheelInstance] : wheelSceneIndex)
            if (placedIndex == i) scene.setInstanceFade(wheelInstance, fade->multiplier);
        if (fade->hidden) drawn[i] = false;
    }
}

}

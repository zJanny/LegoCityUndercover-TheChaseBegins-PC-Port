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
    for (const runtime::ScriptParticleRequest& request : levelScripts.takeParticleRequests()) {
        const bool started = request.definition >= 0 && particles.system().start(unsigned(request.definition), {request.position, std::nullopt, 0}) != 0;
        std::cout << "particles " << request.definition << " at " << request.object << (started ? " started" : " refused") << " at update " << updatesForLog << '\n';
    }
    if (const std::optional<std::string> target = levelScripts.vehicleAi().takeTargetHeartsRequest()) showTargetHearts(*target);
    for (const runtime::ScriptDebrisRequest& request : levelScripts.takeDebrisRequests()) {
        if (effects.debris()) effects.debris()->burst(request.burst, debrisLineStandIn(), debrisLineStandIn());
        std::cout << "debris burst at " << request.object << ": " << request.burst.count << " bricks at update " << updatesForLog << '\n';
    }
    const std::uint16_t bulletTimeOwner = levelScripts.soundNatives().ownerOf("player1");
    if (levelScripts.bulletTime().cameraShotRequested()) {
        std::cout << "bullet time camera shot not ported (SCRIPT-437) at update " << updatesForLog << '\n';
        levelScripts.bulletTime().clearCameraShotRequest();
    }
    if (const runtime::BulletTimeModeCall modeCall = levelScripts.bulletTime().takeModeCall(); modeCall != runtime::BulletTimeModeCall::None)
        std::cout << "bullet time camera mode call " << (modeCall == runtime::BulletTimeModeCall::GameplayRecords ? "0x221054" : "0x1fc060")
                  << " not ported (SCRIPT-437) at update " << updatesForLog << '\n';
    for (const runtime::BulletTimeSound& sound : levelScripts.bulletTime().takeSounds()) {
        if (sound.voices != runtime::BulletTimeVoices::None) {
            const bool pause = sound.voices == runtime::BulletTimeVoices::Pause;
            std::cout << "bullet time sound voices " << (pause ? "paused" : "resumed") << " at update " << updatesForLog << '\n';
            if (shared.audio) shared.audio->pauseVoices(pause);
            continue;
        }
        if (sound.stop) {
            std::cout << "bullet time sound " << sound.id << " stopped over " << sound.fadeSeconds << " s at update " << updatesForLog << '\n';
            if (shared.audio) shared.audio->applySound({runtime::SoundRequest::Kind::Stop, sound.id, bulletTimeOwner, false, {}, sound.fadeSeconds});
            continue;
        }
        if (shared.audio && player) shared.audio->play({sound.id, player->position(), bulletTimeOwner});
        if (sound.id == runtime::kBulletTimeSoundLoop && !(shared.audio && shared.audio->playing(sound.id, bulletTimeOwner))) continue;
        std::cout << "bullet time sound " << sound.id << " at update " << updatesForLog << '\n';
        soundLog.emplace_back(updates, sound.id);
    }
    scriptedRooms.clear();
    for (const std::string& room : levelScripts.takeRoomRequests()) scriptedRooms.insert(runtime::roomNumber(room));
    for (const std::size_t vehicle : pendingVehicleBreaks)
        if (!vehicleDrives[vehicle].wreck.active) breakVehicleNode(vehicle);
    pendingVehicleBreaks.clear();
}

// 0x2ec070 on the real clock, then the scale it blends becomes the scheduler's C+0x1c for the next step. STAND-IN SCRIPT-S323: the system steps at
// the start of the level update (the original's order of the system updates is not read).
void LevelSession::updateBulletTime(float realDt) {
    runtime::BulletTimeSystem& bulletTime = levelScripts.bulletTime();
    const std::uint16_t owner = levelScripts.soundNatives().ownerOf("player1");
    const bool loopIdle = shared.audio && shared.audio->open() && !shared.audio->playing(runtime::kBulletTimeSoundStart, owner) &&
                          !shared.audio->playing(runtime::kBulletTimeSoundLoop, owner);
    const bool reached = bulletTime.update(realDt, loopIdle);
    if (scheduler) scheduler->setScale(bulletTime.timeScale());
    if (reached) std::cout << "time scale " << bulletTime.timeScale() << " at update " << updatesForLog << '\n';
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

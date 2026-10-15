#include "runtime/collision_enable.hpp"
#include "game_level.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/script_placement.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase::assets;
using game_doors::DoorObject;
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

namespace {
constexpr float kScriptDt = 0.033f;
}

void LevelSession::advance(const runtime::PadState& liveInput) {
    if (script.empty()) scheduler->advance(now(), [&](float dt) { simulate(dt, liveInput); });
    else if (updates + shared.scriptOffset < scriptLength) simulate(kScriptDt, scriptedPad(updates + shared.scriptOffset));
}

runtime::PadState LevelSession::scriptedPad(unsigned index) const {
    runtime::PadState scripted;
    for (const game_options::ScriptStep& step : script) {
        if (index < step.updates) {
            scripted.stickX = std::int16_t(step.stickX);
            scripted.stickY = std::int16_t(step.stickY);
            scripted.held = step.buttons;
            break;
        }
        index -= step.updates;
    }
    return scripted;
}

void LevelSession::requestSceneChange(const runtime::SceneChangeObject* object) {
    if (object && object->destination && sceneChangeFlow.request(*object->destination))
        std::cout << "scene change requested by " << object->instance << " at update " << updatesForLog << '\n';
}

void LevelSession::reportUnhandled() {
    const std::vector<std::string>& names = levelScripts.unhandledFunctions();
    for (; reportedUnhandled < names.size(); ++reportedUnhandled)
        std::cout << "script function without handler (continues at once): " << names[reportedUnhandled] << '\n';
    const std::vector<std::string>& events = levelScripts.unhandledTriggerEvents();
    for (; reportedEvents < events.size(); ++reportedEvents)
        std::cout << "script trigger event without check (never fires): " << events[reportedEvents] << '\n';
}

void LevelSession::applyPlacement(const runtime::ScriptPlacement& request) {
    const std::optional<Placement> target = objectStart(level, request.target);
    if (request.object == "player1" && player && target) {
        player->motion.x = target->position[0];
        player->vertical.y = target->position[1];
        player->motion.z = target->position[2];
        player->motion.facing = game_player::headingFromDegrees(target->orientation[1]);
        player->motion.lastIntent = player->motion.facing;
        if (runtime::entityHasController(runtime::kPlayerEntityKind, 0, runtime::measuredControllerKindList()))
            runtime::resetControllerAfterPlacement(player->vertical);
        cancelBoardingByPlacement();
        player->pose(scene);
    } else {
        std::cout << "PositionObject not applied (only the player is moved): " << request.object << " to " << request.target << '\n';
    }
}

void LevelSession::updateDoors(const float playerPosition[3], const std::function<void(const runtime::SoundCall&)>& emit) {
    std::vector<runtime::DoorSoundEvent> doorSounds;
    for (DoorObject& door : doors) {
        doorSounds.clear();
        if (objectState.isDisabled(door.instance) || levelScripts.objectNatives().entityUpdateSkipped(game_doors::lowerCase(door.instance))) continue;
        runtime::doorUpdate(door.state, door.params, runtime::doorZoneContains(door.placement.data(), playerPosition, door.params), doorSounds);
        game_doors::poseDoor(scene, door);
        for (const runtime::DoorSoundEvent& event : doorSounds) {
            if (event.id == 0) continue;
            if (event.action == runtime::DoorSoundEvent::Action::Play) emit({event.id, door.position, door.soundOwner});
            else if (shared.audio) shared.audio->stop(event.id, door.soundOwner);
        }
    }
}

void LevelSession::updateCarry(const float playerPosition[3]) {
    runtime::CarryInput input;
    std::copy_n(playerPosition, 3, input.player);
    const float heading = runtime::headingRadians(player->motion.facing);
    input.facing[0] = std::sin(heading);
    input.facing[2] = std::cos(heading);
    input.actionPressed = pad.slot(kSlotConfirmButton).press > 0;
    input.playerFree = !levelScripts.playerControlTaken() && !cutscenePlaying();
    runtime::applyCarryActions(carry, carry.update(objectState, input), objectState, levelScripts);
    for (const runtime::CarryObject& object : carry.objects())
        for (std::size_t i = 0; i < placedObjects.size(); ++i) {
            if (game_doors::lowerCase(placedObjects[i].instance) != object.instance) continue;
            Placement moved = placedObjects[i];
            std::copy_n(object.position, 3, moved.position);
            scene.setTransform(placedSceneIndex[i], placementMatrix(moved));
        }
}

void LevelSession::updateLevelCharacters(float dt) {
    levelCharacters->update(scene, dt, [this](const chase::assets::CharacterObject& character) {
        return !objectState.isDisabled(character.placement.instance) && roomShown(runtime::roomNumber(character.placement.room));
    });
}

void LevelSession::updateBuildits(const float playerPosition[3]) {
    const std::vector<runtime::BuilditAction> actions = buildits.update(objectState, playerPosition, pad.slot(kSlotConfirmButton).value > 0.0f);
    runtime::applyBuilditActions(buildits, actions, objectState, levelScripts);
}

void LevelSession::updateFollowCamera(float dt) {
    runtime::FollowCameraInput input;
    input.dt = dt;
    input.target[0] = player->motion.x;
    input.target[1] = player->vertical.y;
    input.target[2] = player->motion.z;
    input.targetFacing = runtime::headingRadians(player->motion.facing);
    input.stickTurn = runtime::stickTurnInput(pad.slot(kSlotStickX).value, game_player::kPlayerEntityType);
    input.shoulderTurn = runtime::shoulderTurnInput(pad.slot(kSlotLeftShoulder).value, pad.slot(kSlotRightShoulder).value, false);
    runtime::DirectorCamera& director = levelScripts.director();
    director.update(dt, [this](const std::string& object, float out[3]) {
        if (object == "player1" && player) {
            out[0] = player->motion.x;
            out[1] = player->vertical.y;
            out[2] = player->motion.z;
            return true;
        }
        const std::optional<Placement> placed = objectStart(level, object);
        if (placed)
            for (int a = 0; a < 3; ++a) out[a] = placed->position[a];
        return placed.has_value();
    });
    follow->setDirector(director.pose(), director.takeRequests());
    if (director.takeSnapRequest()) follow->snapFollow(input.target, input.targetFacing);
    follow->update(input);
    if (follow->locked() && (pad.slot(kSlotLeftShoulder).value != 0.0f || pad.slot(kSlotRightShoulder).value != 0.0f))
        cameraLockIcon.trigger();
    cameraLockIcon.update(dt);
    if (hud) hud->setCameraLockAlpha(cameraLockIcon.alpha());
    if (shared.audio) {
        const float* eye = follow->eye();
        const float* look = follow->lookAt();
        shared.audio->setListener({eye[0], eye[1], eye[2]}, {look[0] - eye[0], look[1] - eye[1], look[2] - eye[2]});
        shared.audio->update();
    }
}

// One update of the game: the player reads the pad and the camera yaw first, then the camera moves
// (the heading of the recorded traces is reproduced with the yaw from before the camera step).
void LevelSession::simulate(float dt, const runtime::PadState& padInput) {
    pad.poll(padInput);
    updatesForLog = updates;
    if (sceneChangeFlow.pending()) {
        sceneChangeFlow.update(pad.slot(kSlotConfirmButton).press > 0);
        if (const auto due = sceneChangeFlow.takeSwitch()) switchTo = due;
        ++updates;
        ++updatesThisSecond;
        return;
    }
    if (levelScripts.dialogOpen()) {
        if (!dialogReported) std::cout << "script dialog opened at update " << updates << '\n';
        dialogReported = true;
        if (pad.slot(kSlotConfirmButton).press > 0) levelScripts.answerDialog(runtime::kDialogConfirm);
        else if (pad.slot(kSlotCancelButton).press > 0) levelScripts.answerDialog(runtime::kDialogCancel);
        if (!levelScripts.dialogOpen()) {
            std::cout << "script dialog answered at update " << updates << '\n';
            dialogReported = false;
        }
        ++updates;
        ++updatesThisSecond;
        return;
    }
    if (holdForMovie(dt)) {
        ++updates;
        ++updatesThisSecond;
        return;
    }
    const auto emit = [&](const runtime::SoundCall& call) {
        soundLog.emplace_back(updates, call.id);
        if (shared.audio) shared.audio->play(call);
    };
    runtime::applyObjectCollisionState(collision, objectState);
    suppressDrivenVehicleCollision();
    if (player && !cutscenePlaying()) {
        player->sounds.clear();
        runtime::StickIntentInput stick;
        stick.slot14 = {pad.slot(kSlotStickX).value, pad.slot(kSlotStickX).deadzone};
        stick.slot15 = {pad.slot(kSlotStickY).value, pad.slot(kSlotStickY).deadzone};
        const bool controlTaken = levelScripts.playerControlTaken() || carry.busy();
        if (controlTaken) stick.slot14.value = stick.slot15.value = 0.0f;
        stick.entityType = game_player::kPlayerEntityType;
        stick.cameraYaw = follow->yaw();
        if (hud) {
            hud->update(dt);
            const fe::StudSink sink = hud->studSink();
            studs.setFlightTarget(runtime::unprojectTopScreen(sink.x, sink.y, fe::kStudSinkDepth, follow->eye(), follow->yaw(),
                                                              follow->pitch(), follow->verticalFieldOfView()));
        }
        const runtime::BoardingStep boardingStep = stepBoarding(dt);
        game_player::PlayerAnimation shown =
            boardingStep.takesControl ? applyBoarding(boardingStep)
                                      : game_player::updatePlayer(*player, collision, runtime::stickIntent(stick), !controlTaken && pad.slot(kSlotJumpButton).press > 0, dt);
        if (const runtime::AiCharacter* driven = levelScripts.ai().moving("player1") ? levelScripts.ai().character("player1") : nullptr) {
            game_player::applyAiPosition(*player, *driven);
            shown = game_player::aiMoveAnimation(*driven);
        }
        player->animate(scene, dt, shown.role, shown.scale);
        if (!script.empty()) scriptedHeights.push_back(player->vertical.y);
        for (const runtime::StudCollected& collected :
             studs.update(dt, runtime::studPickupPoint(player->motion.x, player->vertical.y, player->motion.z,
                                                       runtime::headingRadians(player->motion.facing))))
            player->sounds.push_back(runtime::studCollectCall(collected, studs.studs()[collected.index].centre));
        for (const runtime::SoundCall& call : player->sounds) emit(call);
        if (hud)
            for (const runtime::StudBanked& banked : studs.takeBanked()) hud->studsBanked(unsigned(banked.kind), banked.total);
    }
    if (player) {
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        updateDoors(playerPosition, emit);
        updateBuildits(playerPosition);
        updateCarry(playerPosition);
    }
    sceneStart.startRequested();
    if (player && !levelScripts.ai().moving("player1")) levelScripts.ai().setPosition("player1", player->position().data());
    levelScripts.update(dt);
    cutsceneHost->update(levelScripts.cutscenes());
    if (player && levelScripts.ai().movedLastUpdate("player1")) game_player::applyAiPosition(*player, *levelScripts.ai().character("player1"));
    studs.addToCounter(levelScripts.progressNatives().takeStudsAdded());
    saveWriter.update(levelScripts.progressNatives().saveRequests(), levelScripts.progressNatives().slotImage());
    sceneStart.update(dt);
    if (player) {
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        const bool inVehicle = boarding && boarding->seatedVehicle();
        levelScripts.checkTriggers(playerPosition, inVehicle);
        for (const std::string& started : levelScripts.updateWaypoints(dt, playerPosition, previousWaypointVehicle ? &*previousWaypointVehicle : nullptr))
            std::cout << "waypoint started script " << started << " at update " << updates << '\n';
        previousWaypointVehicle = waypointVehicle();
    }
    reportUnhandled();
    for (const std::string& bound : levelScripts.takeSceneChanges())
        requestSceneChange(runtime::findSceneChange(shared.sceneChanges, currentWorldLevel.instance, bound));
    if (player) {
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        for (const runtime::SceneChangeObject& object : shared.sceneChanges)
            if (!object.boundName.empty() && object.boundWorldLevel == currentWorldLevel.instance &&
                levelScripts.playerInNamedBound(object.boundName, playerPosition))
                requestSceneChange(&object);
    }
    for (const runtime::ScriptPlacement& request : levelScripts.takePlacements()) applyPlacement(request);
    for (const runtime::ScriptObjectMessage& sent : levelScripts.takeObjectMessages())
        for (DoorObject& door : doors)
            if (game_doors::lowerCase(door.instance) == sent.instance)
                runtime::doorMessage(door.state, door.params,
                                     sent.message == runtime::kMessageTrigger ? runtime::DoorMessage::Open : runtime::DoorMessage::Close);
    provisionalStudClockSeconds += double(dt);
    updateLevelCharacters(dt);
    if (follow) updateFollowCamera(dt);
    else camera.update(dt, pad);
    updateRoomStreaming();
    ++updates;
    ++updatesThisSecond;
}

}

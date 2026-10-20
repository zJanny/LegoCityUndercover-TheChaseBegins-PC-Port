#include "runtime/collision_enable.hpp"
#include "game_level.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/update_order.hpp"
#include "runtime/script_placement.hpp"
#include <cmath>
#include <iostream>
#include <utility>

namespace game_level {
using namespace chase::assets;
using game_doors::DoorObject;
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

namespace {
constexpr float kScriptDt = 0.033f;
}

void LevelSession::advance(const runtime::PadState& liveInput) {
    if (script.empty()) scheduler->advance(pacer.present(now()), [&](float dt) { simulate(dt, liveInput); });
    else if (updates + shared.scriptOffset < scriptLength) {
        applyScriptTeleports(updates + shared.scriptOffset);
        simulate(kScriptDt, scriptedPad(updates + shared.scriptOffset));
    }
}

void LevelSession::applyScriptTeleports(unsigned index) {
    unsigned start = 0;
    for (const game_options::ScriptStep& step : script) {
        if (step.teleport && start == index && player) {
            if (placeSeatedVehicle(step.position.data(), step.facingDegrees)) std::cout << "script teleport of the seated vehicle: ";
            else placePlayer(step.position.data(), step.facingDegrees);
            std::cout << "script teleport to " << step.position[0] << ' ' << step.position[1] << ' ' << step.position[2] << " at update " << updates << '\n';
        }
        start += step.updates;
        if (start > index) break;
    }
}

void LevelSession::placePlayer(const float position[3], std::optional<float> facingDegrees) {
    player->motion.x = position[0];
    player->vertical.y = position[1];
    player->motion.z = position[2];
    if (facingDegrees) {
        player->motion.facing = game_player::headingFromDegrees(*facingDegrees);
        player->motion.lastIntent = player->motion.facing;
    }
    runtime::resetControllerAfterPlacement(player->vertical);
    cancelBoardingByPlacement();
    player->pose(scene);
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

void LevelSession::requestSameLevelEntry(const runtime::SameLevelEntry* entry) {
    if (entry && sameLevelFlow.request(*entry))
        std::cout << "same-level entry requested by " << entry->instance << " at update " << updatesForLog << '\n';
}

void LevelSession::applySameLevelEntry(const runtime::SameLevelEntry& entry) {
    const std::optional<Placement> point = runtime::destinationPoint(level, {{}, entry.pointHash});
    if (!point) throw std::runtime_error("same-level entry destination point missing for " + entry.instance);
    placePlayer(point->position, point->orientation[1]);
    std::cout << "same-level entry " << entry.instance << " to " << point->instance << " at update " << updates << '\n';
}

void LevelSession::requestSceneChange(const runtime::SceneChangeObject* object) {
    if (object && object->destination && runtime::destinationInLoadedLevel(*object->destination, currentWorldLevel.instance)) return;
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
    if (request.object == "player1" && player && target && boarding && boarding->seatedVehicle()) {
        if (placeSeatedVehicle(target->position, target->orientation[1])) {
            runtime::resetControllerAfterPlacement(player->vertical, true);
            std::cout << "PositionPlayer: the seated vehicle to " << request.target << " at update " << updates << '\n';
            return;
        }
    }
    if (request.object == "player1" && player && target) {
        player->motion.x = target->position[0];
        player->vertical.y = target->position[1];
        player->motion.z = target->position[2];
        player->motion.facing = game_player::headingFromDegrees(target->orientation[1]);
        player->motion.lastIntent = player->motion.facing;
        if (runtime::entityHasController(runtime::kPlayerEntityKind, 0, runtime::measuredControllerKindList()))
            runtime::resetControllerAfterPlacement(player->vertical);
        cancelBoardingByPlacement();
        if (runtime::takeNodeShow(player->vertical.placement) && !cutscenePlaying()) player->setVisible(scene, true);
        player->pose(scene);
        return;
    }
    if (target && placeVehicle(request.object, *target)) return;
    if (target && levelCharacters->place(request.object, placementMatrix(*target))) {
        std::cout << "PositionObject: " << request.object << " to " << request.target << " at update " << updates << '\n';
        return;
    }
    if (target && placeProp(request.object, *target)) {
        std::cout << "PositionObject: " << request.object << " to " << request.target << " at update " << updates << '\n';
        return;
    }
    std::cout << "PositionObject not applied (only the player, vehicles, level characters and placed props are moved): " << request.object << " to " << request.target << '\n';
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
    const auto heldWorld = [&](const runtime::CarryObject& object, std::size_t placedIndex) {
        return object.carried ? heldObjects.world(shared.archive, placedObjects[placedIndex], object.small, *player) : std::nullopt;
    };
    if (carry.carriedIndex() >= 0) {
        const runtime::CarryObject& held = carry.objects()[std::size_t(carry.carriedIndex())];
        for (std::size_t i = 0; i < placedObjects.size(); ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == held.instance)
                if (const auto world = heldWorld(held, i)) {
                    input.hasHand = true;
                    std::copy_n(world->data() + 12, 3, input.hand);
                    input.handYawDegrees = std::atan2((*world)[8], (*world)[10]) * 57.2957795f;
                    runtime::GroundProbe probe;
                    probe.x = input.hand[0];
                    probe.y = input.hand[1];
                    probe.z = input.hand[2];
                    probe.distance = runtime::kCarryReleaseRayReach;
                    if (const auto ground = runtime::groundHeight(collision, probe)) {
                        input.hasGround = true;
                        input.groundY = *ground;
                    }
                }
    }
    runtime::applyCarryActions(carry, carry.update(objectState, input), objectState, levelScripts);
    if (float target[3]; carry.turnTarget(target)) player->motion.lastIntent = game_carry::headingTowards(*player, target);
    for (const runtime::CarryObject& object : carry.objects())
        for (std::size_t i = 0; i < placedObjects.size(); ++i) {
            if (game_doors::lowerCase(placedObjects[i].instance) != object.instance) continue;
            if (const auto world = heldWorld(object, i)) {
                scene.setTransform(placedSceneIndex[i], *world);
                continue;
            }
            Placement moved = placedObjects[i];
            std::copy_n(object.position, 3, moved.position);
            if (object.hasYaw) moved.orientation[0] = moved.orientation[2] = 0, moved.orientation[1] = object.yawDegrees;
            scene.setTransform(placedSceneIndex[i], placementMatrix(moved));
        }
}

void LevelSession::updateLevelCharacters(float dt) {
    levelCharacters->update(scene, dt, [this](const chase::assets::CharacterObject& character) {
        return !objectState.isDisabled(character.placement.instance) && roomShown(runtime::roomNumber(character.placement.room));
    });
    updatePropAnimations(dt);
}

std::vector<runtime::SoundCall> LevelSession::updateBuildits(const float playerPosition[3]) {
    const bool held = pad.slot(kSlotConfirmButton).value > 0.0f;
    const bool carrying = carry.carryingMode() != 0 || carry.busy();
    const std::vector<runtime::BuilditAction> actions = buildits.update(objectState, playerPosition, held, carrying);
    runtime::applyBuilditActions(buildits, actions, objectState, levelScripts);
    for (const runtime::BuilditAction& action : actions)
        if (action.kind == runtime::BuilditActionKind::Built) {
            const runtime::BuilditObject& built = buildits.objects()[action.index];
            if (const auto mode = studDrops.drop(runtime::builditFinishStuds(built), debrisLineStandIn(), debrisLineStandIn()))
                std::cout << "buildit " << built.instance << ": stud drop mode " << unsigned(*mode) << '\n';
        }
    const bool free = !carrying && !levelScripts.playerControlTaken() && !cutscenePlaying() && player->vertical.posture == runtime::Posture::Ground;
    builditPlayer.update(buildits.lastUse(), pad.slot(kSlotConfirmButton).press > 0, held, free);
    return runtime::builditSounds(buildits, actions, runtime::kBuilditSoundOwnerBase);
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
    selectCameraSubject(input);
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
    follow->setBoundsEnabled(levelScripts.boundsEnabled());
    follow->setDirector(director.pose(), director.takeRequests());
    if (director.takeSnapRequest() || std::exchange(cameraSnapRequest, false)) follow->snapFollow(input.target, input.targetFacing);
    follow->update(input);
    applyScreenShake(dt);
    if (follow->locked() && (pad.slot(kSlotLeftShoulder).value != 0.0f || pad.slot(kSlotRightShoulder).value != 0.0f))
        cameraLockIcon.trigger();
    cameraLockIcon.update(dt);
    if (hud) hud->setCameraLockAlpha(cameraLockIcon.alpha());
    if (shared.audio) {
        const float* eye = follow->eye();
        const float* look = follow->lookAt();
        shared.audio->setListener({eye[0], eye[1], eye[2]}, {look[0] - eye[0], look[1] - eye[1], look[2] - eye[2]});
        shared.audio->update();
        shared.audio->updateMusic(dt);
    }
}

// One update of the game. The first part follows runtime::updatePhases() (docs/re/scripts-runtime.md); the parts without a phase
// run after it in a fixed order (STAND-IN SCRIPT-S284). The heading of the recorded traces is reproduced with the yaw from before the camera step.
void LevelSession::simulate(float dt, const runtime::PadState& padInput) {
    pad.poll(padInput);
    updatesForLog = updates;
    sameLevelFlow.update();
    if (const auto due = sameLevelFlow.takeEntry()) applySameLevelEntry(*due);
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
    logCutsceneChange();
    if (holdForMovie(dt)) {
        ++updates;
        ++updatesThisSecond;
        return;
    }
    if (holdForSuperBuildit(dt)) return;
    if (holdForPause(dt, padInput)) return;
    particles.update(dt);
    stepVehicleFlashes();
    const auto emit = [&](const runtime::SoundCall& call) {
        soundLog.emplace_back(updates, call.id);
        if (shared.audio) shared.audio->play(call);
    };
    runtime::UpdateSequence order;
    order.at(runtime::UpdatePhase::Systems, [&] { updatePedestrianSystem(); });
    order.at(runtime::UpdatePhase::Runner, [&] {
        sceneStart.startRequested();
        if (player && !levelScripts.ai().moving("player1")) levelScripts.ai().setPosition("player1", player->position().data());
        levelScripts.update(dt);
        applyScriptNatives();
        runtime::applyObjectCollisionState(collision, objectState);
        suppressDrivenVehicleCollision();
    });
    order.at(runtime::UpdatePhase::Characters, [&] {
    if (player && !cutscenePlaying()) {
        player->sounds.clear();
        runtime::StickIntentInput stick;
        stick.slot14 = {pad.slot(kSlotStickX).value, pad.slot(kSlotStickX).deadzone};
        stick.slot15 = {pad.slot(kSlotStickY).value, pad.slot(kSlotStickY).deadzone};
        const runtime::PlayerStateStep scriptStep = playerScriptAnimation.update(dt);
        const bool scriptClip = scriptStep != runtime::PlayerStateStep::Locomotion;
        const bool controlTaken = scriptClip || levelScripts.playerControlTaken() || carry.busy() || builditPlayer.active() || detective.controlTaken() || scanner.takesPlayerControl() ||
                                  fastTravel.takesPlayerControl();
        if (controlTaken) stick.slot14.value = stick.slot15.value = 0.0f;
        stick.entityType = game_player::kPlayerEntityType;
        stick.cameraYaw = follow->yaw();
        if (hud) {
            hud->update(dt);
            syncVehicleHud();
            updateHeadIcon(dt);
            const fe::StudSink sink = hud->studSink();
            studs.setFlightTarget(runtime::unprojectTopScreen(sink.x, sink.y, fe::kStudSinkDepth, follow->eye(), follow->yaw(),
                                                              follow->pitch(), follow->verticalFieldOfView()));
        }
        player->motion.carryingBig = carry.carryingMode() == 2;
        player->motion.runRole = player->carryAnimations ? player->carryAnimations->clip(runtime::kAnimationRun, carry.carryingMode()) : runtime::kAnimationRun;
        player->motion.detectiveWalk = detective.modeActive();
        if (detective.modeActive() && detectiveRoles) player->motion.runRole = detectiveRoles->clip(runtime::kAnimationRun);
        pole.cameraYaw = follow->yaw();
        pole.usePressed = !controlTaken && pad.slot(kSlotConfirmButton).press > 0;
        const runtime::BoardingStep boardingStep = stepBoarding(dt);
        const runtime::Posture postureBeforeStep = player->vertical.posture;
        const float fallSpeedBeforeStep = player->vertical.fallSpeed;
        const std::optional<runtime::ScriptRootStep> scriptRoot = scriptStep == runtime::PlayerStateStep::UserAnimation ? playerScriptAnimation.rootMotion(player->motion.facing) : std::nullopt;
        if (scriptRoot) runtime::applyScriptRootStep(*scriptRoot, player->motion, player->vertical, collision, dt);
        game_player::PlayerAnimation shown =
            boardingStep.takesControl ? applyBoarding(boardingStep)
            : scriptClip              ? game_player::PlayerAnimation{}
                                      : attackMovePlayer(runtime::stickIntent(stick), !controlTaken && carry.allowsJump() && !detective.modeActive() && pad.slot(kSlotJumpButton).press > 0,
                                                         !controlTaken && pad.slot(kSlotConfirmButton).press > 0, pad.slot(kSlotButtonY).value > 0.0f,
                                                         !controlTaken && carry.carryingMode() == 0 && !detective.modeActive(), dt);
        deliverAttackHits();
        bounceOnLanding(postureBeforeStep, fallSpeedBeforeStep);
        if (const runtime::AiCharacter* driven = levelScripts.ai().moving("player1") ? levelScripts.ai().character("player1") : nullptr) {
            game_player::applyAiPosition(*player, *driven);
            shown = game_player::aiMoveAnimation(*driven);
        }
        if (const std::optional<unsigned> entered = playerScriptAnimation.enteredState()) {
            runtime::applyScriptStateEnter(*entered, player->vertical);
            std::cout << "player state " << *entered << " entered at update " << updates << '\n';
        }
        player->animate(scene, dt, detectiveClip(player->carriedRole(shown.role, carry.carryingMode(), carry.phaseClip() ? carry.phaseClip() : builditPlayer.clip() ? builditPlayer.clip() : fastTravel.playerClip() ? fastTravel.playerClip() : scanner.playerClip())), shown.scale);
        if (const auto* clip = playerScriptAnimation.posesScriptClip() ? playerScriptAnimation.animation() : nullptr) player->poseClip(scene, *clip, playerScriptAnimation.frame());
        handItems.update(*player);
        if (!script.empty()) scriptedHeights.push_back(player->vertical.y);
        const std::array<float, 3> pickupPoint =
            runtime::studPickupPoint(player->motion.x, player->vertical.y, player->motion.z, runtime::headingRadians(player->motion.facing));
        for (const runtime::SoundCall& call : studDrops.update(dt, pickupPoint, studs.flightTarget(), studs)) player->sounds.push_back(call);
        for (const runtime::StudCollected& collected : studs.update(dt, pickupPoint))
            player->sounds.push_back(runtime::studCollectCall(collected, studs.studs()[collected.index].centre));
        for (const runtime::SoundCall& call : updateDebrisPickup(dt, pickupPoint)) player->sounds.push_back(call);
        effects.update(dt, {follow->eye()[0], follow->eye()[1], follow->eye()[2]}, superBricks.tint());
        for (const runtime::SoundCall& call : player->sounds) emit(call);
        if (hud)
            for (const runtime::StudBanked& banked : studs.takeBanked()) {
                hud->studsBanked(unsigned(banked.kind), banked.total);
                chase::save::setSlotStudType(levelScripts.progressNatives().slotImage().data(), unsigned(banked.kind));
            }
    }
    });
    order.at(runtime::UpdatePhase::Objects, [&] { updatePedestrianCharacters(dt); });
    order.at(runtime::UpdatePhase::Objects, [&] {
        if (!player) return;
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        updateDoors(playerPosition, emit);
    });
    order.at(runtime::UpdatePhase::AfterCharactersStandIn, [&] {
        if (!player) return;
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        for (const runtime::SoundCall& call : updateBuildits(playerPosition)) emit(call);
        updateSuperBuildits(playerPosition, dt);
        updateFastTravel(dt);
        updateCarry(playerPosition);
        updateDetective(dt, carry.carryingMode() == 0 && !carry.busy() && !builditPlayer.active() && !buildits.lastUse().inZone, emit);
    });
    order.at(runtime::UpdatePhase::UseSpots, [&] {
        if (!player) return;
        updateUseSpots(carry.carryingMode() == 0 && !carry.busy() && !builditPlayer.active() && !buildits.lastUse().inZone);
        updateUseMarkers(dt);
        updateScanner();
        updateSuperBricks(dt);
    });
    order.run();
    propParts.step(dt);
    if (player && follow) mapMarkerField.advance(dt, player->motion.x, player->motion.z, follow->yaw(), activeMapMarkers());
    updateObjectiveMarkers(dt);
    cutsceneHost->update(levelScripts.cutscenes());
    if (player && levelScripts.ai().movedLastUpdate("player1")) game_player::applyAiPosition(*player, *levelScripts.ai().character("player1"));
    studs.addToCounter(levelScripts.progressNatives().takeStudsAdded());
    for (const runtime::MusicRequest& request : levelScripts.musicNatives().takeRequests())
        if (shared.audio) shared.audio->applyMusic(request);
    for (const runtime::SoundRequest& request : levelScripts.soundNatives().takeRequests())
        if (shared.audio) shared.audio->applySound(request);
    game_save::update(shared.progress, levelScripts.progressNatives(), seenSaveRequests, studs.counter(), superBricks.total(),
                      [this](chase::save::Bytes& slot) { game_save::refreshLevelState(slot, level, saveLevelId, saveLevelState); });
    sceneStart.update(dt);
    if (player) {
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        const bool inVehicle = boarding && boarding->seatedVehicle();
        levelScripts.checkTriggers(playerPosition, inVehicle);
        for (const std::string& started : levelScripts.updateWaypoints(dt, playerPosition, previousWaypointVehicle ? &*previousWaypointVehicle : nullptr)) {
            threadLog->learn(started);
            std::cout << "waypoint started script " << started << " at update " << updates << '\n';
        }
        previousWaypointVehicle = waypointVehicle();
    }
    reportUnhandled();
    for (const std::string& bound : levelScripts.takeSceneChanges()) {
        if (const runtime::SameLevelEntry* entry = runtime::findSameLevelEntry(sameLevelEntries, bound)) requestSameLevelEntry(entry);
        else requestSceneChange(runtime::findSceneChange(shared.sceneChanges, currentWorldLevel.instance, bound));
    }
    if (player) {
        const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
        for (const runtime::SceneChangeObject& object : shared.sceneChanges)
            if (!object.boundName.empty() && object.boundWorldLevel == currentWorldLevel.instance &&
                levelScripts.playerInNamedBound(object.boundName, playerPosition))
                requestSceneChange(&object);
        for (const runtime::SameLevelEntry& entry : sameLevelEntries)
            if (levelScripts.playerInNamedBound(entry.boundName, playerPosition)) requestSameLevelEntry(&entry);
    }
    for (const runtime::ScriptPlacement& request : levelScripts.takePlacements()) applyPlacement(request);
    for (const runtime::ScriptObjectMessage& sent : levelScripts.takeObjectMessages()) {
        if (sent.message == runtime::kMessageTrigger) detective.triggerMessage(sent.instance);
        for (DoorObject& door : doors)
            if (game_doors::lowerCase(door.instance) == sent.instance)
                runtime::doorMessage(door.state, door.params,
                                     sent.message == runtime::kMessageTrigger ? runtime::DoorMessage::Open : runtime::DoorMessage::Close);
    }
    provisionalStudClockSeconds += double(dt);
    updateLevelCharacters(dt);
    fallApart.update(debrisLineStandIn(), updates);
    if (follow) updateFollowCamera(dt);
    else camera.update(dt, pad);
    updateRoomStreaming();
    logCutsceneChange();
    for (const std::string& line : threadLog->lines(levelScripts.takeThreadEvents(), updates)) std::cout << line << '\n';
    ++updates;
    ++updatesThisSecond;
}

}

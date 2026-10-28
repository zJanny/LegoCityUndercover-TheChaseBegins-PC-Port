#include "frontend/map_buttons_view.hpp"
#include "game_level.hpp"
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::loadFastTravel() {
    fastTravel.load(level, classOf);
    fastTravelState.helicopters.clear();
    for (std::size_t i = 0; i < fastTravel.pads().size(); ++i) {
        const runtime::FastTravelPad& pad = fastTravel.pads()[i];
        if (pad.helicopter.empty()) continue;
        game_fast_travel::Helicopter helicopter;
        helicopter.pad = i;
        helicopter.model = runtime::loadFastTravelHelicopter(shared.archive, pad.helicopterMesh);
        if (!helicopter.model) continue;
        fastTravel.setHelicopterClips(i, helicopter.model->clips);
        helicopter.key = runtime::propSceneKey(helicopter.model->modelPath, pad.helicopter);
        helicopter.sceneIndex = scene.instances();
        if (!scene.addModel(helicopter.model->modelPath, pad.helicopterWorld, {}, helicopter.key)) continue;
        scene.setInstanceVisible(helicopter.sceneIndex, false);
        fastTravelState.helicopters.push_back(std::move(helicopter));
    }
    std::cout << "fast travel pads: " << fastTravel.pads().size() << '\n';
    runtime::MapButtonsSetup setup = chase::frontend::mapButtonsSetup(hudResources);
    // STAND-IN UI-S8: the level index 0x51640e is the save's level of the session, and the count of 0x1bd424 (level block 2 +0x1c) is the
    // session's StoryProgress.
    setup.level = std::int8_t(saveLevelId);
    setup.scan = setup.level > 2 || (setup.level > 1 && fastTravelState.storyProgress > 2);
    fastTravelState.mapButtons.emplace(setup);
    if (shared.options.fastTravelArrival && !fastTravel.arrive(shared.options.fastTravelArrival))
        std::cout << "fast travel arrival: no pad with a Helicopter for the scene change's FastTravel reference\n";
}

void LevelSession::updateFastTravel(float dt, const runtime::PadState& padInput) {
    runtime::FastTravelInput input;
    input.dt = dt;
    input.player[0] = player->motion.x;
    input.player[1] = player->vertical.y;
    input.player[2] = player->motion.z;
    input.facing = player->motion.facing;
    input.playerFree = carry.carryingMode() == 0 && !carry.busy() && !levelScripts.playerControlTaken() && !cutscenePlaying() &&
                       !builditPlayer.active() && player->vertical.posture == runtime::Posture::Ground && !(boarding && boarding->seatedVehicle());
    input.actionPressed = pad.slot(kSlotConfirmButton).press > 0;
    updateMapButtons(dt, padInput, input.playerFree);
    runtime::FastTravelPlayer step;
    for (const runtime::FastTravelEvent& event : fastTravel.update(objectState, input, step)) {
        const std::string& name = fastTravel.pads()[event.pad].instance;
        switch (event.kind) {
        case runtime::FastTravelEventKind::Entered:
            std::cout << "fast travel " << name << ": state 0x180 entered at update " << updates << '\n';
            // STAND-IN LEVOBJ-S103: the unlock bits 0x5208ae of the slots are not read from the save; every slot is locked.
            if (fastTravelState.mapButtons) fastTravelState.mapButtons->openFastTravel(0);
            break;
        case runtime::FastTravelEventKind::Active:
            std::cout << "fast travel " << name << ": state 0x181 entered at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::HelicopterIdle:
            std::cout << "fast travel " << name << ": helicopter landed, map ready at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::HelicopterHidden:
            std::cout << "fast travel " << name << ": helicopter hidden at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::TakeoffStarted:
            std::cout << "fast travel " << name << ": helicopter takeoff at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::Arrived:
            std::cout << "fast travel " << name << ": state 0x183 entered at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::Released:
            std::cout << "fast travel " << name << ": player state 1 entered at update " << updates << '\n';
            break;
        }
    }
    if (step.moved) {
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
    }
    if (step.turned) player->motion.facing = step.facing;
    if (step.resetRunTimer) player->motion.runTimer = 0.0f;
    if (step.lastIntent) player->motion.lastIntent = *step.lastIntent;
    for (const game_fast_travel::Helicopter& helicopter : fastTravelState.helicopters) {
        const runtime::FastTravelHelicopter& flying = fastTravel.helicopter(helicopter.pad);
        scene.setInstanceVisible(helicopter.sceneIndex, flying.shown && !objectState.isDisabled(fastTravel.pads()[helicopter.pad].instance));
        if (!flying.shown) continue;
        const std::vector<chase::assets::Matrix> skin = runtime::fastTravelHelicopterPose(*helicopter.model, flying, fastTravel.clock());
        if (!skin.empty()) scene.pose(helicopter.key, skin);
    }
}

}

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::updateMapButtons(float dt, const runtime::PadState& padInput, bool playerFree) {
    fastTravelState.touchPhase = runtime::nextTouchPhase(fastTravelState.touchPhase, padInput.touch);
    if (!fastTravelState.mapButtons) return;
    runtime::MapButtons& buttons = *fastTravelState.mapButtons;
    buttons.setBoxes(chase::frontend::mapButtonBoxes(hudResources, buttons));
    runtime::MapButtonsInput input;
    input.dt = dt;
    input.phase = fastTravelState.touchPhase;
    input.x = float(padInput.touchX);
    input.y = float(padInput.touchY);
    // STAND-IN UI-S8: 0x20ac88 and 0x27c298 are the port's "player free" flag outside the scanner, characters are always available (0x51b24d),
    // the scanner never answers ready (0x1ee4f0) and nothing blocks the update.
    input.scannerActive = scanner.viewShown();
    input.canChangeCharacter = playerFree && !input.scannerActive;
    input.characterSwitchReady = input.canChangeCharacter;
    input.charactersAvailable = true;
    input.mapReady = fastTravel.mapReady();
    for (const runtime::MapButtonsEvent& event : buttons.update(input)) {
        switch (event.kind) {
        case runtime::MapButtonsEventKind::Sound:
            if (shared.audio) shared.audio->playUi(unsigned(event.value));
            break;
        case runtime::MapButtonsEventKind::FastTravelClosed:
            std::cout << "map buttons: fast travel map closed at update " << updates << '\n';
            if (fastTravel.requestExit(true)) std::cout << "fast travel: exit touched at update " << updates << '\n';
            break;
        case runtime::MapButtonsEventKind::CharactersTapped:
            std::cout << "map buttons: characters tapped at update " << updates << " (the character change is not ported)\n";
            break;
        case runtime::MapButtonsEventKind::CharactersOpened:
            std::cout << "map buttons: characters panel opened at update " << updates << '\n';
            break;
        case runtime::MapButtonsEventKind::CharactersClosed:
            std::cout << "map buttons: characters panel closed at update " << updates << '\n';
            break;
        case runtime::MapButtonsEventKind::MapSwitch:
            std::cout << "map buttons: map of level " << event.value << " at update " << updates << '\n';
            break;
        case runtime::MapButtonsEventKind::FastTravelQuestion:
        case runtime::MapButtonsEventKind::FastTravelGo:
            std::cout << "map buttons: fast travel slot " << event.value << " at update " << updates << " (the question is not ported)\n";
            break;
        default:
            std::cout << "map buttons: button event " << int(event.kind) << " at update " << updates << " (not ported)\n";
            break;
        }
    }
}

}

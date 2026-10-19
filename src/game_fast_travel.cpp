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
}

void LevelSession::updateFastTravel(float dt) {
    runtime::FastTravelInput input;
    input.dt = dt;
    input.player[0] = player->motion.x;
    input.player[1] = player->vertical.y;
    input.player[2] = player->motion.z;
    input.facing = player->motion.facing;
    input.playerFree = carry.carryingMode() == 0 && !carry.busy() && !levelScripts.playerControlTaken() && !cutscenePlaying() &&
                       !builditPlayer.active() && player->vertical.posture == runtime::Posture::Ground && !(boarding && boarding->seatedVehicle());
    input.actionPressed = pad.slot(kSlotConfirmButton).press > 0;
    runtime::FastTravelPlayer step;
    for (const runtime::FastTravelEvent& event : fastTravel.update(objectState, input, step)) {
        const std::string& name = fastTravel.pads()[event.pad].instance;
        switch (event.kind) {
        case runtime::FastTravelEventKind::Entered:
            std::cout << "fast travel " << name << ": state 0x180 entered at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::Active:
            std::cout << "fast travel " << name << ": state 0x181 entered at update " << updates << '\n';
            break;
        case runtime::FastTravelEventKind::HelicopterIdle:
            std::cout << "fast travel " << name << ": helicopter landed, map ready at update " << updates << " (the map menu is not ported)\n";
            break;
        case runtime::FastTravelEventKind::HelicopterHidden:
            std::cout << "fast travel " << name << ": helicopter hidden at update " << updates << '\n';
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

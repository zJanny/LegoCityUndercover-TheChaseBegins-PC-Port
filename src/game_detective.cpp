#include "game_level.hpp"
#include "runtime/collision.hpp"
#include "runtime/player_locomotion.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase::assets;
namespace runtime = chase::runtime;

namespace {
// STAND-IN DETECT-S3: the models of the footprint pool (the 32 entities at 0x515cf0 are made by code that was not read); FootprintType 0 and 1
// take the archive models whose names match.
constexpr const char* kFootprintModels[] = {"models/ability_detective_foot/ability_detective_foot.bfnmdl",
                                            "models/ability_detective_dog/ability_detective_dog.bfnmdl"};
// STAND-IN DETECT-S4: one sound owner key for every detective object (the u16 at entity +0x14 in the original).
constexpr std::uint16_t kDetectiveSoundOwnerBase = 0x5000;
// The footprint 0x26024c places on the ground: a ray from 1 above to 1 below the stored point (0x2501ac with +1.0 / -1.0, 0x21ab20).
constexpr float kFootprintRayAbove = 1.0f, kFootprintRayLength = 2.0f;
}

void LevelSession::loadDetective(FuseArchive& archive) {
    detective.load(level, classOf);
    std::cout << "detective: " << detective.spots().size() << " spots, " << detective.revealSpots().size() << " reveal spots, "
              << detective.revealProps().size() << " reveal props\n";
    if (const FuseEntry* animationTable = archive.find("tables/tablesanimations.bxls")) {
        const Table table = parseTable(archive.read(*animationTable));
        detectiveRoles.emplace(table);
        if (player && player->carryAnimations) {
            std::vector<int> clips = detectiveRoles->clips();
            for (const int id : {runtime::kAnimationDetectOut, runtime::kAnimationDetectIn, runtime::kAnimationDetectFind}) clips.push_back(id);
            for (const int id : clips) {
                const std::string path = std::string(game_player::kPlayerAnimationDirectory) + player->carryAnimations->name(id) + game_player::kPlayerAnimationSuffix;
                if (!player->animations.count(id) && archive.find(path)) player->animations[id] = chase::render::loadAnimation(archive, path);
            }
        }
    }
    for (unsigned slot = 0; slot < runtime::kFootprintPoolSize; ++slot) {
        const std::size_t instance = scene.instances();
        const char* model = kFootprintModels[std::min<std::size_t>(slot / runtime::kFootprintPoolPerType, 1)];
        if (scene.addModel(model)) {
            scene.setInstanceVisible(instance, false);
            footprintInstances.push_back(instance);
        } else {
            footprintInstances.push_back(std::size_t(-1));
        }
    }
}

int LevelSession::detectiveClip(int role) const {
    if (const int clip = detective.playerClip(); clip != 0 && player->animations.count(clip)) return clip;
    if (detective.modeActive() && detectiveRoles) {
        const int clip = detectiveRoles->clip(role);
        if (player->animations.count(clip)) return clip;
    }
    return role;
}

void LevelSession::updateDetective(float dt, bool actionFree, const std::function<void(const runtime::SoundCall&)>& emit) {
    runtime::DetectiveInput input;
    input.dt = dt;
    input.player[0] = player->motion.x;
    input.player[1] = player->vertical.y;
    input.player[2] = player->motion.z;
    const float heading = runtime::headingRadians(player->motion.facing);
    input.forward[0] = std::sin(heading);
    input.forward[2] = std::cos(heading);
    input.actionPressed = pad.slot(kSlotConfirmButton).press > 0;
    input.playerFree = actionFree && !levelScripts.playerControlTaken() && !cutscenePlaying() && player->vertical.posture == runtime::Posture::Ground;
    const std::vector<runtime::DetectiveAction> actions = detective.update(objectState, input);
    logDetectiveStates();
    for (const runtime::DetectiveAction& action : actions) {
        switch (action.kind) {
        case runtime::DetectiveActionKind::Show: objectState.disabled.erase(action.instance); break;
        case runtime::DetectiveActionKind::Hide: objectState.disabled.insert(action.instance); break;
        case runtime::DetectiveActionKind::Activate:
            std::cout << "detective: " << action.instance << " activated at update " << updates << '\n';
            levelScripts.triggerObjectEnabling(action.instance);
            break;
        case runtime::DetectiveActionKind::Sound: {
            if (action.sound == 0) break;
            runtime::SoundCall call;
            call.id = action.sound;
            call.position = {action.position[0], action.position[1], action.position[2]};
            if (!action.instance.empty())
                if (const std::optional<Placement> placed = objectStart(level, action.instance))
                    call.position = {placed->position[0], placed->position[1], placed->position[2]};
            call.owner = kDetectiveSoundOwnerBase;
            emit(call);
            break;
        }
        }
    }
}

void LevelSession::logDetectiveStates() {
    std::vector<int> states;
    for (const runtime::DetectiveSpot& spot : detective.spots()) states.push_back(spot.state);
    for (const runtime::RevealSpot& spot : detective.revealSpots()) states.push_back(spot.state);
    for (const runtime::RevealProp& prop : detective.revealProps()) states.push_back(prop.state);
    states.push_back(int(detective.playerState()));
    if (loggedDetectiveStates.size() != states.size()) loggedDetectiveStates.assign(states.size(), 0);
    std::size_t index = 0;
    const auto report = [&](const std::string& name, int state) {
        if (loggedDetectiveStates[index] != state) std::cout << "detective: " << name << " state " << state << " at update " << updates << '\n';
        loggedDetectiveStates[index++] = state;
    };
    for (const runtime::DetectiveSpot& spot : detective.spots()) report(spot.instance, spot.state);
    for (const runtime::RevealSpot& spot : detective.revealSpots()) report(spot.instance, spot.state);
    for (const runtime::RevealProp& prop : detective.revealProps()) report(prop.instance, prop.state);
    report("player", int(detective.playerState()));
}

void LevelSession::drawFootprints() {
    for (std::size_t instance : footprintInstances)
        if (instance != std::size_t(-1)) scene.setInstanceVisible(instance, false);
    for (const runtime::DetectiveSpot& spot : detective.spots())
        for (const runtime::DetectiveFootprint& footprint : spot.footprints) {
            if (footprint.slot < 0 || std::size_t(footprint.slot) >= footprintInstances.size()) continue;
            const std::size_t instance = footprintInstances[std::size_t(footprint.slot)];
            if (instance == std::size_t(-1)) continue;
            float point[3];
            for (int a = 0; a < 3; ++a) point[a] = spot.position[a] + float(footprint.offset[std::size_t(a)]) * 0.25f;
            runtime::GroundProbe probe;
            probe.x = point[0];
            probe.y = point[1] + kFootprintRayAbove;
            probe.z = point[2];
            probe.distance = kFootprintRayLength;
            if (const auto ground = runtime::groundHeight(collision, probe)) point[1] = *ground;
            chase::render::Mat4 matrix = chase::render::identity();
            matrix[12] = point[0];
            matrix[13] = point[1];
            matrix[14] = point[2];
            scene.setTransform(instance, matrix);
            scene.setInstanceFade(instance, float(footprint.alpha) / 255.0f);
            scene.setInstanceVisible(instance, footprint.alpha != 0);
        }
}

}

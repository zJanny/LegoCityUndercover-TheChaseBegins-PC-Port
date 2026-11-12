#include "game_level.hpp"
#include "runtime/player_controller_state.hpp"
#include <iostream>

namespace game_level {
using namespace chase;

void LevelSession::loadCopWhistle(chase::assets::FuseArchive& archive) {
    copWhistleClipLoaded = false;
    if (!player || !player->carryAnimations) return;
    const std::string path = std::string(game_player::kPlayerAnimationDirectory) + player->carryAnimations->name(runtime::kCopWhistleClipRow) +
                             game_player::kPlayerAnimationSuffix;
    const chase::assets::FuseEntry* entry = archive.find(path);
    if (!entry) return;
    if (!player->animations.count(runtime::kCopWhistleClipRow)) player->animations[runtime::kCopWhistleClipRow] = chase::assets::parseAnimation(archive.read(*entry));
    const runtime::AttackClip clip = runtime::makeAttackClip(runtime::kCopWhistleClipRow, player->animations.at(runtime::kCopWhistleClipRow));
    copWhistle.setClipUpdates(clip.updates);
    copWhistleClipLoaded = true;
    std::cout << "cop whistle: clip " << runtime::kCopWhistleClipRow << ", " << clip.updates << " updates\n";
}

// The D-pad Down press edge of the player input 0x200354 (pad slot 1). STAND-IN TRAFFIC-S43: the idle state, the free hands and the character
// class byte are derived from the port's player state, not read from the controller.
std::optional<game_player::PlayerAnimation> LevelSession::updateCopWhistle(bool playerFree) {
    if (!player || !copWhistleClipLoaded) return std::nullopt;
    runtime::PlayerCopWhistleInput input;
    input.extras = shared.options.extras.value_or(0);
    input.state = playerFree && !(boarding && boarding->seatedVehicle()) ? runtime::playerControllerState(player->vertical, player->animation) : 0;
    input.downPressed = pad.slot(1).press > 0;
    input.carrying = carry.carryingMode() != 0;
    input.clock = runtime::gameClockAfter(updatesForLog);
    const runtime::PlayerCopWhistleStep step = copWhistle.update(input, traffic.whistle(), traffic.random());
    if (step.entered) std::cout << "cop whistle: player state " << runtime::kControllerStateCopWhistle << " at update " << updatesForLog << '\n';
    if (step.finished) {
        std::cout << "cop whistle: player state 1 IDLE at update " << updatesForLog << '\n';
        return game_player::PlayerAnimation{0, 1.0f};
    }
    if (copWhistle.active()) return game_player::PlayerAnimation{runtime::kCopWhistleClipRow, 1.0f};
    return std::nullopt;
}

}

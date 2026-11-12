#include "game_level.hpp"

#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

// The state update's movement of a character in MELEE PUTDOWN (0x27726c) or a takedown state (0x283b10), before the takedown machine takes the clip
// end and the hook (runtime/character_air_mover.hpp). STAND-IN SCRIPT-S347: the pair motion of MELEE PUTDOWN (the alignment 0x285850 and the
// partner's slide correction 0x2057e4) and the clip's root motion (0x2c9770, its axis bits not read) are not ported, so the mover gets no movement.
void LevelSession::moveTakedownCharacter(const std::string& name, std::uint16_t state, bool clipFinished, float dt) {
    const runtime::TakedownMover mover = runtime::takedownMover(state);
    if (mover == runtime::TakedownMover::None) return;
    const std::optional<chase::assets::Matrix> world = levelCharacters->world(name);
    if (!world) return;
    runtime::CharacterAir& air = characterAir[name];
    chase::assets::Matrix node = *world;
    runtime::AirVector position{node[12], node[13], node[14]};
    const float forwardLength = std::hypot(node[8], node[10]);
    const float forwardX = forwardLength > 0 ? node[8] / forwardLength : 0.0f, forwardZ = forwardLength > 0 ? node[10] / forwardLength : 1.0f;
    const std::optional<game_characters::LevelCharacters::ClipFrame> clip = levelCharacters->currentClip(name);
    if (clip && clip->animation && clip->animation->frames > 0) runtime::sampleFallAllowed(air, *clip->animation, clip->frame / float(clip->animation->frames));
    const bool wasOnGround = air.onGround;
    runtime::CharacterMoveResult moved;
    if (const std::optional<runtime::ThrownFall> flight = runtime::thrownFlight(air, position, forwardX, forwardZ, collision, dt)) {
        moved = flight->move;
        if (flight->step == runtime::ThrownFallStep::Landed) {
            levelCharacters->pauseClip(name, false);
            std::cout << "character " << name << ": thrown fall ended at update " << updatesForLog << '\n';
        }
    } else if (mover == runtime::TakedownMover::Takedown || !clipFinished) {
        runtime::CharacterMove move;
        move.gravity = mover == runtime::TakedownMover::Takedown;
        moved = runtime::moveCharacter(air, position, forwardX, forwardZ, move, collision, dt);
    } else {
        return;
    }
    if (runtime::startThrownFall(air, position[1])) {
        levelCharacters->pauseClip(name, true);
        std::cout << "character " << name << ": thrown fall started at update " << updatesForLog << '\n';
    }
    if (moved.fallStateRequest && wasOnGround)
        std::cout << "character " << name << ": state 5 FALL asked by the fall event, not ported (SCRIPT-S346) at update " << updatesForLog << '\n';
    if (moved.landed && runtime::fallArrestHitOnLanding(engagedTable.autoArrested(name), state)) {
        characterTakedowns[name].states.request(runtime::kFallArrestHitState);
        std::cout << "character " << name << ": lands auto arrested, state " << runtime::kFallArrestHitState << " requested at update " << updatesForLog << '\n';
    }
    engagedTable.setOnGround(name, air.onGround);
    node[12] = position[0];
    node[13] = position[1];
    node[14] = position[2];
    levelCharacters->place(name, node);
}

// The end of the takedown enter 0x28cbf8 (the gun HIT states that start a fall in the air are not ported, COMBAT-7).
void LevelSession::enterTakedownAir(const std::string& name, std::uint16_t state) {
    if (runtime::takedownMover(state) != runtime::TakedownMover::Takedown) return;
    const auto found = characterAir.find(name);
    if (found != characterAir.end()) runtime::takedownEnterAir(found->second, false);
}

}

#include "game_level.hpp"

namespace game_level {
namespace runtime = chase::runtime;


void LevelSession::applyScreenShake(float dt) {
    runtime::DirectorCamera::Shake request;
    if (levelScripts.director().takeShake(request))
        screenShake.start(unsigned(int(request.amount)), unsigned(int(request.amount)), unsigned(int(request.amount)), request.damp, false, request.seconds);
    float offset[3];
    screenShake.update(dt, breakRandomStandIn, offset);
    follow->setEyeShake(offset);
}

}

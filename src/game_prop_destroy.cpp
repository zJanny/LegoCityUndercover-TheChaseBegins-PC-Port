#include "game_level.hpp"

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::propDestroyMessage(const std::string& lowerInstance) {
    if (const auto data = propSmasher.destroyData(lowerInstance)) propDestroyedEffects(*data);
}

}

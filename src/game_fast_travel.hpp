#pragma once
// LevelSession glue of the fast travel pads (runtime/fast_travel_pad.hpp): the player's states 0x180 / 0x181, the Helicopter models.
#include "runtime/fast_travel_pad.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace game_fast_travel {

struct Helicopter {
    std::size_t pad = 0;
    std::string key;
    std::size_t sceneIndex = 0;
    std::optional<chase::runtime::FastTravelHelicopterModel> model;
};

struct State {
    std::vector<Helicopter> helicopters;
    bool touching = false;
};

}

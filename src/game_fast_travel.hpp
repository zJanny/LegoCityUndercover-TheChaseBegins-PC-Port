#pragma once
// LevelSession glue of the fast travel pads (runtime/fast_travel_pad.hpp): the player's states 0x180 / 0x181, the Helicopter models, and of the
// map buttons of the bottom screen (runtime/map_buttons.hpp) whose fast travel map the pad opens.
#include "runtime/fast_travel_pad.hpp"
#include "runtime/map_buttons.hpp"

#include <cstddef>
#include <cstdint>
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
    std::optional<chase::runtime::MapButtons> mapButtons;
    chase::runtime::TouchPhase touchPhase = chase::runtime::TouchPhase::None;
    std::uint16_t storyProgress = 0;
};

}

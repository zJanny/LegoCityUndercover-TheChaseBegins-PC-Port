#pragma once
#include "qol/game_port.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace chase::runtime {

struct QolGameSetup {
    std::string loadFile;
    std::string saveFile;
    int language = 1;
    unsigned soundVolume = 10;
    unsigned musicVolume = 8;
    std::function<void(std::optional<float>, std::optional<float>)> setGains;
};

std::function<std::unique_ptr<qol::GamePort>()> qolGamePortFactory(QolGameSetup setup);

}

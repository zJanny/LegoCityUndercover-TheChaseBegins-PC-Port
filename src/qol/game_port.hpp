#pragma once

#include "qol/game_tab.hpp"

namespace qol {

class GamePort : public GameOptions {
public:
    virtual void applied() = 0;
    virtual void left() = 0;
};

}

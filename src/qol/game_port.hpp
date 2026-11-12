#pragma once

#include "qol/game_tab.hpp"

namespace qol {

class GamePort : public GameOptions {
public:
    virtual int startLanguage() const = 0;
    virtual void chooseLanguage(int language) = 0;
    virtual void applied() = 0;
    virtual void left() = 0;
};

}

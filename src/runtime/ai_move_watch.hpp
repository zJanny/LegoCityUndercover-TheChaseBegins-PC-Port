#pragma once

#include <optional>
#include <string>

namespace chase::runtime {

// Reports where a scripted AI move of the player (AIMove, AISuperJump) starts and ends, for the comparison with the original's
// per-update traces of the end of mission 1-3 (docs/re/pushables.md).
class AiMoveWatch {
public:
    std::optional<std::string> observe(bool moving, const float position[3], unsigned update);

private:
    bool moving_ = false;
    static constexpr unsigned kSettleUpdates = 60;
    unsigned startUpdate_ = 0, endUpdate_ = 0;
};

}

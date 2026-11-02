#include "runtime/ai_move_watch.hpp"

#include <sstream>

namespace chase::runtime {

std::optional<std::string> AiMoveWatch::observe(bool moving, const float position[3], unsigned update) {
    if (moving == moving_) {
        if (moving || endUpdate_ == 0 || update != endUpdate_ + kSettleUpdates) return std::nullopt;
        std::ostringstream settled;
        settled << "player AI move: " << kSettleUpdates << " updates after the end the player is at " << position[0] << ' ' << position[1] << ' ' << position[2];
        endUpdate_ = 0;
        return settled.str();
    }
    moving_ = moving;
    std::ostringstream line;
    if (moving) {
        startUpdate_ = update;
        line << "player AI move: started at update " << update << " from " << position[0] << ' ' << position[1] << ' ' << position[2];
    } else {
        endUpdate_ = update;
        line << "player AI move: ended at update " << update << " at " << position[0] << ' ' << position[1] << ' ' << position[2] << ", " << update - startUpdate_ << " updates";
    }
    return line.str();
}

}

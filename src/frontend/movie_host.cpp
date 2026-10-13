#include "frontend/movie_host.hpp"
#include <fstream>

namespace chase::frontend {

bool MoviePictureHost::open(const std::string& name, const std::string& languageSuffix) {
    player_.reset();
    for (const std::string& file : {name + "_" + languageSuffix, name}) {
        const std::string path = directory_ + "/" + file + ".moflex";
        if (!std::ifstream(path).good()) continue;
        player_ = std::make_unique<movie::SteppedPlayer>(path);
        image_ = assets::Image();
        image_.width = player_->width();
        image_.height = player_->height();
        seconds_ = 0;
        return true;
    }
    return false;
}

bool MoviePictureHost::advance(double dt) {
    if (!player_ || !player_->advance(dt)) return false;
    image_.rgba = player_->rgba();
    seconds_ = player_->frameTime();
    return true;
}

std::vector<std::uint8_t> MoviePictureHost::takeAudio() {
    return player_ ? player_->takeAudio() : std::vector<std::uint8_t>();
}

}

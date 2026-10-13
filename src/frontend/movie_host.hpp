#pragma once
#include "frontend/frontend.hpp"
#include "movie/movie.hpp"
#include <memory>
#include <string>

namespace chase::frontend {

class MoviePictureHost : public PictureHost {
public:
    explicit MoviePictureHost(std::string cutsceneDirectory) : directory_(std::move(cutsceneDirectory)) {}
    bool open(const std::string& name, const std::string& languageSuffix) override;
    bool advance(double dt) override;
    double frameSeconds() const override { return seconds_; }
    const assets::Image& frame() const override { return image_; }
    std::vector<std::uint8_t> takeAudio() override;

private:
    std::string directory_;
    std::unique_ptr<movie::SteppedPlayer> player_;
    assets::Image image_;
    double seconds_ = 0;
};

}

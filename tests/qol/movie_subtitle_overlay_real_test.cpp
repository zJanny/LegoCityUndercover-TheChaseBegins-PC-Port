// Spec: docs/design/qol-screen-layout.md §2026-10-30T14:07:44, answer "Bubble over picture, no panel" (user, 2026-11-02): the subtitle
// bubble of a movie keeps the original art, font and wrap and is drawn over the lower part of the top picture with Improved only.
// Checked by 20261102-qolmenu-C on 2026-11-02 on the real data: the intro movie of the front end (made-up picture host, real subtitle
// file) has a cue at 20 s; on the picture the bubble changes only pixels of its box, shifted by the placement of QOLBOT-30 (open for the user), and its
// pixels equal the original bubble drawn on a transparent bottom canvas (columns 2..317, where the 320 canvas does not clip it).
// Exit code 77 when the game data is missing.
#include "frontend/frontend.hpp"
#include "qol/movie_subtitle_overlay.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace {

namespace fe = chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

class StillMovie : public fe::PictureHost {
public:
    bool open(const std::string&, const std::string&) override {
        clock_ = 0;
        return true;
    }
    bool advance(double dt) override {
        clock_ += dt;
        return clock_ < 60;
    }
    double frameSeconds() const override { return clock_; }
    const chase::assets::Image& frame() const override { return frame_; }
    std::vector<std::uint8_t> takeAudio() override { return {}; }

private:
    double clock_ = 0;
    chase::assets::Image frame_;
};

qol::Settings withMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    return settings;
}

fe::Canvas grey() {
    fe::Canvas canvas(fe::kTopWidth, fe::kScreenHeight);
    canvas.clear(90, 90, 90);
    return canvas;
}

std::vector<std::string> introCue(fe::Resources& resources, StillMovie& movie) {
    fe::FlowConfig config;
    fe::FrontEnd front(resources, &movie, config);
    front.flow().start(fe::Screen::IntroMovie);
    for (int update = 0; update < 30 * 20; ++update) front.update(1.0f / 30, {});
    require(front.flow().screen() == fe::Screen::IntroMovie && front.flow().movieShowing(), "the intro movie plays");
    const auto lines = front.subtitleLines();
    require(!lines.empty(), "the intro movie has a cue at 20 s");
    return lines;
}

void originalAndCustomUnchanged(fe::Resources& resources, const std::vector<std::string>& lines) {
    for (qol::Master master : {qol::Master::Original, qol::Master::Custom}) {
        fe::Canvas picture = grey();
        require(!qol::drawMovieSubtitlesFor(withMaster(master), picture, resources, lines) && picture.rgba() == grey().rgba(),
                "the picture is untouched while the bottom screen shows");
    }
}

void noCueNoBubble(fe::Resources& resources) {
    fe::Canvas picture = grey();
    require(!qol::drawMovieSubtitlesFor(withMaster(qol::Master::Improved), picture, resources, {}) && picture.rgba() == grey().rgba(),
            "no cue: the picture is untouched");
}

void bubbleOverPicture(fe::Resources& resources, const std::vector<std::string>& lines) {
    const qol::Settings improved = withMaster(qol::Master::Improved);
    const fe::IntRect box = fe::subtitleBoxRect(resources, lines);
    const auto offset = qol::movieBubbleOffset(improved, box, fe::kTopWidth, fe::kScreenHeight);
    require(offset.has_value(), "a cue has a bubble");
    if (!offset) return;
    fe::Canvas picture = grey();
    require(qol::drawMovieSubtitlesFor(improved, picture, resources, lines), "Improved draws the bubble");
    const fe::Canvas before = grey();
    const int left = box.x + offset->x, top = box.y + offset->y;
    require(top + box.height == fe::kScreenHeight - box.y, "the bubble ends as far above the bottom edge as the original below the top");
    bool changedInside = false, changedOutside = false;
    for (int y = 0; y < fe::kScreenHeight; ++y)
        for (int x = 0; x < fe::kTopWidth; ++x) {
            const bool changed = !std::equal(picture.pixel(x, y), picture.pixel(x, y) + 4, before.pixel(x, y));
            const bool inside = x >= left && x < left + box.width && y >= top && y < top + box.height;
            (inside ? changedInside : changedOutside) |= changed;
        }
    require(changedInside && !changedOutside, "only the bubble's box changes on the picture");

    fe::Canvas original(fe::kBottomWidth, fe::kScreenHeight), shifted(fe::kTopWidth, fe::kScreenHeight);
    original.clearTransparent();
    shifted.clearTransparent();
    fe::drawSubtitleBox(original, resources, lines, 0, 0);
    fe::drawSubtitleBox(shifted, resources, lines, offset->x, offset->y);
    bool same = true;
    for (int y = box.y; y < box.y + box.height; ++y)
        for (int x = 2; x < fe::kBottomWidth - 2; ++x)
            same &= std::equal(original.pixel(x, y), original.pixel(x, y) + 4, shifted.pixel(x + offset->x, y + offset->y));
    require(same, "the bubble keeps the original art, font and wrap");
}

}

int main(int argc, char** argv) {
    if (argc < 3) return 2;
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) return 77;
    chase::assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    fe::Resources resources(archive);
    StillMovie movie;
    const auto lines = introCue(resources, movie);
    originalAndCustomUnchanged(resources, lines);
    noCueNoBubble(resources);
    if (!lines.empty()) bubbleOverPicture(resources, lines);
    if (failures) return 1;
    std::cout << "qol-movie-subtitle-overlay-real-data ok\n";
    return 0;
}

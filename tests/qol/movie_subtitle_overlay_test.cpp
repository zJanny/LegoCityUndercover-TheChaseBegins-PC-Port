// Spec: docs/design/qol-screen-layout.md §2026-10-30T14:07:44, answer "Bubble over picture, no panel" (user, 2026-11-02): with Improved
// no bottom panel shows while a movie plays and the subtitle bubble is drawn over the lower part of the fitted top picture; other pages
// keep the panel; Original and Custom are unchanged. Values computed by hand by 20261102-qolmenu-C on 2026-11-02 from that rule and the
// placement of QOLBOT-30, open for the user (the bubble centred like the bottom screen under the top one, (400 - 320) / 2 = 40, and as far above the
// picture's bottom edge as the original box is below the bottom screen's top edge); the one-line box (-2, 10, 324, 56) is the box of
// fe::subtitleBoxRect for one line (kBubbleX 6 - margin 8, top 10, width 308 + 16, height 2 * (12 + 16)).
#include "qol/movie_subtitle_overlay.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

qol::Settings withMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    return settings;
}

void originalAndCustomUnchanged() {
    for (qol::Master master : {qol::Master::Original, qol::Master::Custom}) {
        const qol::Settings settings = withMaster(master);
        require(!qol::movieBubbleOffset(settings, {-2, 10, 324, 56}, 400, 240), "no bubble on the picture while the bottom screen shows");
        for (bool movie : {false, true})
            require(!qol::frontMoviePanelsFor(settings, movie, 1920, 1080, 400, 240), "the presenter shows both screens as before");
    }
}

void movieHasNoPanel() {
    const qol::Settings improved = withMaster(qol::Master::Improved);
    const auto movie = qol::frontMoviePanelsFor(improved, true, 1920, 1080, 400, 240);
    require(movie && near(movie->top.x, 60) && near(movie->top.y, 0) && near(movie->top.width, 1800) && near(movie->top.height, 1080),
            "the movie is the fitted top picture");
    require(movie && movie->panel.width == 0 && movie->panel.height == 0, "no panel while a movie plays");
    const auto page = qol::frontMoviePanelsFor(improved, false, 1920, 1080, 400, 240);
    const auto panels = qol::frontPanelsFor(improved, 1920, 1080, 400, 240);
    require(page && panels && near(page->panel.x, panels->panel.x) && near(page->panel.width, panels->panel.width) &&
                near(page->panel.height, panels->panel.height),
            "other pages keep the centred panel");
    require(!qol::frontMoviePanelsFor(improved, true, 0, 1080, 400, 240), "an empty window has no picture");
}

void bubblePlacement() {
    const qol::Settings improved = withMaster(qol::Master::Improved);
    auto offset = qol::movieBubbleOffset(improved, {-2, 10, 324, 56}, 400, 240);
    require(offset && offset->x == 40 && offset->y == 164, "one line: box at (38, 174), 10 above the picture's bottom edge");
    offset = qol::movieBubbleOffset(improved, {-2, 10, 324, 80}, 400, 240);
    require(offset && offset->x == 40 && offset->y == 140, "two lines: the box grows upwards, bottom still at 230");
    require(!qol::movieBubbleOffset(improved, {0, 0, 0, 0}, 400, 240), "no cue: no bubble");
}

}

int main() {
    originalAndCustomUnchanged();
    movieHasNoPanel();
    bubblePlacement();
    if (failures) return 1;
    std::cout << "qol-movie-subtitle-overlay ok\n";
    return 0;
}

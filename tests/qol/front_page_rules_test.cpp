// Spec: docs/design/qol-screen-layout.md §2026-10-30T14:07:44 (QOLBOT-21: with Improved every front end page with bottom-screen content
// shows its 320x240 page as the centred panel; logos and movies show no panel; the boot and "Saving Game" notices show the alert panel).
// Page classes pinned from that page list by 20261102-qolmore-B on 2026-11-02: panel for Boot, Language, Prompt, Autosave, SlotSelect
// (the slot page and its delete confirm), SavingGame, MissionCard; no panel for Logos, FusionMovie, IntroMovie, Done and while the screen
// is black between pages. Loading is not in the spec list; its entry (panel, as built before) keeps the panel with a small spinner (user, 2026-11-02: "Integrate it nicely, so it looks like it belongs there. Maybe a small spining icon somewhere"). Spinner box 268,198 size 28 proposed by 20261102-qolmore-B inside the tip box (kTipX 8..312, kTipY 78..230 of src/frontend/pages.cpp) and confirmed by the user ("Looks good", 2026-11-02).
#include "qol/front_page_rules.hpp"

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

qol::Settings withMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    return settings;
}

void pageClasses() {
    for (fe::Screen screen : {fe::Screen::Boot, fe::Screen::Language, fe::Screen::Prompt, fe::Screen::Autosave, fe::Screen::SlotSelect,
             fe::Screen::SavingGame, fe::Screen::MissionCard})
        require(qol::frontPageView(screen, false) == qol::PageView::Panel, "a page with bottom content shows the panel");
    for (fe::Screen screen : {fe::Screen::Logos, fe::Screen::FusionMovie, fe::Screen::IntroMovie, fe::Screen::Done})
        require(qol::frontPageView(screen, false) == qol::PageView::PictureOnly, "logos and movies show no panel");
    require(qol::frontPageView(fe::Screen::Loading, false) == qol::PageView::Panel, "loading keeps the panel");
}

void blackoutHidesThePanel() {
    for (fe::Screen screen : {fe::Screen::Boot, fe::Screen::Language, fe::Screen::SlotSelect, fe::Screen::MissionCard})
        require(qol::frontPageView(screen, true) == qol::PageView::PictureOnly, "the panel disappears while the screen is black");
}

void panelsFollowThePage() {
    const qol::Settings improved = withMaster(qol::Master::Improved);
    auto p = qol::frontPagePanelsFor(improved, fe::Screen::Language, false, 1920, 1080, 400, 240);
    require(p && p->panel.scale > 0 && p->panel.width == 1440 && p->top.width == 1800, "a panel page keeps both boxes");
    p = qol::frontPagePanelsFor(improved, fe::Screen::Logos, false, 1920, 1080, 400, 240);
    require(p && p->panel.scale == 0 && p->panel.width == 0 && p->top.width == 1800 && p->top.height == 1080,
        "a logo page keeps the fitted picture and has an empty panel");
    p = qol::frontPagePanelsFor(improved, fe::Screen::SlotSelect, true, 1920, 1080, 400, 240);
    require(p && p->panel.width == 0, "a black screen has an empty panel");
}

void loadingSpinner() {
    const auto place = qol::loadingSpinnerFor(withMaster(qol::Master::Improved), fe::Screen::Loading, false);
    require(place && place->x == 268 && place->y == 198 && place->size == 28, "Improved: a 28 pixel spinner at the lower right of the tip box");
    require(place && place->x + place->size <= 312 && place->y + place->size <= 230, "the spinner lies inside the tip box (x 8..312, y 78..230)");
    require(!qol::loadingSpinnerFor(withMaster(qol::Master::Improved), fe::Screen::Loading, true), "no spinner while black");
    require(!qol::loadingSpinnerFor(withMaster(qol::Master::Improved), fe::Screen::SlotSelect, false), "no spinner on other pages");
    for (qol::Master master : {qol::Master::Original, qol::Master::Custom})
        require(!qol::loadingSpinnerFor(withMaster(master), fe::Screen::Loading, false), "no extra spinner while the bottom screen is shown");
}

void originalAndCustomUnchanged() {
    for (qol::Master master : {qol::Master::Original, qol::Master::Custom})
        for (fe::Screen screen : {fe::Screen::Language, fe::Screen::Logos, fe::Screen::MissionCard})
            require(!qol::frontPagePanelsFor(withMaster(master), screen, false, 1920, 1080, 400, 240), "no panels while the bottom screen is shown");
}

}

int main() {
    originalAndCustomUnchanged();
    pageClasses();
    blackoutHidesThePanel();
    panelsFollowThePage();
    loadingSpinner();
    if (failures) return 1;
    std::cout << "qol-front-page-rules OK\n";
    return 0;
}

#include "qol/front_page_rules.hpp"

namespace qol {

namespace fe = chase::frontend;

PageView frontPageView(fe::Screen screen, bool blackout) {
    if (blackout) return PageView::PictureOnly;
    switch (screen) {
    case fe::Screen::Logos:
    case fe::Screen::FusionMovie:
    case fe::Screen::IntroMovie:
    case fe::Screen::Done:
        return PageView::PictureOnly;
    case fe::Screen::Boot:
    case fe::Screen::Language:
    case fe::Screen::Prompt:
    case fe::Screen::Autosave:
    case fe::Screen::SlotSelect:
    case fe::Screen::SavingGame:
    case fe::Screen::Loading:
    case fe::Screen::MissionCard:
        return PageView::Panel;
    }
    return PageView::Panel;
}

std::optional<FrontPanels> frontPagePanelsFor(const Settings& settings, fe::Screen screen, bool blackout, int windowWidth, int windowHeight,
    int topWidth, int screenHeight) {
    auto panels = frontPanelsFor(settings, windowWidth, windowHeight, topWidth, screenHeight);
    if (panels && frontPageView(screen, blackout) == PageView::PictureOnly) panels->panel = {0, 0, 0, 0, 0};
    return panels;
}

std::optional<SpinnerPlace> loadingSpinnerFor(const Settings& settings, fe::Screen screen, bool blackout) {
    if (!bottomScreenHidden(settings) || blackout || screen != fe::Screen::Loading) return std::nullopt;
    return SpinnerPlace{268, 198, 28};
}

}

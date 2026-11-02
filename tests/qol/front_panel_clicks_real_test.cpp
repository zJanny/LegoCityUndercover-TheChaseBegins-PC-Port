// Row and button clicks on the front end panel (QOLBOT-22), pinned to docs/design/qol-screen-layout.md §2026-10-30T14:07:44: with Improved a
// click on a list row selects it and presses A, a click on a button picture (A, B, Y) is that button, a click elsewhere in the panel does
// nothing; Original and Custom keep today's behaviour (the left click is the bound A button). Original mode first.
// Geometry (bottom-canvas pixels) written by 20261102-qolctl2-D on 2026-11-02 from the layout files (chase-layoutdump --tracks) by adding
// the element positions down the tree, the way layoutElementBox does:
//  - page_alerttext: Option1 (166.023, 208.339) holds ButtonIcon1 at (122, -1) 32x32; Option2 (6.023, 208.339) holds ButtonIcon2 at (-6, -1).
//  - shortcuts drawn at (-80, -16): SelectButton_3DS (362, 233) holds Button at (-6, -8) 32x32, ExtraButton_3DS (85, 233) holds Button at (6, -8).
//  - page_selectslot: ListItem1 (81, 97), ListItem2 (81, 151), each holding Button at (-32, 0) 256x27; the TransitionOn x track is -280 until
//    0.3 s (ListItem1 reaches 0 at 0.5 s, ListItem2 at 0.6 s).
//  - language list rows: centre y 20 + 24 row (frontend.cpp drawLanguage), band height 24, full page width.
// Window 1920x1080 with Improved puts the panel at x 240, y 0 with scale 4.5 (qol-front-panel). Needs the extracted RomFS; exits with 77
// without it.
#include "qol/active_settings.hpp"
#include "qol/front_panel_clicks_sdl.hpp"

#include "frontend/frontend.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
namespace fe = chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

constexpr int width = 1920, height = 1080;

bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

bool same(const fe::TargetBox& box, float x, float y, float w, float h) {
    return near(box.x, x) && near(box.y, y) && near(box.width, w) && near(box.height, h);
}

void setMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    qol::setActiveSettings(settings);
}

SDL_Event click(Uint32 type, int button, float pageX, float pageY) {
    SDL_Event event{};
    event.type = type;
    event.button.button = Uint8(button);
    event.button.x = int(std::lround(240 + 4.5f * pageX));
    event.button.y = int(std::lround(4.5f * pageY));
    return event;
}

SDL_Event leftClick(float pageX, float pageY) { return click(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, pageX, pageY); }

fe::FlowConfig twoUsedSlots() {
    fe::FlowConfig config;
    config.slots = {{true, "05:48", "0.2%", 1}, {false, {}, {}, 0}};
    return config;
}

bool hasButton(const fe::PageTargets& targets, std::uint32_t button, float x, float y) {
    for (const fe::ButtonTarget& target : targets.buttons)
        if (target.button == button && same(target.box, x, y, 32, 32)) return true;
    return false;
}

void geometry(fe::Resources& resources) {
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());

    front.flow().start(fe::Screen::Language);
    fe::PageTargets targets = front.targets();
    require(targets.rows.size() == 9 && targets.buttons.size() == 1, "the language page has 9 rows and the A picture");
    require(hasButton(targets, fe::kButtonA, 276, 209), "the language page's A picture is the shortcut bar's Select button");
    require(same(targets.rows[0], 0, 8, 320, 24) && same(targets.rows[3], 0, 80, 320, 24) && same(targets.rows[8], 0, 200, 320, 24),
        "language rows are full-width 24-pixel bands around the text lines at y 20 + 24 row");

    front.flow().start(fe::Screen::Prompt);
    targets = front.targets();
    require(targets.rows.empty() && targets.buttons.size() == 2, "the StreetPass question has Yes (A) and No (B) and no rows");
    require(hasButton(targets, fe::kButtonA, 288.023f, 207.339f) && hasButton(targets, fe::kButtonB, 0.023f, 207.339f),
        "the alert page's A icon is at the right and its B icon at the left");
    front.flow().update(0.03f, fe::FrontInput{fe::kButtonB});
    targets = front.targets();
    require(targets.buttons.size() == 1 && hasButton(targets, fe::kButtonA, 288.023f, 207.339f), "the HOME notice has only the A icon");

    front.flow().start(fe::Screen::Autosave);
    targets = front.targets();
    require(targets.rows.empty() && targets.buttons.size() == 1 && hasButton(targets, fe::kButtonA, 288.023f, 207.339f),
        "the autosave notice has the OK (A) icon");

    front.flow().start(fe::Screen::SlotSelect);
    targets = front.targets();
    require(targets.rows.size() == 2 && same(targets.rows[0], -231, 97, 256, 27) && same(targets.rows[1], -231, 151, 256, 27),
        "at the start of the slot page both rows are 280 pixels to the left");
    require(hasButton(targets, fe::kButtonA, 276, 209) && hasButton(targets, fe::kButtonY, 11, 209) && targets.buttons.size() == 2,
        "with a used slot selected the page shows the A and the Y (Delete) pictures");
    for (int i = 0; i < 25; ++i) front.flow().update(0.04f, {});
    targets = front.targets();
    require(same(targets.rows[0], 49, 97, 256, 27) && same(targets.rows[1], 49, 151, 256, 27), "after 1 s the rows rest in the layout");
    front.flow().update(0.03f, fe::FrontInput{fe::kButtonDown});
    targets = front.targets();
    require(targets.buttons.size() == 1 && hasButton(targets, fe::kButtonA, 276, 209), "with the empty New Game slot selected there is no Y picture");
    front.flow().update(0.03f, fe::FrontInput{fe::kButtonA});
    require(front.flow().confirming() && front.targets().empty(), "while the choice is confirmed the page offers nothing");

    for (const fe::Screen screen : {fe::Screen::Boot, fe::Screen::Logos, fe::Screen::SavingGame, fe::Screen::Loading, fe::Screen::MissionCard}) {
        front.flow().start(screen);
        require(front.targets().empty(), "pages without rows or buttons offer nothing");
    }
}

void originalAndCustomKeepTheBoundButton(fe::Resources& resources) {
    for (const qol::Master master : {qol::Master::Original, qol::Master::Custom}) {
        setMaster(master);
        fe::FrontEnd front(resources, nullptr, twoUsedSlots());
        front.flow().start(fe::Screen::Language);
        qol::FrontClicks clicks;
        require(!clicks.consume(leftClick(160, 92), width, height, front), "with the bottom screen shown the left click goes to the bound button");
        fe::FrontInput pad;
        clicks.apply(pad);
        require(pad.pressed == 0 && !pad.selectRow, "nothing was queued");
    }
}

void languageClicks(fe::Resources& resources) {
    setMaster(qol::Master::Improved);
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    front.flow().start(fe::Screen::Language);
    qol::FrontClicks clicks;
    require(!clicks.consume(click(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT, 160, 92), width, height, front), "a right click is not a panel click");
    require(!clicks.consume(click(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 160, 92), width, height, front), "a release passes on");
    require(!clicks.consume(SDL_Event{}, width, height, front), "other events pass on");
    require(clicks.consume(leftClick(160, 100), width, height, front), "a left click on a row is taken");
    fe::FrontInput pad;
    clicks.apply(pad);
    require(pad.pressed == fe::kButtonA && pad.selectRow && *pad.selectRow == 3, "a row click on row 4 (y 80-104) selects row 3 and presses A");
    front.update(0.03f, pad);
    require(front.flow().screen() == fe::Screen::Prompt && front.flow().language() == 4, "the front end takes that row's language (German)");
    fe::FrontInput again;
    clicks.apply(again);
    require(again.pressed == 0 && !again.selectRow, "a click is applied once");

    front.flow().start(fe::Screen::Language);
    require(clicks.consume(leftClick(160, 4), width, height, front), "a click above the first row still belongs to the panel");
    fe::FrontInput nothing;
    clicks.apply(nothing);
    require(nothing.pressed == 0 && !nothing.selectRow, "a click on no row does nothing");
    require(clicks.consume(leftClick(290, 215), width, height, front), "a click on the A picture is taken");
    fe::FrontInput a;
    clicks.apply(a);
    require(a.pressed == fe::kButtonA && !a.selectRow, "the A picture presses A over the row band below it");
    SDL_Event outside = leftClick(160, 100);
    outside.button.x = 100;
    require(clicks.consume(outside, width, height, front), "a click in the bar beside the panel is taken too");
    fe::FrontInput bar;
    clicks.apply(bar);
    require(bar.pressed == 0 && !bar.selectRow, "and does nothing");
}

void buttonClicks(fe::Resources& resources) {
    setMaster(qol::Master::Improved);
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    qol::FrontClicks clicks;
    front.flow().start(fe::Screen::Prompt);
    require(clicks.consume(leftClick(16, 223), width, height, front), "a click on the B icon is taken");
    fe::FrontInput pad;
    clicks.apply(pad);
    require(pad.pressed == fe::kButtonB && !pad.selectRow, "the B icon presses B");
    front.update(0.03f, pad);
    require(front.flow().prompt() == fe::Prompt::HomeNotice, "B answers No");

    front.flow().start(fe::Screen::Autosave);
    require(clicks.consume(leftClick(300, 220), width, height, front), "a click on the OK icon is taken");
    fe::FrontInput ok;
    clicks.apply(ok);
    front.update(0.03f, ok);
    require(front.flow().confirming(), "the A icon confirms the autosave notice");
}

void slotClicks(fe::Resources& resources) {
    setMaster(qol::Master::Improved);
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    qol::FrontClicks clicks;
    front.flow().start(fe::Screen::SlotSelect);
    for (int i = 0; i < 25; ++i) front.update(0.04f, {});
    require(clicks.consume(leftClick(100, 165), width, height, front), "a click on slot 2 is taken");
    fe::FrontInput pad;
    clicks.apply(pad);
    require(pad.pressed == fe::kButtonA && pad.selectRow && *pad.selectRow == 1, "it selects row 2 and presses A");
    front.update(0.03f, pad);
    require(front.flow().selectedSlot() == 1 && front.flow().confirming(), "the front end confirms slot 2");

    front.flow().start(fe::Screen::SlotSelect);
    for (int i = 0; i < 25; ++i) front.update(0.04f, {});
    require(clicks.consume(leftClick(20, 220), width, height, front), "a click on the Delete picture is taken");
    fe::FrontInput y;
    clicks.apply(y);
    require(y.pressed == fe::kButtonY && !y.selectRow, "the Y picture presses Y");
    require(clicks.consume(leftClick(160, 135), width, height, front), "a click between the rows is taken");
    fe::FrontInput between;
    clicks.apply(between);
    require(between.pressed == 0 && !between.selectRow, "and does nothing");
    front.update(0.03f, between);
    require(!front.flow().confirming(), "so the page stays");
}

void pagesWithoutTargetsKeepTheBoundButton(fe::Resources& resources) {
    setMaster(qol::Master::Improved);
    fe::FrontEnd front(resources, nullptr, twoUsedSlots());
    qol::FrontClicks clicks;
    front.flow().start(fe::Screen::Logos);
    require(!clicks.consume(leftClick(160, 120), width, height, front), "a page without rows or buttons leaves the click to the bound button");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-front-panel-clicks-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-front-panel-clicks-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        originalAndCustomKeepTheBoundButton(resources);
        geometry(resources);
        languageClicks(resources);
        buttonClicks(resources);
        slotClicks(resources);
        pagesWithoutTargetsKeepTheBoundButton(resources);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "qol-front-panel-clicks-real-data OK\n";
    return 0;
}

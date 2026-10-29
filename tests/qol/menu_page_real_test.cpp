// The settings page drawn from the round menu layout (qol-settings-drawing.md §2026-10-29T12:11:55; the user's answers of 2026-10-29 on
// the name plate and Game tab labels): setup values read from the shipped layouts and game text, the round form scene, every element the
// scene names, and the page drawn into windows of the spec's sizes with the rest black. Provenance of the pinned boxes and texts:
// 20261029-qolmenu2-A, 2026-10-29, chase-layoutdump of sprites/ui_roundaboutmenu/main.bxaml and sprites/blend/ui_frontend/shortcuts.bxaml,
// gametext_uk / systemtext_uk entries of the keys in pause_menu.hpp and options_page.hpp and of the identifiers ON and OFF. Needs the
// extracted RomFS; exits with 77 without it.
#include "qol/menu_page.hpp"

#include "frontend/pages.hpp"
#include "qol/controls_port.hpp"
#include "qol/menu_list_page.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <string>
#include <vector>

namespace {
namespace fe = chase::frontend;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

bool boxIs(const qol::Box& box, float x, float y, float width, float height) {
    return std::fabs(box.x - x) < 1e-3f && std::fabs(box.y - y) < 1e-3f && box.width == width && box.height == height;
}

class FixedOptions : public qol::GameOptions {
public:
    int sfxVolume() const override { return 7; }
    int musicVolume() const override { return 5; }
    bool invertCamera() const override { return true; }
    int language() const override { return 1; }
    void setSfxVolume(int) override {}
    void setMusicVolume(int) override {}
    void setInvertCamera(bool) override {}
    void openLanguages() override {}
};

std::string textOf(const fe::LayoutScene& scene, const std::string& element) {
    const auto found = scene.text.find(element);
    return found == scene.text.end() ? std::string("<none>") : found->second.text;
}

void namesExist(fe::Resources& resources, const fe::LayoutScene& scene, const std::string& what,
    const std::string& layout = fe::kRoundMenuLayout) {
    auto check = [&](const std::string& name) {
        require(fe::layoutElementBox(resources, layout, name).has_value(), what + ": the layout has " + name);
    };
    for (const auto& entry : scene.text) check(entry.first);
    for (const auto& entry : scene.images) check(entry.first);
    for (const auto& entry : scene.opacities) check(entry.first);
    for (const std::string& name : scene.hidden) check(name);
}

bool black(const fe::Canvas& canvas, int x, int y) {
    const std::uint8_t* p = canvas.pixel(x, y);
    return p[0] == 0 && p[1] == 0 && p[2] == 0;
}

int litPixels(const fe::Canvas& canvas, int x0, int y0, int x1, int y1) {
    int lit = 0;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) lit += !black(canvas, x, y);
    return lit;
}

void drawn(fe::Resources& resources, const qol::MenuModel& model, const qol::MenuPageSetup& setup, int width, int height) {
    const std::string size = std::to_string(width) + "x" + std::to_string(height);
    fe::Canvas window(width, height);
    qol::drawMenuPage(window, resources, model, setup, qol::controlsListForm(setup));
    const qol::PageRect page = qol::menuPageRect(width, height);
    const int left = int(std::lround(page.x)), right = int(std::lround(page.x + page.width));
    if (left > 0) require(litPixels(window, 0, 0, left, height) == 0, size + ": the left bar is black");
    if (right < width) require(litPixels(window, right, 0, width, height) == 0, size + ": the right bar is black");
    const int top = int(std::lround(page.y)), bottom = int(std::lround(page.y + page.height));
    if (top > 0) require(litPixels(window, 0, 0, width, top) == 0, size + ": the top bar is black");
    if (bottom < height) require(litPixels(window, 0, bottom, width, height) == 0, size + ": the bottom bar is black");
    const int centreX = int(std::lround(page.x + 159 * page.scale)), centreY = int(std::lround(page.y + 119 * page.scale));
    require(!black(window, centreX, centreY), size + ": the centre slot is drawn");
    require(!black(window, int(page.x + page.width / 2), int(page.y + 2 * page.scale)), size + ": the top bar is drawn");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-menu-page-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-menu-page-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        const qol::MenuPageSetup setup = qol::menuPageSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc");

        require(setup.on == "On" && setup.off == "Off", "gametext ON and OFF are On and Off");
        require(setup.gameLabels[1] == "Music", "the Music label of the Options page");
        require(!setup.gameLabels[0].empty() && !setup.gameLabels[2].empty() && !setup.gameLabels[3].empty(), "every Game label is set");
        require(boxIs(setup.geometry.back, 11, 209, 32, 32), "Back icon box");
        require(boxIs(setup.geometry.select, 276, 209, 32, 32), "Select icon box");
        require(boxIs(setup.geometry.previousTab, 2, 95, 16, 32) && boxIs(setup.geometry.nextTab, 298, 95, 16, 32), "arrow boxes");
        require(boxIs(setup.geometry.header, -70, 31.667f, 256, 32), "subtitle bar box");
        require(setup.geometry.slotRadius == fe::kRoundMenuTouchRadius && setup.geometry.slotCentres[1] == std::array<float, 2>{159, 119},
            "tap circles of the round");

        qol::Settings settings;
        qol::MenuModel model(settings);
        model.open();
        FixedOptions options;
        const fe::LayoutScene game = qol::menuPageScene(model, setup, &options);
        require(textOf(game, "TEXT_Title") == "PC Settings", "title");
        require(textOf(game, "TEXT_Subtitle") == "Game", "subtitle shows the tab");
        require(textOf(game, "PageNumber") == "1/4", "page number shows the tab place");
        require(textOf(game, "TEXT_Icon") == setup.gameLabels[0] + ": 7", "the name plate shows the selected row");
        require(game.images.at("Button_3_Image") == "sprites/ui_statusscreen/options/ui_options_sfx.bpng", "centre picture");
        require(game.images.at("Button_4_Image") == "sprites/ui_statusscreen/options/ui_options_music.bpng", "right picture");
        require(game.images.at("Button_2_Image") == "sprites/ui_statusscreen/options/ui_options_language.bpng", "left picture wraps");
        require(game.opacities.at("Pageslots") == 1, "the page arrows are shown");
        require(qol::rowPlateText(model, 1, setup, &options) == "Music: 5", "music row");
        require(qol::rowPlateText(model, 2, setup, &options) == setup.gameLabels[2] + ": On", "invert camera shows On");
        require(qol::rowPlateText(model, 3, setup, &options) == setup.gameLabels[3], "the Language row shows its label only");
        namesExist(resources, game, "Game tab");

        model.handle({qol::EventKind::TabPrevious});
        const fe::LayoutScene quality = qol::menuPageScene(model, setup);
        require(textOf(quality, "TEXT_Subtitle") == "Quality of life" && textOf(quality, "PageNumber") == "4/4", "Quality of life tab");
        require(textOf(quality, "TEXT_Icon") == "Quality of life: Original", "master row plate");
        require(qol::rowPlateText(model, 1, setup, nullptr) == "Widened view: Off", "feature row plate");
        require(quality.images.at("Button_3_Image") == "sprites/ui_statusscreen/ui_statusscreen_options.bpng", "Options picture");
        namesExist(resources, quality, "Quality of life tab");

        model.handle({qol::EventKind::TabPrevious});
        std::vector<std::string> actions{"Up", "Down", "Walk"};
        std::vector<qol::BindingEntry> defaults{{"Up", "key", "W", "hold"}, {"Down", "key", "S", "hold"}};
        qol::ControlsTab controlsTab(actions, defaults, defaults, [](const std::string&) { return true; });
        qol::ControlsTabPort controlsPort(controlsTab);
        model.setControls(&controlsPort);
        const qol::ListForm form = qol::controlsListForm(setup);
        const fe::LayoutScene controls = qol::pageScene(model, setup, form);
        require(textOf(controls, "PageNumber") == "3/4" && controls.hidden.count("Menu") == 1, "Controls tab uses the list form");
        require(textOf(controls, "TEXT_Subtitle") == "Controls", "Controls subtitle");
        require(!form.overlay(model).has_value(), "no overlay while the list shows");
        require(!qol::pageHitAreas(model, setup, form).empty(), "the list has hit areas");
        namesExist(resources, controls, "Controls list");
        require(setup.yes == "Yes" && setup.no == "No", "gametext Yes and No");

        model.handle({qol::EventKind::Activate});
        const std::optional<fe::LayoutScene> notice = form.overlay(model);
        require(notice.has_value() && textOf(*notice, "Text_Info_NoHeader") == "Press a key or button for Up", "waiting overlay");
        if (notice) namesExist(resources, *notice, "waiting notice", qol::noticePageLayout);
        model.assignInput("key", "X");
        model.handle({qol::EventKind::Activate});
        model.assignInput("key", "Y");
        for (int i = 0; i < 3; ++i) model.handle({qol::EventKind::Down});
        model.handle({qol::EventKind::Activate});
        const std::optional<fe::LayoutScene> question = form.overlay(model);
        require(question.has_value() && textOf(*question, "Text_Option1") == setup.yes && textOf(*question, "Text_Option2") == setup.no,
            "question overlay with the gametext words");
        require(qol::pageHitAreas(model, setup, form).size() == 2, "the question has its two buttons");
        fe::Canvas questionWindow(640, 480), listWindow(640, 480);
        qol::drawMenuPage(questionWindow, resources, model, setup, form);
        model.handle({qol::EventKind::Back});
        require(!form.overlay(model).has_value(), "Back closes the question");
        qol::drawMenuPage(listWindow, resources, model, setup, form);
        require(listWindow.rgba() != questionWindow.rgba(), "the question is drawn over the page");
        namesExist(resources, qol::menuShortcutsScene(setup), "shortcuts", fe::kShortcutsLayout);

        model.handle({qol::EventKind::TabPrevious});
        require(textOf(qol::menuPageScene(model, setup), "TEXT_Icon").rfind("Window mode: ", 0) == 0, "Display row plate");

        model.handle({qol::EventKind::TabPrevious});
        drawn(resources, model, setup, 400, 240);
        drawn(resources, model, setup, 1920, 1080);
        drawn(resources, model, setup, 640, 960);
    } catch (const std::exception& error) {
        std::cerr << "qol-menu-page-real-test: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "qol-menu-page-real-test: ok\n";
    return 0;
}

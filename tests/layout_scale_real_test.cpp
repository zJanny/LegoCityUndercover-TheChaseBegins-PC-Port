// Layout drawing at a scale (HUD-95) and the element scale set by code (HUD-96: 0.5 halves the 47 pixel title; recorded by 20261030-qolpanels-D, 2026-10-30, from this test's own run): element opacities set by code, text drawn through the node scale, element boxes inside references and
// the shortcut bar scene, each left unchanged while unused. Provenance of the pinned boxes: 20261029-qolmenu2-A, 2026-10-29, chase-layoutdump
// of sprites/ui_roundaboutmenu/main.bxaml (Pageslots at 59,163 holds Left_Arrow at -57,-68 and Right_Arrow at 239,-68, 16x32) and of
// sprites/blend/ui_frontend/shortcuts.bxaml (BackButton_3DS at 85,233 references backbutton.bxaml, whose Button is at 6,-8, 32x32). Needs
// the extracted RomFS; exits with 77 without it.
#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/pages.hpp"
#include "frontend/resources.hpp"
#include "frontend/round_menu.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace chase::frontend;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool boxIs(const std::optional<std::array<float, 4>>& box, float x, float y, float width, float height) {
    return box && (*box)[0] == x && (*box)[1] == y && (*box)[2] == width && (*box)[3] == height;
}

int litWidth(const Canvas& canvas) {
    int first = -1, last = -1;
    for (int x = 0; x < canvas.width(); ++x)
        for (int y = 0; y < canvas.height(); ++y) {
            const std::uint8_t* p = canvas.pixel(x, y);
            if (p[0] >= 200 && p[1] >= 200 && p[2] >= 200) {
                if (first < 0) first = x;
                last = x;
                break;
            }
        }
    return first < 0 ? 0 : last - first + 1;
}

Canvas drawTitle(Resources& resources, float scale, bool scaleText, float elementScale = 1) {
    LayoutScene scene;
    scene.hidden = {"Backgrounds", "Menu", "Item_Name", "Menu_Strip", "Menu_Bars", "Menu_List", "Subtitle", "Bottom_Left_BTN",
                    "Bottom_Right_BTN", "backButton", "selectButton", "itemSmallGroup", "codegrp", "Pageslots", "Particles", "Fade",
                    "images/UI_Barstrip_Tile.png", "images/UI_Barstrip.png"};
    TextSlot title;
    title.text = "Options";
    title.align = Align::Left;
    scene.text["TEXT_Title"] = title;
    scene.scaleText = scaleText;
    if (elementScale != 1) scene.scales["TEXT_Title"] = {elementScale, elementScale};
    Canvas canvas(int(320 * scale), int(40 * scale));
    canvas.clear(0, 0, 0);
    drawLayout(canvas, resources, kRoundMenuLayout, scene, 0, 0, scale);
    return canvas;
}

std::uint8_t arrowAlpha(Resources& resources, bool shown) {
    LayoutScene scene;
    scene.storyboard = "TransitionOn_Round";
    scene.time = 0.5f;
    if (shown) scene.opacities["Pageslots"] = 1;
    Canvas canvas(320, 240);
    canvas.clear(0, 0, 0);
    Canvas plain(320, 240);
    plain.clear(0, 0, 0);
    LayoutScene without = scene;
    without.hidden.insert("Left_Arrow");
    drawLayout(canvas, resources, kRoundMenuLayout, scene, 0, 0);
    drawLayout(plain, resources, kRoundMenuLayout, without, 0, 0);
    int differing = 0;
    for (int y = 95; y < 127; ++y)
        for (int x = 2; x < 18; ++x)
            for (int c = 0; c < 3; ++c) differing += canvas.pixel(x, y)[c] != plain.pixel(x, y)[c];
    return std::uint8_t(differing > 0);
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: layout-scale-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "layout-scale-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);

        require(boxIs(layoutElementBox(resources, kRoundMenuLayout, "Left_Arrow"), 2, 95, 16, 32), "Left_Arrow box");
        require(boxIs(layoutElementBox(resources, kRoundMenuLayout, "Right_Arrow"), 298, 95, 16, 32), "Right_Arrow box");
        require(boxIs(layoutElementBox(resources, kShortcutsLayout, "BackButton_3DS/Button"), 91, 225, 32, 32), "Button inside the reference");
        require(!layoutElementBox(resources, kRoundMenuLayout, "No_Such_Element"), "no box without the element");
        require(!layoutElementBox(resources, kShortcutsLayout, "BackButton_3DS/No_Such_Element"), "no box without the child");

        require(arrowAlpha(resources, false) == 0, "TransitionOn_Round keeps the page arrows at opacity 0");
        require(arrowAlpha(resources, true) == 1, "an opacity set by code shows them");

        const int unscaled = litWidth(drawTitle(resources, 1, false));
        const int plainAtTwo = litWidth(drawTitle(resources, 2, false));
        const int scaledAtTwo = litWidth(drawTitle(resources, 2, true));
        const int scaledAtOne = litWidth(drawTitle(resources, 1, true));
        std::cout << "title widths " << unscaled << " " << plainAtTwo << " " << scaledAtTwo << " " << scaledAtOne << '\n';
        require(unscaled == 47, "the title is 47 lit pixels wide at scale 1 (layout-text-real-data)");
        require(plainAtTwo == unscaled, "without scaleText the text keeps glyph scale 1");
        require(scaledAtOne == unscaled, "scaleText at scale 1 draws the same width");
        require(scaledAtTwo >= 2 * unscaled - 2 && scaledAtTwo <= 2 * unscaled + 2, "scaleText at scale 2 doubles the width");

        const int halfSized = litWidth(drawTitle(resources, 1, true, 0.5f));
        const int halfPlain = litWidth(drawTitle(resources, 1, false, 0.5f));
        const int unitScaled = litWidth(drawTitle(resources, 1, true, 1.0f));
        std::cout << "element scale widths " << halfSized << " " << halfPlain << " " << unitScaled << '\n';
        require(halfSized >= unscaled / 2 - 2 && halfSized <= unscaled / 2 + 2, "an element scale of 0.5 halves the text width with scaleText");
        require(halfPlain == unscaled, "an element scale leaves the text at glyph scale 1 without scaleText");
        require(unitScaled == unscaled, "an element scale of 1 changes nothing");

        ShortcutView back{"Back", "TransitionOn", 1.1f, true}, select{"Select", "TransitionOn", 1.1f, true};
        Canvas direct(320, 240), fromScene(320, 240);
        direct.clear(0, 0, 0);
        fromScene.clear(0, 0, 0);
        drawBackSelectShortcuts(direct, resources, back, select);
        drawLayout(fromScene, resources, kShortcutsLayout, backSelectShortcutsScene(back, select), kShortcutsX, kShortcutsY);
        require(direct.rgba() == fromScene.rgba(), "the shortcut bar scene draws what drawBackSelectShortcuts draws");
    } catch (const std::exception& error) {
        std::cerr << "layout-scale-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "layout-scale-real-test: ok\n";
    return 0;
}

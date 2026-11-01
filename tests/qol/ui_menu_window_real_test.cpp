// The new settings menu opened through FrontMenu over the real front-end resources and drawn by UiDraw into a hidden GL 4.6 window
// at 1920x1080 and 3840x2160. Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (full-window overlay, tabs across
// the top, hint bar at the bottom, sized from the window height) with answer 10 (rows 5.5 percent of the height) and answer 29 (tab
// bar and hint bar opaque in the panel colour, picture dim 0.72 over the black window of stand-in QOLSET-S7). The language names are
// the gametext names of the nine languages (src/runtime/languages_page.hpp). Written by 20261101-qolui3-F on 2026-11-01. Needs the
// extracted RomFS and a GL 4.6 context; exits with 77 without them.
#include "qol/menu_window_sdl.hpp"

#include "frontend/frontend.hpp"
#include "render/window.hpp"
#include "runtime/languages_page.hpp"

#include <SDL.h>
#include <cmath>
#include <cstdlib>
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

struct FakeGame final : qol::GamePort {
    int sfxVolume() const override { return 10; }
    int musicVolume() const override { return 8; }
    bool invertCamera() const override { return false; }
    int language() const override { return 1; }
    void setSfxVolume(int) override {}
    void setMusicVolume(int) override {}
    void setInvertCamera(bool) override {}
    void openLanguages() override {}
    int startLanguage() const override { return 1; }
    void applied() override {}
    void left() override {}
};

qol::StoresPorts ports() {
    qol::StoresPorts out;
    out.settingsPath = (std::filesystem::temp_directory_path() / "chase-qol-ui-menu-window-never-written.cfg").string();
    out.modes = {{1280, 720}, {1920, 1080}, {3840, 2160}};
    out.desktop = {3840, 2160};
    out.makeGame = [] { return std::make_unique<FakeGame>(); };
    return out;
}

bool pixelIs(const std::vector<std::uint8_t>& pixels, int width, int x, int y, int r, int g, int b) {
    const std::uint8_t* p = &pixels[(std::size_t(y) * std::size_t(width) + std::size_t(x)) * 4];
    return std::abs(p[0] - r) <= 2 && std::abs(p[1] - g) <= 2 && std::abs(p[2] - b) <= 2;
}

void openAndDraw(fe::Resources& resources) {
    qol::FrontMenu menu(resources, ports());
    menu.open();
    require(menu.isOpen() && menu.menu()->tabTitles() == std::vector<std::string>{"Game", "Display", "Quality of life"},
        "the menu opens with the tabs of the given stores");
    const std::string english = menu.menu()->screen().rows.at(3).value;
    require(!english.empty() && english == resources.textForKey("localisation/gametext_uk.loc", chase::runtime::kLanguageNameKeys[0]),
        "the Language row shows the gametext name of language 1");
    for (const auto& size : {std::pair<int, int>{1920, 1080}, {3840, 2160}}) {
        const int width = size.first, height = size.second;
        const std::string at = " at " + std::to_string(width) + "x" + std::to_string(height);
        require(menu.present(width, height), "the open menu is presented" + at);
        const std::vector<std::uint8_t> pixels = chase::render::readWindowPixels(width, height);
        const int row = int(std::lround(height * 0.055));
        require(pixelIs(pixels, width, 2, row / 2, 0x16, 0x18, 0x1D), "the tab bar is the opaque panel colour" + at);
        require(pixelIs(pixels, width, width - 3, height - row / 2, 0x16, 0x18, 0x1D), "the hint bar is the opaque panel colour" + at);
        require(pixelIs(pixels, width, 2, height / 2, 0, 0, 0), "beside the row column the dimmed black window shows" + at);
        int text = 0;
        for (int x = 0; x < width; ++x) text += !pixelIs(pixels, width, x, row / 2, 0x16, 0x18, 0x1D);
        require(text > 0, "the tab titles are drawn in the tab bar" + at);
    }
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-ui-menu-window-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-ui-menu-window-real-test: data not available, skipped\n";
        return 77;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "qol-ui-menu-window-real-test: no video, skipped\n";
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("ui menu window test", 0, 0, 3840, 2160, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
    if (!context) {
        std::cout << "qol-ui-menu-window-real-test: no GL 4.6 context, skipped\n";
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        openAndDraw(resources);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        ++failures;
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-ui-menu-window-real-test: ok\n";
    return 0;
}

// The window glue (src/runtime/qol_window.*) gives the SDL window the minimum size 400x240 at creation.
// Spec: docs/design/qol-screen-layout.md 2026-10-28T22:21:19 ("The window has a minimum size of 400x240, so a smaller size can't happen").
// Cases by 20261102-qolctl2-F, 2026-11-02, with SDL_VIDEODRIVER=dummy (no display, no OpenGL).
#include "runtime/qol_window.hpp"

#include <SDL.h>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

int failures = 0;

void expect(bool ok, const char* what) {
    if (ok) return;
    std::printf("FAIL: %s\n", what);
    ++failures;
}

struct Created {
    int width = 0, height = 0;
    int minimumWidth = 0, minimumHeight = 0;
    int actualWidth = 0, actualHeight = 0;
    int shrunkWidth = 0, shrunkHeight = 0;
};

Created create(const std::string& config, std::optional<std::pair<int, int>> explicitSize) {
    chase::runtime::QolWindow glue(config, explicitSize);
    Created made;
    made.width = glue.width();
    made.height = glue.height();
    SDL_Window* window = SDL_CreateWindow("window minimum test", 0, 0, glue.width(), glue.height(), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (!window) {
        std::printf("FAIL: cannot create a window (%s)\n", SDL_GetError());
        ++failures;
        return made;
    }
    glue.attach(window);
    SDL_GetWindowMinimumSize(window, &made.minimumWidth, &made.minimumHeight);
    SDL_GetWindowSize(window, &made.actualWidth, &made.actualHeight);
    SDL_SetWindowSize(window, 100, 100);
    SDL_GetWindowSize(window, &made.shrunkWidth, &made.shrunkHeight);
    SDL_DestroyWindow(window);
    return made;
}

}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::printf("FAIL: no dummy video driver (%s)\n", SDL_GetError());
        return 1;
    }
    const auto directory = std::filesystem::temp_directory_path() / "qol-window-minimum-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const std::string config = (directory / "qol.cfg").string();

    bool rejected = false;
    try {
        chase::runtime::QolWindow glue(config, std::make_pair(200, 240));
    } catch (const std::runtime_error& error) {
        rejected = std::string(error.what()) == "--window 200x240: below the minimum 400x240";
    }
    expect(rejected, "an explicit 200x240 window stops with an error naming --window");

    const Created exact = create(config, std::make_pair(400, 240));
    expect(exact.width == 400 && exact.height == 240 && exact.actualWidth == 400 && exact.actualHeight == 240,
        "an explicit 400x240 window is created at 400x240");
    expect(exact.minimumWidth == 400 && exact.minimumHeight == 240, "the minimum is 400x240 after creation");
    expect(exact.shrunkWidth >= 400 && exact.shrunkHeight >= 240, "the window cannot be made smaller than 400x240");

    const Created tall = create(config, std::make_pair(400, 480));
    expect(tall.width == 400 && tall.height == 480 && tall.actualWidth == 400 && tall.actualHeight == 480,
        "an explicit size above the minimum is kept");
    expect(tall.shrunkWidth >= 400 && tall.shrunkHeight >= 240, "a window above the minimum cannot be shrunk below it");

    std::filesystem::remove_all(directory);
    SDL_Quit();
    return failures == 0 ? 0 : 1;
}

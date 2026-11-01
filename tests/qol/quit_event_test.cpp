// Quit game on the open settings menu reaches the program as an SDL_QUIT event (QOLCTL-5): the same event the window's close button sends, so
// chase-game's own loops (level and front end) end the program the way they do for it.
// Spec: docs/design/qol-controls.md entry 2026-11-01T13:50:44 (Quit game row with Yes/No; closing the window still ends the program) and its
// answer of 2026-11-01 "Only via PC Settings (Recommended)". Cases written by 20261101-qolctl-B on 2026-11-01.
#include "qol/active_settings.hpp"
#include "qol/game_port.hpp"
#include "qol/ui_menu_sdl.hpp"

#include <SDL.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

struct FakeGame final : qol::GamePort {
    int sfx = 10, music = 8, lang = 1;
    bool invert = false;
    int sfxVolume() const override { return sfx; }
    int musicVolume() const override { return music; }
    bool invertCamera() const override { return invert; }
    int language() const override { return lang; }
    void setSfxVolume(int v) override { sfx = v; }
    void setMusicVolume(int v) override { music = v; }
    void setInvertCamera(bool v) override { invert = v; }
    void openLanguages() override {}
    int startLanguage() const override { return lang; }
    void applied() override {}
    void left() override {}
};

SDL_Event key(SDL_Keycode sym) {
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = sym;
    return event;
}

int quitEventsPending() {
    SDL_PumpEvents();
    return SDL_PeepEvents(nullptr, 0, SDL_PEEKEVENT, SDL_QUIT, SDL_QUIT);
}

struct Fixture {
    std::unique_ptr<qol::UiMenuWindow> window;

    Fixture() {
        qol::setActiveSettings(qol::Settings{});
        const auto folder = std::filesystem::temp_directory_path() / "chase-qol-quit-event";
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        qol::StoresPorts ports;
        ports.settingsPath = (folder / "qol.cfg").string();
        ports.modes = {{800, 600}, {1920, 1080}};
        ports.desktop = {1920, 1080};
        ports.makeGame = []() -> std::unique_ptr<qol::GamePort> { return std::make_unique<FakeGame>(); };
        window = std::make_unique<qol::UiMenuWindow>(ports, std::vector<std::string>{"English", "French", "Italian", "German", "Spanish", "Dutch", "Danish", "Portuguese", "Russian"});
        window->open();
    }

    void press(SDL_Keycode sym) { window->consume(key(sym), 1280, 720); }
    void toQuitQuestion() {
        for (int i = 0; i < 4; ++i) press(SDLK_DOWN);
        press(SDLK_RETURN);
    }
};

void yesPushesTheCloseEvent() {
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    Fixture f;
    f.toQuitQuestion();
    require(quitEventsPending() == 0, "asking pushes no event");
    f.press(SDLK_RETURN);
    require(quitEventsPending() == 0, "Enter on the default No pushes no event");
    f.press(SDLK_RETURN);
    f.press(SDLK_LEFT);
    f.press(SDLK_RETURN);
    require(quitEventsPending() == 1, "Yes pushes one SDL_QUIT");
    SDL_Event quit{};
    quit.type = SDL_QUIT;
    require(!f.window->consume(quit, 1280, 720), "a window close passes through the open menu");
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
}

void cancelPushesNothing() {
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    Fixture f;
    f.toQuitQuestion();
    f.press(SDLK_ESCAPE);
    require(f.window->isOpen() && quitEventsPending() == 0, "Escape cancels: no event and the menu stays open");
    f.press(SDLK_RETURN);
    f.press(SDLK_LEFT);
    f.press(SDLK_RIGHT);
    f.press(SDLK_RETURN);
    require(quitEventsPending() == 0, "Left, Right and Enter ending on No pushes no event");
}

}

int main() {
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << '\n';
        return 1;
    }
    yesPushesTheCloseEvent();
    cancelPushesNothing();
    SDL_Quit();
    if (failures == 0) std::cout << "qol-quit-event: all checks passed\n";
    return failures == 0 ? 0 : 1;
}

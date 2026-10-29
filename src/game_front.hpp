#pragma once
#include "assets/assets.hpp"
#include "frontend/frontend.hpp"
#include "frontend/movie_host.hpp"
#include "frontend/script.hpp"
#include "frontend/slot_summaries.hpp"
#include "game_audio.hpp"
#include "game_hud.hpp"
#include "game_movie_sound.hpp"
#include "input.hpp"
#include "render/debug_overlay.hpp"
#include "render/front_presenter.hpp"
#include "render/scene.hpp"
#include "render/window.hpp"
#include "runtime/debug_metrics.hpp"
#include "runtime/scheduler.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>

namespace game_front {

namespace fe = chase::frontend;
using ::MovieSound;

struct Options {
    std::string cutsceneDirectory;
    int language = 1;
    bool firstRun = true;
    std::string script;
    std::string screenshot;
    std::string saveFile;
    bool debugOverlay = false;
    std::function<void()> frame;
};

inline std::string directoryOf(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}

struct Result {
    bool quit = false;
    bool scriptOnly = false;
    fe::Handoff handoff;
};

class FrontDebugOverlay {
public:
    explicit FrontDebugOverlay(bool startVisible) : startVisible_(startVisible) {}
    void frame() { metrics_.frame(); }
    void draw(const chase::InputState& input, const fe::FrontEnd& front, int width, int height) {
        const bool visible = startVisible_ != input.active(chase::Action::DebugOverlay);
        metrics_.setVisible(visible);
        if (!visible) return;
        if (!overlay_) overlay_.emplace();
        overlay_->draw(chase::runtime::debugFrontEndLines(metrics_.readout(), fe::screenName(front.flow().screen())), width, height);
    }

private:
    bool startVisible_;
    chase::runtime::DebugMetrics metrics_;
    std::optional<chase::render::DebugOverlay> overlay_;
};

inline Result run(SDL_Window* window, chase::InputState& input, chase::assets::FuseArchive& archive, GameAudio* audio,
                  const Options& options, const std::function<std::uint32_t()>& clockNow) {
    fe::Resources resources(archive);
    fe::MoviePictureHost movies(options.cutsceneDirectory);
    fe::FlowConfig config;
    config.language = options.language;
    config.firstRun = options.firstRun;
    if (!options.saveFile.empty()) config.slots = fe::slotSummariesFromSave(options.saveFile, archive, options.language);
    fe::FrontEnd front(resources, &movies, config);
    fe::Canvas top(fe::kTopWidth, fe::kScreenHeight), bottom(fe::kBottomWidth, fe::kScreenHeight);
    chase::render::FrontPresenter presenter(fe::kTopWidth, fe::kBottomWidth, fe::kScreenHeight);
    MovieSound movieSound;
    int width = 0, height = 0;
    FrontDebugOverlay debugOverlay(options.debugOverlay);

    if (!options.script.empty()) {
        fe::ScriptHooks hooks;
        const unsigned updates = fe::runScript(front, options.script, hooks);
        Result result;
        result.handoff = front.flow().handoff();
        if (front.flow().done()) return result;
        front.draw(top, bottom);
        SDL_GL_GetDrawableSize(window, &width, &height);
        presenter.present(game_hud::pixelsOf(top), game_hud::pixelsOf(bottom), width, height);
        debugOverlay.draw(input, front, width, height);
        chase::render::waitForGpu();
        if (!options.screenshot.empty()) chase::render::saveScreenshot(options.screenshot, width, height);
        std::cout << "front end: " << updates << " update(s), screen " << fe::screenName(front.flow().screen()) << ", "
                  << (options.screenshot.empty() ? "" : "saved " + options.screenshot) << '\n';
        SDL_GL_SwapWindow(window);
        result.scriptOnly = true;
        return result;
    }

    chase::runtime::Scheduler scheduler({}, clockNow());
    bool wasUp = false, wasDown = false, wasA = false, wasB = false, wasX = false, wasY = false, wasStart = false;
    bool running = true;
    while (running && !front.flow().done()) {
        debugOverlay.frame();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: case SDL_KEYUP:
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
                input.event(chase::Source::Key, event.key.keysym.sym, event.type == SDL_KEYDOWN, event.key.repeat != 0);
                break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
                input.event(chase::Source::Mouse, event.button.button, event.type == SDL_MOUSEBUTTONDOWN);
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) input.reset();
                break;
            default: break;
            }
        }
        if (options.frame) options.frame();
        scheduler.advance(clockNow(), [&](float dt) {
            using chase::Action;
            const bool up = input.vertical() < 0, down = input.vertical() > 0;
            const bool a = input.active(Action::A), b = input.active(Action::B), x = input.active(Action::X),
                       y = input.active(Action::Y), start = input.active(Action::Start);
            fe::FrontInput pad;
            if (up && !wasUp) pad.pressed |= fe::kButtonUp;
            if (down && !wasDown) pad.pressed |= fe::kButtonDown;
            if (a && !wasA) pad.pressed |= fe::kButtonA;
            if (b && !wasB) pad.pressed |= fe::kButtonB;
            if (x && !wasX) pad.pressed |= fe::kButtonX;
            if (y && !wasY) pad.pressed |= fe::kButtonY;
            if (start && !wasStart) pad.pressed |= fe::kButtonStart;
            wasUp = up; wasDown = down; wasA = a; wasB = b; wasX = x; wasY = y; wasStart = start;
            front.update(dt, pad);
            if (audio)
                for (const int id : front.takeSounds()) audio->playUi(unsigned(id));
            movieSound.queue(front.takeMovieAudio());
        });
        if (audio) audio->update();
        front.draw(top, bottom);
        SDL_GL_GetDrawableSize(window, &width, &height);
        presenter.present(game_hud::pixelsOf(top), game_hud::pixelsOf(bottom), width, height);
        debugOverlay.draw(input, front, width, height);
        SDL_GL_SwapWindow(window);
    }
    Result result;
    result.quit = !running;
    result.handoff = front.flow().handoff();
    return result;
}

}

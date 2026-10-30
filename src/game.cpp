// chase-game: development runtime skeleton. Not the playable game.
// Joins the recovered frame scheduler, pad pipeline and player locomotion (chase_runtime), the controls
// (chase_input) and the development renderer (chase_render) into one loop. The player character walks,
// runs and sprints with the recovered follow camera; the level's static collision keeps him on the floor and
// stops him at walls, and he falls off ledges, jumps and lands. The camera keeps out of the level's static
// collision. There is no gameplay.
#include "assets/assets.hpp"
#include "game_audio.hpp"
#include "game_front.hpp"
#include "game_level.hpp"
#include "game_options.hpp"
#include "game_save.hpp"
#include "game_touch.hpp"
#include "input.hpp"
#include "pad_sdl.hpp"
#include "render/debug_overlay.hpp"
#include "render/scene.hpp"
#include "render/window.hpp"
#include "runtime/debug_metrics.hpp"
#include "runtime/level_start.hpp"
#include "runtime/pad.hpp"
#include "runtime/qol_window.hpp"
#include "runtime/qol_game_wiring.hpp"
#include "runtime/scene_change.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;
using namespace chase::render;
namespace runtime = chase::runtime;
using game_options::Options;

constexpr int kStickFullDeflection = 145;
// Development stand-in, not the original: a keyboard has no half deflection, so holding Walk (Left Alt) tilts the
// stick to this length (raw 3DS units), which is inside the walk regime (walk below raw length 117).
constexpr int kStickWalkDeflection = 90;

int resolveKey(const std::string& name) {
    std::string spaced = name;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    return SDL_GetKeyFromName(spaced.c_str());
}

runtime::PadState padState(const chase::InputState& input) {
    using chase::Action;
    using runtime::PadButton;
    const std::pair<Action, PadButton> buttons[] = {
        {Action::A, runtime::PadA}, {Action::B, runtime::PadB}, {Action::X, runtime::PadX},
        {Action::Y, runtime::PadY}, {Action::L, runtime::PadL}, {Action::R, runtime::PadR},
        {Action::Start, runtime::PadStart}, {Action::Select, runtime::PadSelect},
    };
    runtime::PadState state;
    for (const auto& [action, bit] : buttons)
        if (input.active(action)) state.held |= bit;
    const double x = input.horizontal(), y = -input.vertical();
    const double length = std::hypot(x, y);
    if (length > 0) {
        const double deflection = input.active(Action::Walk) ? kStickWalkDeflection : kStickFullDeflection;
        state.stickX = std::int16_t(std::lround(x / length * deflection));
        state.stickY = std::int16_t(std::lround(y / length * deflection));
    }
    return state;
}

void pollEvents(chase::InputState& input, PadDevice& pad, bool& running, game_touch::Tracker& touch, SDL_Window* window,
                game_level::LevelSession& session) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (session.pauseSettingsConsume(event, window)) continue;
        if (touch.consume(event, window)) continue;
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
        default: pad.handle(event, input); break;
        }
    }
}

void shutDown(SDL_GLContext context, SDL_Window* window) {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

std::string configDirectory(const std::string& file) {
    const std::size_t slash = file.find_last_of('/');
    return slash == std::string::npos ? std::string(".") : file.substr(0, slash);
}

int run(const Options& given) {
    Options options = given;
    std::ifstream config(options.config);
    if (!config) throw std::runtime_error("cannot open controls: " + options.config);
    chase::InputState input(chase::readBindings(config, resolveKey));
    FuseArchive archive(options.fib);
    archive.loadNames(options.names);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error(SDL_GetError());
    PadDevice pad;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    runtime::QolWindow qolWindow(configDirectory(options.config) + "/qol.cfg",
        options.windowGiven ? std::optional<std::pair<int, int>>({options.windowWidth, options.windowHeight}) : std::nullopt);
    int width = qolWindow.width(), height = qolWindow.height();
    SDL_Window* window = SDL_CreateWindow("The Chase Begins - development runtime (not the playable game)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | qolWindow.flags());
    if (!window) throw std::runtime_error(SDL_GetError());
    qolWindow.attach(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(1);

    std::optional<GameAudio> audio;
    try {
        audio.emplace(archive, std::uint8_t(options.soundVolume), std::uint8_t(options.musicVolume));
    } catch (const std::exception& error) {
        std::cout << "audio: disabled (" << error.what() << ")\n";
    }
    const std::uint64_t frontFrequency = SDL_GetPerformanceFrequency();
    const std::uint64_t frontOrigin = SDL_GetPerformanceCounter();
    if (!options.gameplay) {
        game_front::Options front;
        front.cutsceneDirectory = game_front::directoryOf(options.fib) + "/cutscenes";
        front.language = int(options.language);
        front.firstRun = !options.returningPlayer;
        front.script = options.frontScript;
        front.screenshot = options.screenshot;
        front.saveFile = options.frontSave;
        front.debugOverlay = options.debugOverlay;
        front.frame = [&]() { qolWindow.poll(input); };
        front.qolStores = qolWindow.menuStores(options.config, input);
        front.qolStores.makeGame = runtime::qolGamePortFactory({options.frontSave, options.saveOut, int(options.language),
            options.soundVolume, options.musicVolume, [&audio](std::optional<float> sound, std::optional<float> music) {
                if (audio) audio->setOptionGains(sound, music);
            }});
        const game_front::Result result =
            game_front::run(window, input, archive, audio ? &*audio : nullptr, front,
                            [&]() { return game_level::clockTicks(SDL_GetPerformanceCounter() - frontOrigin, frontFrequency); });
        if (result.quit || result.scriptOnly) {
            audio.reset();
            shutDown(context, window);
            return 0;
        }
        options.saveSlot = result.handoff.slot;
        if (result.handoff.newGame) {
            options.introMovieShown = true;
            options.level = game_options::kNewGameLevel;
            options.start = game_options::kNewGameStart;
        } else {
            options.saveFile = options.frontSave;
        }
    }
    if (!options.saveFile.empty() && !options.levelGiven) game_save::chooseContinueLevel(archive, options);
    const FuseEntry* masterEntry = archive.find(runtime::kMasterLevelFile);
    if (!masterEntry) throw std::runtime_error(std::string("not in archive: ") + runtime::kMasterLevelFile);
    const Level master = parseLevel(archive.read(*masterEntry));
    const std::vector<runtime::SceneChangeObject> sceneChanges = runtime::sceneChangeObjects(master);
    runtime::DebugMetrics debugMetrics;
    std::optional<chase::render::DebugOverlay> debugOverlay;
    unsigned scriptOffset = 0;
    bool afterSwitch = false;
    game_save::Progress progress;
    for (;;) {
        game_level::LevelSession session({archive, options, audio, master, sceneChanges, scriptOffset, afterSwitch, progress, qolWindow.menuStores(options.config, input)});
        bool running = true;
        game_touch::Tracker touch;
        Uint32 secondStart = SDL_GetTicks();
        while (running) {
            pollEvents(input, pad, running, touch, window, session);
            qolWindow.poll(input);
            if (!session.frameDue()) {
                SDL_Delay(1);
                continue;
            }
            debugMetrics.frame();
            runtime::PadState pad = padState(input);
            touch.apply(pad);
            session.advance(pad);

            SDL_GL_GetDrawableSize(window, &width, &height);
            const bool debugVisible = options.debugOverlay != input.active(chase::Action::DebugOverlay);
            debugMetrics.setVisible(debugVisible);
            if (debugVisible && !debugOverlay) debugOverlay.emplace();
            if (debugVisible) debugOverlay->beginFrame();
            session.draw(width, height);
            if (debugVisible) {
                const chase::render::FrameStats renderStats = debugOverlay->endFrame();
                chase::runtime::DebugReadout readout = debugMetrics.readout();
                if (renderStats.gpuValid) readout.gpuMilliseconds = renderStats.gpuMilliseconds;
                readout.drawCalls = renderStats.drawCalls;
                debugOverlay->draw(runtime::debugOverlayLines(readout, session.debugValues()), width, height);
            }

            const bool scriptDone = session.scriptDone();
            if (scriptDone) session.reportScripted();
            if (!options.screenshot.empty() && (session.scripted() ? scriptDone : session.updateCount() >= options.frames)) {
                waitForGpu();
                saveScreenshot(options.screenshot, width, height);
                std::cout << "saved " << options.screenshot << " after " << session.updateCount() << " update(s)\n";
                running = false;
            }
            if (scriptDone || session.switchTo) running = false;
            SDL_GL_SwapWindow(window);

            Uint32 tick = SDL_GetTicks();
            if (tick - secondStart >= 1000) {
                session.reportRate();
                secondStart = tick;
            }
        }
        debugOverlay.reset();
        session.releaseOverlays();
        if (!session.switchTo) break;
        session.handOverProgress();
        game_level::advanceToDestination(archive, options, *session.switchTo, session.updateCount(), scriptOffset, afterSwitch);
    }
    shutDown(context, window);
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(game_options::parse(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "chase-game: " << error.what() << '\n';
        return 1;
    }
}

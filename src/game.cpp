// chase-game: development runtime skeleton. Not the playable game.
// Joins the recovered frame scheduler and pad pipeline (chase_runtime), the controls
// (chase_input) and the development renderer (chase_render) into one loop. Gameplay is not recovered.
#include "assets/assets.hpp"
#include "input.hpp"
#include "render/scene.hpp"
#include "runtime/pad.hpp"
#include "runtime/scheduler.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;
using namespace chase::render;
namespace runtime = chase::runtime;

constexpr int kStickFullDeflection = 145;
constexpr std::size_t kSlotLeftShoulder = 12, kSlotRightShoulder = 13, kSlotStickX = 14, kSlotStickY = 15;
constexpr std::size_t kSlotButtonX = 8, kSlotButtonY = 9;

int resolveKey(const std::string& name) {
    std::string spaced = name;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    return SDL_GetKeyFromName(spaced.c_str());
}

struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib";
    std::string names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string config = "config/controls.cfg";
    std::string level = "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl";
    std::string screenshot;
    unsigned frames = 2;
};

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--fib") o.fib = value();
        else if (arg == "--names") o.names = value();
        else if (arg == "--config") o.config = value();
        else if (arg == "--level") o.level = value();
        else if (arg == "--screenshot") o.screenshot = value();
        else if (arg == "--frames") o.frames = unsigned(std::stoul(value()));
        else throw std::runtime_error("usage: chase-game [--fib F] [--names CSV] [--config CFG] "
                                      "[--level levels/levels/<name>/<name>_main.blvl] [--screenshot OUT.bmp] [--frames N]");
    }
    return o;
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
    state.stickX = std::int16_t(input.horizontal() * kStickFullDeflection);
    state.stickY = std::int16_t(-input.vertical() * kStickFullDeflection);
    return state;
}

// Development stand-in, not recovered game logic: a free camera driven by the pad channels.
struct PlaceholderCamera {
    float eye[3] = {0, 0, 0};
    float yaw = 35.0f * float(M_PI) / 180, pitch = -25.0f * float(M_PI) / 180;
    float speed = 1.0f;

    void frameScene(const float centre[3], float radius) {
        float d = radius * 2.2f;
        eye[0] = centre[0] - d * std::cos(pitch) * std::sin(yaw);
        eye[1] = centre[1] - d * std::sin(pitch);
        eye[2] = centre[2] + d * std::cos(pitch) * std::cos(yaw);
        speed = radius * 0.5f;
    }

    void update(float dt, const runtime::PadDevice& pad) {
        float strafe = pad.slot(kSlotStickX).value, forward = pad.slot(kSlotStickY).value;
        float rise = pad.slot(kSlotRightShoulder).value - pad.slot(kSlotLeftShoulder).value;
        yaw += 1.5f * dt * (pad.slot(kSlotButtonY).value - pad.slot(kSlotButtonX).value);
        float step = speed * dt;
        eye[0] += step * (forward * std::cos(pitch) * std::sin(yaw) + strafe * std::cos(yaw));
        eye[1] += step * (forward * std::sin(pitch) + rise);
        eye[2] += step * (-forward * std::cos(pitch) * std::cos(yaw) + strafe * std::sin(yaw));
    }
};

std::uint32_t clockTicks(std::uint64_t counter, std::uint64_t frequency) {
    std::uint64_t seconds = counter / frequency, rest = counter % frequency;
    return std::uint32_t(seconds * runtime::kClockFrequency + rest * runtime::kClockFrequency / frequency);
}

int run(const Options& options) {
    std::ifstream config(options.config);
    if (!config) throw std::runtime_error("cannot open controls: " + options.config);
    chase::InputState input(chase::readBindings(config, resolveKey));
    FuseArchive archive(options.fib);
    archive.loadNames(options.names);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    int width = 960, height = 600;
    SDL_Window* window = SDL_CreateWindow("The Chase Begins - development runtime (not the playable game)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) throw std::runtime_error(SDL_GetError());
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(1);

    GLuint program = createProgram();
    Scene scene(archive, {"sky"});
    for (const auto& model : levelModels(archive, options.level, ""))
        if (!scene.addModel(model)) throw std::runtime_error("not in archive: " + model);
    const FuseEntry* entry = archive.find(options.level);
    if (!entry) throw std::runtime_error("not in archive: " + options.level);
    std::size_t placed = 0, unresolved = 0;
    for (const auto& placement : placements(parseLevel(archive.read(*entry))))
        if (scene.addModel(meshModelPath(placement.mesh), placementMatrix(placement))) ++placed;
        else ++unresolved;

    float centre[3], radius;
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    PlaceholderCamera camera;
    camera.frameScene(centre, radius);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    runtime::PadDevice pad;
    const std::uint64_t counterFrequency = SDL_GetPerformanceFrequency();
    const std::uint64_t counterOrigin = SDL_GetPerformanceCounter();
    auto now = [&]() { return clockTicks(SDL_GetPerformanceCounter() - counterOrigin, counterFrequency); };
    runtime::Scheduler scheduler({}, now());

    bool running = true;
    unsigned updates = 0, updatesThisSecond = 0;
    Uint32 secondStart = SDL_GetTicks();
    while (running) {
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
        const runtime::PadState state = padState(input);
        scheduler.advance(now(), [&](float dt) {
            pad.poll(state);
            camera.update(dt, pad);
            ++updates;
            ++updatesThisSecond;
        });

        SDL_GL_GetDrawableSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.16f, 0.17f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        beginFrame(program, 1.0f);
        Mat4 mvp = chase::render::multiply(
            perspective(1.0f, float(width) / float(std::max(height, 1)), radius * 0.002f, radius * 20),
            chase::render::view(camera.eye, camera.yaw, camera.pitch));
        scene.draw(program, mvp);

        if (!options.screenshot.empty() && updates >= options.frames) {
            glFinish();
            saveScreenshot(options.screenshot, width, height);
            std::cout << "saved " << options.screenshot << " after " << updates << " update(s)\n";
            running = false;
        }
        SDL_GL_SwapWindow(window);

        Uint32 tick = SDL_GetTicks();
        if (tick - secondStart >= 1000) {
            std::cout << "updates/s " << updatesThisSecond << ", dt " << scheduler.lastDt() << " s, stick "
                      << pad.slot(kSlotStickX).value << ' ' << pad.slot(kSlotStickY).value << '\n';
            updatesThisSecond = 0;
            secondStart = tick;
        }
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(parse(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "chase-game: " << error.what() << '\n';
        return 1;
    }
}

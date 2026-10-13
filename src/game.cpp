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
#include <map>
#include <optional>
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

// The class whose CharacterEnum 9 row ("police") is the one character the completion count excludes,
// taken as the starting character (inference). The idle animation is picked by file name only.
constexpr const char* kPlayerCharacter = "levels/chars/char_police.blvl";
constexpr const char* kPlayerBaseCharacter = "levels/chars/char_player.blvl";
constexpr const char* kPlayerIdleAnimation = "models/char_minifig/class_police/idle_minifig.bfnanm";

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
        float axisForward[3], axisRight[3];
        chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
        for (int i = 0; i < 3; ++i) eye[i] = centre[i] - d * axisForward[i];
        speed = radius * 0.5f;
    }

    void frameBehind(const Mat4& player) {
        constexpr float distance = 5.0f, height = 1.6f, lookHeight = 0.9f;
        const float forward[2] = {player[8], player[10]};
        eye[0] = player[12] - distance * forward[0];
        eye[1] = player[13] + height;
        eye[2] = player[14] - distance * forward[1];
        yaw = std::atan2(forward[0], forward[1]);
        pitch = std::atan2(lookHeight - height, distance);
    }

    void update(float dt, const runtime::PadDevice& pad) {
        float strafe = pad.slot(kSlotStickX).value, forward = pad.slot(kSlotStickY).value;
        float rise = pad.slot(kSlotRightShoulder).value - pad.slot(kSlotLeftShoulder).value;
        yaw += 1.5f * dt * (pad.slot(kSlotButtonY).value - pad.slot(kSlotButtonX).value);
        float step = speed * dt;
        float axisForward[3], axisRight[3];
        chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
        for (int i = 0; i < 3; ++i) eye[i] += step * (forward * axisForward[i] + strafe * axisRight[i]);
        eye[1] += step * rise;
    }
};

std::string withExtension(std::string path, const std::string& extension) {
    return path.substr(0, path.rfind('.')) + extension;
}

std::string lowerStem(const std::string& file) {
    std::string name = lower(file);
    return name.substr(0, name.rfind('.'));
}

CharacterAppearance readCharacter(FuseArchive& archive, const std::string& path) {
    const FuseEntry* entry = archive.find(path);
    if (!entry) throw std::runtime_error("not in archive: " + path);
    return characterAppearance(parseLevel(archive.read(*entry)));
}

// Places the character at the level's player start. The head model sits on the animated head joint.
// Returns the player's world matrix, or nothing when the level has no "Player1Start".
std::optional<Mat4> placePlayer(FuseArchive& archive, Scene& scene, const Level& level) {
    auto start = playerStart(level);
    if (!start) return std::nullopt;
    const Mat4 player = placementMatrix(*start);
    const CharacterAppearance base = readCharacter(archive, kPlayerBaseCharacter);
    const CharacterAppearance character = readCharacter(archive, kPlayerCharacter);
    const std::string bodyPath = meshModelPath(character.body), headPath = meshModelPath(character.head);
    const std::map<std::string, std::string> bodyTextures = {
        {lowerStem(base.lower), character.lower}, {lowerStem(base.upper), character.upper}};
    const std::map<std::string, std::string> headTextures = {{lowerStem(base.face), character.face}};

    const Skeleton skeleton = parseSkeleton(archive.read(*archive.find(withExtension(bodyPath, ".bfnskl"))));
    const Animation idle = parseAnimation(archive.read(*archive.find(kPlayerIdleAnimation)));
    const std::vector<Matrix> skin = skinMatrices(skeleton, idle, 0);
    const auto head = std::find_if(skeleton.joints.begin(), skeleton.joints.end(),
                                   [](const Joint& joint) { return lower(joint.name) == "head"; });

    if (!scene.addModel(bodyPath, player, bodyTextures)) throw std::runtime_error("not in archive: " + bodyPath);
    scene.pose(bodyPath, skin);
    Mat4 headTransform = player;
    if (head != skeleton.joints.end()) {
        const std::size_t joint = std::size_t(head - skeleton.joints.begin());
        headTransform = chase::assets::multiply(chase::assets::jointAttachment(*head, skin[joint]), player);
    }
    if (!scene.addModel(headPath, headTransform, headTextures)) throw std::runtime_error("not in archive: " + headPath);
    std::cout << "player at " << start->position[0] << ' ' << start->position[1] << ' ' << start->position[2]
              << ", yaw " << start->orientation[1] << ", character " << character.characterEnum << " ("
              << character.head << ")\n";
    return player;
}

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
    const Level level = parseLevel(archive.read(*entry));
    for (const auto& placement : placements(level))
        if (scene.addModel(meshModelPath(placement.mesh), placementMatrix(placement))) ++placed;
        else ++unresolved;
    const std::optional<Mat4> player = placePlayer(archive, scene, level);

    float centre[3], radius;
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    PlaceholderCamera camera;
    camera.frameScene(centre, radius);
    if (player) camera.frameBehind(*player);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    chase::render::setFrontFace(true);

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

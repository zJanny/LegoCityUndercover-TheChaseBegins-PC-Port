// chase-game: development runtime skeleton. Not the playable game.
// Joins the recovered frame scheduler, pad pipeline and player locomotion (chase_runtime), the controls
// (chase_input) and the development renderer (chase_render) into one loop. The player character walks,
// runs and sprints on the ground plane; there is no collision, ground height, camera law or gameplay.
#include "assets/assets.hpp"
#include "input.hpp"
#include "render/scene.hpp"
#include "runtime/pad.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
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
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;
using namespace chase::render;
namespace runtime = chase::runtime;

constexpr int kStickFullDeflection = 145;
// Development stand-in, not the original: a keyboard has no half deflection, so holding L (key Z) tilts the
// stick to this length (raw 3DS units), which is inside the walk regime (walk below raw length 117).
constexpr int kStickWalkDeflection = 90;
constexpr float kScriptDt = 0.033f;
constexpr std::size_t kSlotLeftShoulder = 12, kSlotRightShoulder = 13, kSlotStickX = 14, kSlotStickY = 15;
constexpr std::size_t kSlotButtonX = 8, kSlotButtonY = 9;

// The class whose CharacterEnum 9 row ("police") is the one character the completion count excludes,
// taken as the starting character (inference). The idle animation is picked by file name only.
constexpr const char* kPlayerCharacter = "levels/chars/char_police.blvl";
constexpr const char* kPlayerBaseCharacter = "levels/chars/char_player.blvl";
constexpr const char* kPlayerAnimationDirectory = "models/char_minifig/class_police/";
constexpr const char* kPlayerAnimationSuffix = "_minifig.bfnanm";
// PROVISIONAL: the playback rate of the original (written by the caller that starts an animation) is not
// recovered; this is the viewer's assumed 30 frames per second, times the role's speed scale.
constexpr double kAnimationFramesPerSecond = 30.0;
// ctrl+0x6c of the player is not known; the traced headings match with the stick shaping on, which every
// type except 0x14b and 0x189 gets.
constexpr std::uint16_t kPlayerEntityType = 0x100;

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
    std::string script;
};

struct ScriptStep {
    int stickX = 0, stickY = 0;
    unsigned updates = 0;
};

// "x y count;x y count": raw circle-pad values (+x right, +y up, 145 = full) held for count updates.
std::vector<ScriptStep> parseScript(const std::string& text) {
    std::vector<ScriptStep> steps;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, ';')) {
        std::stringstream fields(part);
        ScriptStep step;
        if (!(fields >> step.stickX >> step.stickY >> step.updates)) throw std::runtime_error("bad --script step: " + part);
        steps.push_back(step);
    }
    return steps;
}

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
        else if (arg == "--script") o.script = value();
        else throw std::runtime_error("usage: chase-game [--fib F] [--names CSV] [--config CFG] "
                                      "[--level levels/levels/<name>/<name>_main.blvl] [--screenshot OUT.bmp] [--frames N] "
                                      "[--script \"X Y COUNT;...\"]");
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
    const double x = input.horizontal(), y = -input.vertical();
    const double length = std::hypot(x, y);
    if (length > 0) {
        const double deflection = input.active(Action::L) ? kStickWalkDeflection : kStickFullDeflection;
        state.stickX = std::int16_t(std::lround(x / length * deflection));
        state.stickY = std::int16_t(std::lround(y / length * deflection));
    }
    return state;
}

// Development stand-in, not recovered game logic: the camera law of the original (writer of the yaw word
// 0x376894) is not recovered. With a player the eye follows behind him and E/R turn the yaw; the yaw is the
// one the stick intent adds to the stick heading, as in the game. Without a player it is a free camera.
struct PlaceholderCamera {
    float eye[3] = {0, 0, 0};
    float yaw = 35.0f * float(M_PI) / 180, pitch = -25.0f * float(M_PI) / 180;
    float speed = 1.0f;
    bool following = false;

    void frameScene(const float centre[3], float radius) {
        float d = radius * 2.2f;
        float axisForward[3], axisRight[3];
        chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
        for (int i = 0; i < 3; ++i) eye[i] = centre[i] - d * axisForward[i];
        speed = radius * 0.5f;
    }

    void follow(const float target[3]) {
        constexpr float distance = 5.0f, height = 1.6f, lookHeight = 0.9f;
        following = true;
        eye[0] = target[0] - distance * std::sin(yaw);
        eye[1] = target[1] + height;
        eye[2] = target[2] - distance * std::cos(yaw);
        pitch = std::atan2(lookHeight - height, distance);
    }

    void update(float dt, const runtime::PadDevice& pad) {
        yaw += 1.5f * dt * (pad.slot(kSlotButtonY).value - pad.slot(kSlotButtonX).value);
        if (following) return;
        float strafe = pad.slot(kSlotStickX).value, forward = pad.slot(kSlotStickY).value;
        float rise = pad.slot(kSlotRightShoulder).value - pad.slot(kSlotLeftShoulder).value;
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

// The player character: locomotion state, skeleton, role animations and the scene instances it moves.
struct PlayerRig {
    runtime::LocomotionState motion;
    float height = 0;
    Skeleton skeleton;
    std::optional<std::size_t> headJoint;
    std::map<int, Animation> animations;  // 0 = idle, runtime::kAnimationWalk / Run / Sprint
    int animation = 0;
    double frame = 0;
    std::string bodyPath;
    std::size_t bodyInstance = 0, headInstance = 0;
    float startX = 0, startZ = 0;

    Mat4 matrix() const {
        const float heading = runtime::headingRadians(motion.facing), s = std::sin(heading), c = std::cos(heading);
        return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, motion.x, height, motion.z, 1};
    }

    void animate(Scene& scene, float dt, const runtime::LocomotionStep& step) {
        if (step.animation != animation) {
            const double phase = frame / double(animations.at(animation).frames);
            animation = step.animation;
            frame = phase * double(animations.at(animation).frames);
        }
        const Animation& current = animations.at(animation);
        frame = std::fmod(frame + double(dt) * kAnimationFramesPerSecond * double(step.animationScale), double(current.frames));
        pose(scene);
    }

    void pose(Scene& scene) const {
        const std::vector<Matrix> skin = skinMatrices(skeleton, animations.at(animation), frame);
        scene.pose(bodyPath, skin);
        const Mat4 player = matrix();
        scene.setTransform(bodyInstance, player);
        Mat4 headTransform = player;
        if (headJoint)
            headTransform = chase::assets::multiply(chase::assets::jointAttachment(skeleton.joints[*headJoint], skin[*headJoint]), player);
        scene.setTransform(headInstance, headTransform);
    }
};

std::int16_t headingFromDegrees(float degrees) {
    return runtime::wrapHeading(std::int32_t(std::lround(degrees / 360.0f * 65536.0f)));
}

// Places the character at the level's player start. The head model sits on the animated head joint.
// Returns the rig, or nothing when the level has no "Player1Start".
std::optional<PlayerRig> placePlayer(FuseArchive& archive, Scene& scene, const Level& level) {
    auto start = playerStart(level);
    if (!start) return std::nullopt;
    const CharacterAppearance base = readCharacter(archive, kPlayerBaseCharacter);
    const CharacterAppearance character = readCharacter(archive, kPlayerCharacter);
    const std::string bodyPath = meshModelPath(character.body), headPath = meshModelPath(character.head);
    const std::map<std::string, std::string> bodyTextures = {
        {lowerStem(base.lower), character.lower}, {lowerStem(base.upper), character.upper}};
    const std::map<std::string, std::string> headTextures = {{lowerStem(base.face), character.face}};

    PlayerRig rig;
    rig.skeleton = parseSkeleton(archive.read(*archive.find(withExtension(bodyPath, ".bfnskl"))));
    const std::pair<int, const char*> roles[] = {
        {0, "idle"}, {runtime::kAnimationWalk, "walk"}, {runtime::kAnimationRun, "run"}, {runtime::kAnimationSprint, "sprint"}};
    for (const auto& [id, name] : roles) {
        const std::string path = std::string(kPlayerAnimationDirectory) + name + kPlayerAnimationSuffix;
        const FuseEntry* entry = archive.find(path);
        if (!entry) throw std::runtime_error("not in archive: " + path);
        rig.animations[id] = parseAnimation(archive.read(*entry));
    }
    const auto head = std::find_if(rig.skeleton.joints.begin(), rig.skeleton.joints.end(),
                                   [](const Joint& joint) { return lower(joint.name) == "head"; });
    if (head != rig.skeleton.joints.end()) rig.headJoint = std::size_t(head - rig.skeleton.joints.begin());

    rig.bodyPath = bodyPath;
    rig.height = start->position[1];
    rig.motion.x = start->position[0];
    rig.motion.z = start->position[2];
    rig.motion.facing = headingFromDegrees(start->orientation[1]);
    rig.motion.lastIntent = rig.motion.facing;
    rig.startX = rig.motion.x;
    rig.startZ = rig.motion.z;
    if (!scene.addModel(bodyPath, rig.matrix(), bodyTextures)) throw std::runtime_error("not in archive: " + bodyPath);
    rig.bodyInstance = scene.instances() - 1;
    if (!scene.addModel(headPath, rig.matrix(), headTextures)) throw std::runtime_error("not in archive: " + headPath);
    rig.headInstance = scene.instances() - 1;
    rig.pose(scene);
    std::cout << "player at " << start->position[0] << ' ' << start->position[1] << ' ' << start->position[2]
              << ", yaw " << start->orientation[1] << ", character " << character.characterEnum << " ("
              << character.head << ")\n";
    return rig;
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
    std::optional<PlayerRig> player = placePlayer(archive, scene, level);

    float centre[3], radius;
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    PlaceholderCamera camera;
    camera.frameScene(centre, radius);
    if (player) {
        camera.yaw = runtime::headingRadians(player->motion.facing);
        const float target[3] = {player->motion.x, player->height, player->motion.z};
        camera.follow(target);
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    chase::render::setFrontFace(true);

    runtime::PadDevice pad;
    const std::uint64_t counterFrequency = SDL_GetPerformanceFrequency();
    const std::uint64_t counterOrigin = SDL_GetPerformanceCounter();
    auto now = [&]() { return clockTicks(SDL_GetPerformanceCounter() - counterOrigin, counterFrequency); };
    runtime::Scheduler scheduler({}, now());

    std::vector<ScriptStep> script;
    if (!options.script.empty()) {
        if (!player) throw std::runtime_error("--script needs a level with a player start");
        script = parseScript(options.script);
    }
    unsigned scriptLength = 0;
    for (const ScriptStep& step : script) scriptLength += step.updates;

    // One update of the game: the player reads the pad and the camera yaw first, then the camera moves
    // (the heading of the recorded traces is reproduced with the yaw from before the camera step).
    unsigned updates = 0, updatesThisSecond = 0;
    auto simulate = [&](float dt, const runtime::PadState& padInput) {
        pad.poll(padInput);
        if (player) {
            runtime::StickIntentInput stick;
            stick.slot14 = {pad.slot(kSlotStickX).value, pad.slot(kSlotStickX).deadzone};
            stick.slot15 = {pad.slot(kSlotStickY).value, pad.slot(kSlotStickY).deadzone};
            stick.entityType = kPlayerEntityType;
            stick.cameraYaw = camera.yaw;
            const runtime::LocomotionStep step = runtime::locomotionUpdate(player->motion, dt, runtime::stickIntent(stick));
            player->animate(scene, dt, step);
        }
        camera.update(dt, pad);
        if (player) {
            const float target[3] = {player->motion.x, player->height, player->motion.z};
            camera.follow(target);
        }
        ++updates;
        ++updatesThisSecond;
    };
    auto scriptedPad = [&](unsigned index) {
        runtime::PadState scripted;
        for (const ScriptStep& step : script) {
            if (index < step.updates) {
                scripted.stickX = std::int16_t(step.stickX);
                scripted.stickY = std::int16_t(step.stickY);
                break;
            }
            index -= step.updates;
        }
        return scripted;
    };

    bool running = true;
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
        if (script.empty()) {
            const runtime::PadState state = padState(input);
            scheduler.advance(now(), [&](float dt) { simulate(dt, state); });
        } else if (updates < scriptLength) {
            simulate(kScriptDt, scriptedPad(updates));
        }

        SDL_GL_GetDrawableSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.16f, 0.17f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        beginFrame(program, 1.0f);
        Mat4 mvp = chase::render::multiply(
            perspective(1.0f, float(width) / float(std::max(height, 1)), radius * 0.002f, radius * 20),
            chase::render::view(camera.eye, camera.yaw, camera.pitch));
        scene.draw(program, mvp);

        const bool scriptDone = !script.empty() && updates >= scriptLength;
        if (scriptDone) {
            const float moveX = player->motion.x - player->startX, moveZ = player->motion.z - player->startZ;
            std::cout << "scripted: " << updates << " updates, player moved " << std::hypot(moveX, moveZ)
                      << " units, facing " << player->motion.facing << ", run timer " << player->motion.runTimer << '\n';
        }
        if (!options.screenshot.empty() && (script.empty() ? updates >= options.frames : scriptDone)) {
            glFinish();
            saveScreenshot(options.screenshot, width, height);
            std::cout << "saved " << options.screenshot << " after " << updates << " update(s)\n";
            running = false;
        }
        if (scriptDone) running = false;
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

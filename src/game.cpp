// chase-game: development runtime skeleton. Not the playable game.
// Joins the recovered frame scheduler, pad pipeline and player locomotion (chase_runtime), the controls
// (chase_input) and the development renderer (chase_render) into one loop. The player character walks,
// runs and sprints with the recovered follow camera; the level's static collision keeps him on the floor and
// stops him at walls, and he falls off ledges, jumps and lands. The camera keeps out of the level's static
// collision. There is no gameplay.
#include "assets/assets.hpp"
#include "input.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "render/studs.hpp"
#include "runtime/collision.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/pad.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/player_vertical.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/studs.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <array>
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
// Development stand-in, not the original: a keyboard has no half deflection, so holding Walk (Left Alt) tilts the
// stick to this length (raw 3DS units), which is inside the walk regime (walk below raw length 117).
constexpr int kStickWalkDeflection = 90;
constexpr float kScriptDt = 0.033f;
constexpr std::size_t kSlotLeftShoulder = 12, kSlotRightShoulder = 13, kSlotStickX = 14, kSlotStickY = 15;
constexpr std::size_t kSlotButtonX = 8, kSlotButtonY = 9, kSlotJumpButton = 7;

// The class whose CharacterEnum 9 row ("police") is the one character the completion count excludes,
// taken as the starting character (inference). The idle animation is picked by file name only.
constexpr const char* kPlayerCharacter = "levels/chars/char_police.blvl";
constexpr const char* kPlayerBaseCharacter = "levels/chars/char_player.blvl";
constexpr const char* kPlayerAnimationDirectory = "models/char_minifig/class_police/";
constexpr const char* kPlayerAnimationSuffix = "_minifig.bfnanm";
// ctrl+0x6c of the player (read in the oracle, locomotion traces).
constexpr std::uint16_t kPlayerEntityType = 3;

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
    bool cameraCollision = true;
    std::optional<std::array<float, 3>> start;
};

struct ScriptStep {
    int stickX = 0, stickY = 0;
    unsigned updates = 0;
    std::uint32_t buttons = 0;
};

// "x y count [buttons];...": raw circle-pad values (+x right, +y up, 145 = full) held for count updates, with
// optional 3DS button letters (A B X Y L R) held as well.
std::vector<ScriptStep> parseScript(const std::string& text) {
    std::vector<ScriptStep> steps;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, ';')) {
        std::stringstream fields(part);
        ScriptStep step;
        if (!(fields >> step.stickX >> step.stickY >> step.updates)) throw std::runtime_error("bad --script step: " + part);
        std::string letters;
        fields >> letters;
        for (const char letter : letters) {
            const std::pair<char, runtime::PadButton> buttons[] = {{'A', runtime::PadA}, {'B', runtime::PadB}, {'X', runtime::PadX},
                                                                  {'Y', runtime::PadY}, {'L', runtime::PadL}, {'R', runtime::PadR}};
            const auto found = std::find_if(std::begin(buttons), std::end(buttons), [&](const auto& entry) { return entry.first == letter; });
            if (found == std::end(buttons)) throw std::runtime_error("bad button in --script step: " + part);
            step.buttons |= found->second;
        }
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
        else if (arg == "--no-camera-collision") o.cameraCollision = false;
        else if (arg == "--start") {
            std::stringstream fields(value());
            std::array<float, 3> position;
            if (!(fields >> position[0] >> position[1] >> position[2])) throw std::runtime_error("bad --start, expected \"X Y Z\"");
            o.start = position;
        }
        else throw std::runtime_error("usage: chase-game [--fib F] [--names CSV] [--config CFG] "
                                      "[--level levels/levels/<name>/<name>_main.blvl] [--screenshot OUT.bmp] [--frames N] "
                                      "[--script \"X Y COUNT [BUTTONS];...\"] [--start \"X Y Z\"] [--no-camera-collision]");
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
        const double deflection = input.active(Action::Walk) ? kStickWalkDeflection : kStickFullDeflection;
        state.stickX = std::int16_t(std::lround(x / length * deflection));
        state.stickY = std::int16_t(std::lround(y / length * deflection));
    }
    return state;
}

// Development free camera for levels without a player start (not game logic). With a player the recovered
// FollowCamera is used.
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

    void update(float dt, const runtime::PadDevice& pad) {
        yaw += 1.5f * dt * (pad.slot(kSlotButtonY).value - pad.slot(kSlotButtonX).value);
        float strafe = pad.slot(kSlotStickX).value, forward = pad.slot(kSlotStickY).value;
        float rise = pad.slot(kSlotRightShoulder).value - pad.slot(kSlotLeftShoulder).value;
        float step = speed * dt;
        float axisForward[3], axisRight[3];
        chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
        for (int i = 0; i < 3; ++i) eye[i] += step * (forward * axisForward[i] + strafe * axisRight[i]);
        eye[1] += step * rise;
    }
};

CharacterAppearance readCharacter(FuseArchive& archive, const std::string& path) {
    const FuseEntry* entry = archive.find(path);
    if (!entry) throw std::runtime_error("not in archive: " + path);
    return characterAppearance(parseLevel(archive.read(*entry)));
}

// The player character: locomotion state, skeleton, role animations and the scene instances it moves.
struct PlayerRig {
    runtime::LocomotionState motion;
    runtime::VerticalState vertical;
    runtime::JumpProfile jumpStanding, jumpMoving;
    bool jumpMovingAnimation = false;
    CharacterRig character;
    CharacterInstance instance;
    std::map<int, Animation> animations;  // 0 = idle, runtime::kAnimationWalk / Run / Sprint
    int animation = 0;
    double frame = 0;
    float startX = 0, startZ = 0;

    Mat4 matrix() const {
        const float heading = runtime::headingRadians(motion.facing), s = std::sin(heading), c = std::cos(heading);
        return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, motion.x, vertical.y, motion.z, 1};
    }

    void animate(Scene& scene, float dt, int role, float scale) {
        if (role != animation) {
            const auto locomotionRole = [](int id) { return id == 0 || id == runtime::kAnimationWalk || id == runtime::kAnimationRun || id == runtime::kAnimationSprint; };
            const bool keepPhase = locomotionRole(role) && locomotionRole(animation);
            const double phase = frame / double(animations.at(animation).frames);
            animation = role;
            frame = keepPhase ? phase * double(animations.at(animation).frames) : 0.0;
        }
        const Animation& current = animations.at(animation);
        const double rate = double(animationFramesPerSecond(current));
        frame = std::fmod(frame + double(dt) * rate * double(scale), double(current.frames));
        pose(scene);
    }

    void pose(Scene& scene) const {
        const Mat4 player = matrix();
        const std::vector<Matrix> skin = poseBody(scene, character, instance, animations.at(animation), frame);
        placeCharacter(scene, character, instance, skin, player);
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
    PlayerRig rig;
    rig.character = loadCharacterRig(archive, meshModelPath(character.body));
    const std::pair<int, const char*> roles[] = {
        {0, "idle"}, {runtime::kAnimationWalk, "walk"}, {runtime::kAnimationRun, "run"}, {runtime::kAnimationSprint, "sprint"},
        {runtime::kAnimationFall, "fall"}, {runtime::kAnimationLand, "land"}, {runtime::kAnimationFallLand, "fallland"},
        {runtime::kAnimationJump, "jump"}, {runtime::kAnimationJumpRun, "jump_run"}};
    for (const auto& [id, name] : roles)
        rig.animations[id] = loadAnimation(archive, std::string(kPlayerAnimationDirectory) + name + kPlayerAnimationSuffix);
    rig.jumpStanding = runtime::makeJumpProfile(rig.animations.at(runtime::kAnimationJump));
    rig.jumpMoving = runtime::makeJumpProfile(rig.animations.at(runtime::kAnimationJumpRun));
    rig.vertical.landingDurations =
        runtime::landingDurations(rig.animations.at(runtime::kAnimationLand), rig.animations.at(runtime::kAnimationFallLand));

    rig.vertical.y = start->position[1];
    rig.motion.x = start->position[0];
    rig.motion.z = start->position[2];
    rig.motion.facing = headingFromDegrees(start->orientation[1]);
    rig.motion.lastIntent = rig.motion.facing;
    rig.startX = rig.motion.x;
    rig.startZ = rig.motion.z;
    rig.instance = addCharacter(scene, base, character, "", "player", rig.matrix());
    rig.pose(scene);
    std::cout << "player at " << start->position[0] << ' ' << start->position[1] << ' ' << start->position[2]
              << ", yaw " << start->orientation[1] << ", character " << character.characterEnum << " ("
              << character.head << ")\n";
    return rig;
}

// The level's static collision: the items of the level's master model (models/<level>/<level>.bfnmdl).
runtime::CollisionWorld loadCollision(FuseArchive& archive, const std::string& levelPath) {
    runtime::CollisionWorld world;
    const std::string path = lower(levelPath);
    const std::string folder = path.substr(0, path.find_last_of('/'));
    const std::string name = folder.substr(folder.find_last_of('/') + 1);
    const FuseEntry* entry = archive.find("models/" + name + "/" + name + ".bfnmdl");
    if (!entry) return world;
    for (const CollisionItem& item : parseModel(archive.read(*entry)).collision) world.add(item);
    return world;
}

std::uint32_t clockTicks(std::uint64_t counter, std::uint64_t frequency) {
    std::uint64_t seconds = counter / frequency, rest = counter % frequency;
    return std::uint32_t(seconds * runtime::kClockFrequency + rest * runtime::kClockFrequency / frequency);
}

struct PlayerAnimation {
    int role = 0;
    float scale = 1.0f;
};

// One player update: locomotion, wall slide (0x21a73c), then the vertical step (jump, ground probe, fall, landing).
// A landing stun suppresses locomotion; in the air the stick keeps its walk or run speed, times 0.8 after a jump.
PlayerAnimation updatePlayer(PlayerRig& player, const runtime::CollisionWorld& collision, const runtime::StickIntent& intent,
                             bool jumpPressed, float dt) {
    using runtime::Posture;
    runtime::VerticalInput input;
    input.dt = dt;
    input.moveRequested = intent.active;
    const auto landingAnimation = [&]() {
        return PlayerAnimation{player.vertical.hardLanding ? runtime::kAnimationFallLand : runtime::kAnimationLand, 1.0f};
    };
    if (player.vertical.posture == Posture::Landing) {
        runtime::verticalUpdate(player.vertical, collision, input);
        return landingAnimation();
    }
    const bool jumping = player.vertical.posture == Posture::Jumping;
    const runtime::RootMotionSpeeds speeds =
        jumping ? runtime::jumpSpeeds(runtime::kCharacterSpeeds) : runtime::kCharacterSpeeds;
    const runtime::LocomotionState before = player.motion;
    const float startX = before.x, startZ = before.z;
    runtime::LocomotionStep step = runtime::locomotionUpdate(player.motion, dt, intent, speeds);
    if (jumping) {
        player.motion.runTimer = before.runTimer;
        player.motion.wasRunning = before.wasRunning;
        player.motion.sprinting = before.sprinting;
        step.animationScale = before.sprinting ? runtime::kCharacterSpeeds.sprintScale : 1.0f;
    }
    float dx = step.deltaX, dz = step.deltaZ;
    const float rootDelta = std::hypot(dx, dz);
    const float heading = runtime::headingRadians(player.motion.facing);
    input.forwardX = std::sin(heading);
    input.forwardZ = std::cos(heading);
    if (rootDelta > 0 && !collision.empty())
        runtime::slideAlongWalls(collision, startX, player.vertical.y, startZ, input.forwardX, input.forwardZ, dx, dz);
    player.motion.x = startX + dx;
    player.motion.z = startZ + dz;
    input.x = player.motion.x;
    input.z = player.motion.z;
    input.rootDelta = rootDelta;
    if (jumpPressed && player.vertical.posture == Posture::Ground) {
        player.jumpMovingAnimation = intent.active;
        input.startJump = intent.active ? &player.jumpMoving : &player.jumpStanding;
    }
    const runtime::VerticalStep vertical = runtime::verticalUpdate(player.vertical, collision, input);
    if (vertical.landed && player.vertical.posture == Posture::Ground && intent.active && step.regime == runtime::Regime::Run) {
        player.motion.runTimer = (player.motion.sprinting ? runtime::kSprintTime : 0.0f) + dt;
        player.motion.wasRunning = true;
        player.motion.sprinting = player.motion.runTimer > runtime::kSprintTime;
    }
    if (player.vertical.posture == Posture::Falling) {
        player.motion.runTimer = 0;
        player.motion.wasRunning = false;
        player.motion.sprinting = false;
        return {runtime::kAnimationFall, 1.0f};
    }
    if (player.vertical.posture == Posture::Jumping || vertical.jumpStarted)
        return {player.jumpMovingAnimation ? runtime::kAnimationJumpRun : runtime::kAnimationJump, 1.0f};
    if (player.vertical.posture == Posture::Landing) return landingAnimation();
    return {step.animation, step.animationScale};
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
    const DrawableObjects drawable = drawableObjects(archive, level);
    for (const auto& placement : drawable.objects)
        if (scene.addModel(meshModelPath(placement.mesh), placementMatrix(placement))) ++placed;
        else ++unresolved;
    const std::size_t characters = addLevelCharacters(scene, archive, drawable.characters);
    std::vector<runtime::StudSpawn> studSpawns;
    for (const StudObject& stud : drawable.studs) {
        runtime::StudSpawn spawn;
        spawn.kind = runtime::StudKind(stud.kind);
        std::copy_n(stud.position, 3, spawn.position);
        spawn.group = stud.group;
        studSpawns.push_back(spawn);
    }
    runtime::StudField studs(studSpawns);
    std::optional<StudSprites> studSprites(std::in_place, archive);
    double provisionalStudClockSeconds = 0;
    std::optional<PlayerRig> player = placePlayer(archive, scene, level);
    if (player && options.start) {
        player->motion.x = player->startX = (*options.start)[0];
        player->vertical.y = (*options.start)[1];
        player->motion.z = player->startZ = (*options.start)[2];
        player->pose(scene);
    }
    const runtime::CollisionWorld collision = loadCollision(archive, options.level);
    std::cout << "collision: " << collision.triangleCount() << " triangles\n";

    float centre[3], radius;
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames, " << characters << " characters standing in place, "
              << studs.studs().size() << " studs (" << studs.count(runtime::StudState::Idle) << " visible), "
              << drawable.missingModel << " without model in the archive, " << drawable.skeletal
              << " other skeletal, " << drawable.customCharacters << " custom character and " << drawable.markers
              << " marker objects skipped), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    PlaceholderCamera camera;
    camera.frameScene(centre, radius);
    std::optional<runtime::FollowCamera> follow;
    if (player) {
        follow.emplace();
        if (options.cameraCollision) follow->setCollision(&collision);
        const float target[3] = {player->motion.x, player->vertical.y, player->motion.z};
        follow->reset(target, runtime::headingRadians(player->motion.facing), runtime::headingRadians(player->motion.facing));
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
    std::vector<float> scriptedHeights;
    auto simulate = [&](float dt, const runtime::PadState& padInput) {
        pad.poll(padInput);
        if (player) {
            runtime::StickIntentInput stick;
            stick.slot14 = {pad.slot(kSlotStickX).value, pad.slot(kSlotStickX).deadzone};
            stick.slot15 = {pad.slot(kSlotStickY).value, pad.slot(kSlotStickY).deadzone};
            stick.entityType = kPlayerEntityType;
            stick.cameraYaw = follow->yaw();
            const PlayerAnimation shown = updatePlayer(*player, collision, runtime::stickIntent(stick), pad.slot(kSlotJumpButton).press > 0, dt);
            player->animate(scene, dt, shown.role, shown.scale);
            if (!script.empty()) scriptedHeights.push_back(player->vertical.y);
            studs.update(dt, runtime::studPickupPoint(player->motion.x, player->vertical.y, player->motion.z,
                                                      runtime::headingRadians(player->motion.facing)));
        }
        provisionalStudClockSeconds += double(dt);
        if (follow) {
            runtime::FollowCameraInput input;
            input.dt = dt;
            input.target[0] = player->motion.x;
            input.target[1] = player->vertical.y;
            input.target[2] = player->motion.z;
            input.targetFacing = runtime::headingRadians(player->motion.facing);
            input.stickTurn = runtime::stickTurnInput(pad.slot(kSlotStickX).value, kPlayerEntityType);
            input.shoulderTurn = runtime::shoulderTurnInput(pad.slot(kSlotLeftShoulder).value, pad.slot(kSlotRightShoulder).value, false);
            follow->update(input);
        } else {
            camera.update(dt, pad);
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
                scripted.held = step.buttons;
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
        const float* eye = follow ? follow->eye() : camera.eye;
        const float viewYaw = follow ? follow->yaw() : camera.yaw, viewPitch = follow ? follow->pitch() : camera.pitch;
        const float fieldOfView = follow ? runtime::FollowCamera::verticalFieldOfView() : 1.0f;
        const Mat4 viewMatrix = chase::render::view(eye, viewYaw, viewPitch);
        Mat4 mvp = chase::render::multiply(
            perspective(fieldOfView, float(width) / float(std::max(height, 1)), radius * 0.002f, radius * 20), viewMatrix);
        scene.draw(program, mvp);
        std::vector<StudSprite> sprites;
        for (const runtime::Stud& stud : studs.studs())
            if (stud.state == runtime::StudState::Idle)
                sprites.push_back({{stud.centre[0], stud.centre[1], stud.centre[2]}, runtime::kStudTint[std::size_t(stud.kind)]});
        studSprites->draw(program, viewMatrix, mvp, sprites, runtime::studSpriteFrame(std::uint32_t(provisionalStudClockSeconds * double(runtime::kClockTicksPerSecond))),
                         runtime::kStudHalfSize);

        const bool scriptDone = !script.empty() && updates >= scriptLength;
        if (scriptDone) {
            const float moveX = player->motion.x - player->startX, moveZ = player->motion.z - player->startZ;
            std::cout << "scripted: " << updates << " updates, player moved " << std::hypot(moveX, moveZ)
                      << " units, facing " << player->motion.facing << ", run timer " << player->motion.runTimer << ", height " << player->vertical.y << '\n';
            std::cout << "studs: counter " << studs.counter() << ", " << studs.count(runtime::StudState::Collecting) << " collecting, "
                      << studs.count(runtime::StudState::Done) << " collected, " << studs.count(runtime::StudState::Idle) << " left\n";
            std::cout << "heights:";
            for (const float y : scriptedHeights) std::cout << ' ' << y;
            std::cout << '\n';
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
    studSprites.reset();
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

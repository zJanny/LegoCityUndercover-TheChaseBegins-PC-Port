// chase-game: development runtime skeleton. Not the playable game.
// Joins the recovered frame scheduler, pad pipeline and player locomotion (chase_runtime), the controls
// (chase_input) and the development renderer (chase_render) into one loop. The player character walks,
// runs and sprints with the recovered follow camera; the level's static collision keeps him on the floor and
// stops him at walls, and he falls off ledges, jumps and lands. The camera keeps out of the level's static
// collision. There is no gameplay.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "game_audio.hpp"
#include "frontend/hud.hpp"
#include "frontend/minimap.hpp"
#include "game_front.hpp"
#include "game_hud.hpp"
#include "input.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "render/studs.hpp"
#include "runtime/collision.hpp"
#include "runtime/door.hpp"
#include "runtime/camera_lock_icon.hpp"
#include "runtime/camera_rig.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/level_start.hpp"
#include "runtime/pad.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/player_vertical.hpp"
#include "runtime/music.hpp"
#include "runtime/player_motion.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/sound_events.hpp"
#include "runtime/studs.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
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
namespace fe = chase::frontend;

constexpr int kStickFullDeflection = 145;
// Development stand-in, not the original: a keyboard has no half deflection, so holding Walk (Left Alt) tilts the
// stick to this length (raw 3DS units), which is inside the walk regime (walk below raw length 117).
constexpr int kStickWalkDeflection = 90;
constexpr float kScriptDt = 0.033f;
constexpr std::size_t kSlotLeftShoulder = 12, kSlotRightShoulder = 13, kSlotStickX = 14, kSlotStickY = 15;
constexpr std::size_t kSlotButtonX = 8, kSlotButtonY = 9, kSlotJumpButton = 7;

// Stand-ins, not the original: the water flag (ctrl+0x317 bit 3) and the ground object of type 11 are not modelled, so
// the landing always plays the default surface sound (docs/re/audio.md).
constexpr bool kProvisionalWaterLanding = false, kProvisionalObject11Landing = false;
// The sprint animation plays at its own 30 frames per second: the recorded footsteps are 10.14 updates apart, which
// is half a cycle at scale 1.0, not at the movement scale ctrl+0x94 = 1.5 (docs/re/audio.md).
constexpr float kSprintAnimationScale = 1.0f;

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
    std::optional<float> facing;
    // Options volumes 0..10 (the game divides them by 10). Read in the oracle at the gameplay start of a new game:
    // sound effects 1.0 (0x37636c + 4), music 0.8 (0x376b3c); the saved options are not read yet.
    unsigned soundVolume = 10, musicVolume = 8;
    // Front-end mode is the default: boot, language, prompts, slot, intro movie, then the level. --gameplay, --script,
    // --level and --start describe a level run and start in the level directly.
    bool gameplay = false;
    std::string frontScript;
    unsigned language = 1;
    bool returningPlayer = false;
    int windowWidth = 960, windowHeight = 600;
};

// A new game starts in Cherry Tree Hills at the position of the player entity in the oracle at gameplay-start
// (docs/re/oracle.md); the missions that place him there are not recovered. STAND-IN.
constexpr const char* kNewGameLevel = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr std::array<float, 3> kNewGameStart = {886.764f, 16.589f, 1111.166f};

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
        else if (arg == "--level") { o.level = value(); o.gameplay = true; }
        else if (arg == "--gameplay") o.gameplay = true;
        else if (arg == "--front-script") o.frontScript = value();
        else if (arg == "--language") o.language = std::max(1u, std::min(9u, unsigned(std::stoul(value()))));
        else if (arg == "--returning") o.returningPlayer = true;
        else if (arg == "--window") {
            std::stringstream fields(value());
            char separator = 0;
            if (!(fields >> o.windowWidth >> separator >> o.windowHeight) || separator != 'x' || o.windowWidth < 200 || o.windowHeight < 240)
                throw std::runtime_error("bad --window, expected WIDTHxHEIGHT");
        }
        else if (arg == "--screenshot") o.screenshot = value();
        else if (arg == "--frames") o.frames = unsigned(std::stoul(value()));
        else if (arg == "--script") { o.script = value(); o.gameplay = true; }
        else if (arg == "--no-camera-collision") o.cameraCollision = false;
        else if (arg == "--sound-volume") o.soundVolume = std::min(10u, unsigned(std::stoul(value())));
        else if (arg == "--music-volume") o.musicVolume = std::min(10u, unsigned(std::stoul(value())));
        else if (arg == "--facing") o.facing = std::stof(value());
        else if (arg == "--start") {
            std::stringstream fields(value());
            std::array<float, 3> position;
            if (!(fields >> position[0] >> position[1] >> position[2])) throw std::runtime_error("bad --start, expected \"X Y Z\"");
            o.start = position;
            o.gameplay = true;
        }
        else throw std::runtime_error("usage: chase-game [--fib F] [--names CSV] [--config CFG] [--gameplay] "
                                      "[--level levels/levels/<name>/<name>_main.blvl] [--screenshot OUT.bmp] [--frames N] "
                                      "[--script \"X Y COUNT [BUTTONS];...\"] [--start \"X Y Z\"] [--facing DEGREES] [--no-camera-collision] [--sound-volume 0-10] [--music-volume 0-10] "
                                      "[--front-script \"run 5; press A; until slot; ...\"] [--language 1-9] [--returning] [--window WIDTHxHEIGHT]");
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
    runtime::FootstepDetector footsteps;
    runtime::CharacterSounds characterSounds;
    std::vector<runtime::SoundCall> sounds;  // the sound calls of the update, filled by updatePlayer and animate

    std::array<float, 3> position() const { return {motion.x, vertical.y, motion.z}; }

    Mat4 matrix() const {
        const float heading = runtime::headingRadians(motion.facing), s = std::sin(heading), c = std::cos(heading);
        return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, motion.x, vertical.y, motion.z, 1};
    }

    void animate(Scene& scene, float dt, int role, float scale) {
        const bool switched = role != animation;
        if (switched) {
            const auto locomotionRole = [](int id) { return id == runtime::kAnimationWalk || id == runtime::kAnimationRun || id == runtime::kAnimationSprint; };
            const bool keepPhase = locomotionRole(role) && locomotionRole(animation);
            const double phase = frame / double(animations.at(animation).frames);
            animation = role;
            frame = keepPhase ? std::floor(phase * double(animations.at(animation).frames)) : 0.0;
        }
        const Animation& current = animations.at(animation);
        const double rate = double(animationFramesPerSecond(current));
        frame = std::fmod(frame + double(dt) * rate * double(scale), double(current.frames));
        pose(scene);
        const float value = runtime::footstepTrackValue(current, float(frame / double(current.frames)));
        if (switched) footsteps.restart(value);
        else if (footsteps.update(value)) sounds.push_back(characterSounds.footstep(position()));
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
    rig.vertical.landingAnimations =
        runtime::landingAnimations(rig.animations.at(runtime::kAnimationLand), rig.animations.at(runtime::kAnimationFallLand));
    const FuseEntry* soundTable = archive.find("tables/tablessoundfx.bxls");
    const FuseEntry* characterTable = archive.find("tables/tablescharacters.bxls");
    if (soundTable && characterTable) {
        const SoundTables sounds = parseSoundTables(parseTable(archive.read(*soundTable)));
        const std::uint8_t category = characterSoundCategory(parseTable(archive.read(*characterTable)), character.characterEnum);
        if (category < sounds.characterSounds.size()) rig.characterSounds = runtime::CharacterSounds(sounds.characterSounds[category]);
    }

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

// The level's collision: the items of the level's master model (models/<level>/<level>.bfnmdl) and the placed objects.
runtime::CollisionWorld loadCollision(FuseArchive& archive, const std::string& levelPath, const std::vector<ObjectCollision>& objects) {
    runtime::CollisionWorld world;
    const std::string path = lower(levelPath);
    const std::string folder = path.substr(0, path.find_last_of('/'));
    const std::string name = folder.substr(folder.find_last_of('/') + 1);
    const FuseEntry* entry = archive.find("models/" + name + "/" + name + ".bfnmdl");
    if (!entry) return world;
    for (const CollisionItem& item : parseModel(archive.read(*entry)).collision) world.add(item);
    for (const ObjectCollision& object : objects) world.addObject(object);
    return world;
}

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

// A door object (classes props/prop_door and leprops/leprop_door): its state machine, the scene instance of its model and the matrices
// that put its node in place (docs/re/doors.md). The node matrix at model+0x20 is applied before the pose and the placement; that order
// is not verified against the original's picture.
struct DoorObject {
    std::string instance;
    runtime::DoorParams params;
    runtime::DoorState state;
    std::size_t sceneInstance[2] = {0, 0};
    bool separateLeaves = false;
    Mat4 placement{};
    Mat4 rest[2] = {identity(), identity()};
    float pivot[2][3] = {};
    std::array<float, 3> position{};
    std::uint16_t soundOwner = 0;
};

std::vector<DoorObject> loadDoors(Scene& scene, FuseArchive& archive, const Level& level, const ClassLookup& classOf, const std::vector<ObjectCollision>& objects,
                                  const std::map<std::string, std::pair<std::size_t, Mat4>>& instances) {
    std::vector<DoorObject> doors;
    for (const ObjectCollision& object : objects) {
        if (!isDoorClass(object.className)) continue;
        const auto instance = instances.find(object.instance);
        if (instance == instances.end() || object.model.empty()) continue;
        const LevelObject* source = nullptr;
        for (const LevelObject& candidate : level.objects)
            if (candidate.instance == object.instance) source = &candidate;
        const FuseEntry* modelEntry = archive.find(object.model);
        if (!source || !modelEntry) continue;
        const Model model = parseModel(archive.read(*modelEntry));
        DoorObject door;
        door.instance = object.instance;
        door.soundOwner = std::uint16_t(doors.size() + 1);
        door.params = runtime::doorParams(*source, *classOf(source->className), model, object.centre, object.half);
        door.sceneInstance[0] = instance->second.first;
        door.placement = instance->second.second;
        for (std::size_t n = 0; n < model.nodeMatrices.size() && n < 2; ++n)
            if (model.nodeMatrices[n][15] != 0.0f) door.rest[n] = model.nodeMatrices[n];
        if (door.params.twoLeaves && model.nodeMesh.size() > 1 && model.nodeMesh[0] >= 0 && model.nodeMesh[1] >= 0 &&
            scene.setInstanceMesh(door.sceneInstance[0], object.model, model.nodeMesh[0])) {
            door.sceneInstance[1] = scene.instances();
            door.separateLeaves = scene.addModel(object.model, door.placement, {}, object.model + "#" + std::to_string(model.nodeMesh[1]), model.nodeMesh[1]);
        }
        for (std::size_t n = 0; n < model.nodePivot.size() && n < 2; ++n) std::copy_n(model.nodePivot[n].data(), 3, door.pivot[n]);
        door.position = {door.placement[12], door.placement[13], door.placement[14]};
        doors.push_back(std::move(door));
    }
    return doors;
}

void poseDoor(Scene& scene, const DoorObject& door) {
    const runtime::DoorPose pose = runtime::doorPose(door.state, door.params, door.pivot);
    for (int leaf = 0; leaf < (door.separateLeaves ? 2 : 1); ++leaf)
        scene.setTransform(door.sceneInstance[leaf], chase::render::multiply(door.placement, chase::render::multiply(pose.node[leaf], door.rest[leaf])));
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
    const runtime::JumpProfiles jumps{&player.jumpStanding, &player.jumpMoving};
    const runtime::PlayerMotionStep update =
        runtime::playerMotionUpdate(player.motion, player.vertical, jumps, collision, intent, jumpPressed, dt);
    const runtime::VerticalStep& vertical = update.vertical;
    const auto landingAnimation = [&]() {
        return PlayerAnimation{player.vertical.hardLanding ? runtime::kAnimationFallLand : runtime::kAnimationLand, 1.0f};
    };
    if (update.stunned) return landingAnimation();
    if (vertical.jumpStarted) {
        player.jumpMovingAnimation = update.jumpMoving;
        player.sounds.push_back(player.characterSounds.jump(player.position()));
    }
    if (vertical.landed) {
        const std::vector<runtime::SoundCall> calls =
            vertical.hard ? player.characterSounds.hardLanding(player.position())
                          : player.characterSounds.landing(update.fallBefore, kProvisionalWaterLanding, kProvisionalObject11Landing, player.position());
        player.sounds.insert(player.sounds.end(), calls.begin(), calls.end());
    }
    if (player.vertical.posture == Posture::Falling || player.vertical.posture == Posture::Sliding) return {runtime::kAnimationFall, 1.0f};
    if (player.vertical.posture == Posture::Jumping || vertical.jumpStarted)
        return {player.jumpMovingAnimation ? runtime::kAnimationJumpRun : runtime::kAnimationJump, 1.0f};
    if (player.vertical.posture == Posture::Landing) return landingAnimation();
    const runtime::LocomotionStep& step = update.locomotion;
    return {step.animation, step.animation == runtime::kAnimationSprint ? kSprintAnimationScale : step.animationScale};
}

int run(const Options& given) {
    Options options = given;
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
    int width = options.windowWidth, height = options.windowHeight;
    SDL_Window* window = SDL_CreateWindow("The Chase Begins - development runtime (not the playable game)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) throw std::runtime_error(SDL_GetError());
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
        const game_front::Result result =
            game_front::run(window, input, archive, audio ? &*audio : nullptr, front,
                            [&]() { return clockTicks(SDL_GetPerformanceCounter() - frontOrigin, frontFrequency); });
        if (result.quit || result.scriptOnly) {
            audio.reset();
            SDL_GL_DeleteContext(context);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 0;
        }
        options.level = kNewGameLevel;
        options.start = kNewGameStart;
    }
    GLuint program = createProgram();
    Scene scene(archive, {"sky"});
    for (const auto& model : levelModels(archive, options.level, ""))
        if (!scene.addModel(model)) throw std::runtime_error("not in archive: " + model);
    const FuseEntry* entry = archive.find(options.level);
    if (!entry) throw std::runtime_error("not in archive: " + options.level);
    std::size_t placed = 0, unresolved = 0;
    const Level level = parseLevel(archive.read(*entry));
    const DrawableObjects drawable = drawableObjects(archive, level);
    std::map<std::string, std::pair<std::size_t, Mat4>> placedInstances;
    for (const auto& placement : drawable.objects) {
        const std::size_t instanceIndex = scene.instances();
        const Mat4 matrix = placementMatrix(placement);
        if (scene.addModel(meshModelPath(placement.mesh), matrix)) {
            ++placed;
            placedInstances[placement.instance] = {instanceIndex, matrix};
        } else {
            ++unresolved;
        }
    }
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
    std::vector<std::pair<unsigned, std::uint16_t>> soundLog;
    std::optional<StudSprites> studSprites(std::in_place, archive);
    double provisionalStudClockSeconds = 0;
    std::optional<PlayerRig> player = placePlayer(archive, scene, level);
    if (player && options.start) {
        player->motion.x = player->startX = (*options.start)[0];
        player->vertical.y = (*options.start)[1];
        player->motion.z = player->startZ = (*options.start)[2];
        player->pose(scene);
    }
    if (player && options.facing) {
        player->motion.facing = headingFromDegrees(*options.facing);
        player->motion.lastIntent = player->motion.facing;
        player->pose(scene);
    }
    const ClassLookup classOf = archiveClassLookup(archive);
    PlacedObjectState objectState = runtime::levelInitialState(level, classOf);
    runtime::ScriptRunner levelScripts(level, objectState);
    levelScripts.setAllRoomsLoaded();
    std::size_t reportedUnhandled = 0, reportedEvents = 0;
    const auto reportUnhandled = [&] {
        const std::vector<std::string>& names = levelScripts.unhandledFunctions();
        for (; reportedUnhandled < names.size(); ++reportedUnhandled)
            std::cout << "script function without handler (continues at once): " << names[reportedUnhandled] << '\n';
        const std::vector<std::string>& events = levelScripts.unhandledTriggerEvents();
        for (; reportedEvents < events.size(); ++reportedEvents)
            std::cout << "script trigger event without check (never fires): " << events[reportedEvents] << '\n';
    };
    std::uint32_t startupUpdates = 0;
    if (levelScripts.start("startup"))
        while (levelScripts.running("startup", false) && startupUpdates < runtime::kStartupMaxUpdates) {
            levelScripts.update(runtime::kStartupStepDt);
            ++startupUpdates;
        }
    reportUnhandled();
    std::cout << "scripts: startup ended after " << startupUpdates << " updates, " << levelScripts.threadCount() << " threads running\n";
    const std::vector<ObjectCollision> placedCollisions = objectCollisions(archive, level, objectState);
    const runtime::CollisionWorld collision = loadCollision(archive, options.level, placedCollisions);
    std::vector<DoorObject> doors = loadDoors(scene, archive, level, classOf, placedCollisions, placedInstances);
    for (const DoorObject& door : doors) poseDoor(scene, door);
    std::cout << "doors: " << doors.size() << '\n';
    std::cout << "collision: " << collision.triangleCount() << " triangles in " << collision.entries().size() << " entries\n";

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
    std::optional<runtime::CameraRig> follow;
    runtime::CameraLockIcon cameraLockIcon;
    if (player) {
        std::string camText;
        if (const FuseEntry* camEntry = archive.find(options.level.substr(0, options.level.size() - 5) + ".cam")) {
            const Bytes camBytes = archive.read(*camEntry);
            camText.assign(camBytes.begin(), camBytes.end());
        }
        follow.emplace(level, camText);
        if (options.cameraCollision) follow->setCollision(&collision);
        const float target[3] = {player->motion.x, player->vertical.y, player->motion.z};
        follow->reset(target, runtime::headingRadians(player->motion.facing), runtime::headingRadians(player->motion.facing));
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    chase::render::setFrontFace(true);

    fe::Resources hudResources(archive);
    std::optional<fe::Hud> hud;
    if (player) {
        fe::HudConfig hudConfig;
        const std::string characterFile = kPlayerCharacter;
        const std::size_t nameStart = characterFile.find("char_") + 5;
        hudConfig.portrait = characterFile.substr(nameStart, characterFile.find(".blvl") - nameStart);
        hud.emplace(hudResources, hudConfig);
    }
    fe::Canvas hudCanvas(fe::kTopWidth, fe::kScreenHeight), mapCanvas(fe::kBottomWidth, fe::kScreenHeight);
    std::optional<game_hud::Overlay> overlay(std::in_place);
    const std::string levelFolder = lower(options.level).substr(0, lower(options.level).find_last_of('/'));
    const std::string levelName = levelFolder.substr(levelFolder.find_last_of('/') + 1);

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
        const auto emit = [&](const runtime::SoundCall& call) {
            soundLog.emplace_back(updates, call.id);
            if (audio) audio->play(call);
        };
        if (player) {
            player->sounds.clear();
            runtime::StickIntentInput stick;
            stick.slot14 = {pad.slot(kSlotStickX).value, pad.slot(kSlotStickX).deadzone};
            stick.slot15 = {pad.slot(kSlotStickY).value, pad.slot(kSlotStickY).deadzone};
            stick.entityType = kPlayerEntityType;
            stick.cameraYaw = follow->yaw();
            if (hud) {
                hud->update(dt);
                const fe::StudSink sink = hud->studSink();
                studs.setFlightTarget(runtime::unprojectTopScreen(sink.x, sink.y, fe::kStudSinkDepth, follow->eye(), follow->yaw(),
                                                                  follow->pitch(), follow->verticalFieldOfView()));
            }
            const PlayerAnimation shown = updatePlayer(*player, collision, runtime::stickIntent(stick), pad.slot(kSlotJumpButton).press > 0, dt);
            player->animate(scene, dt, shown.role, shown.scale);
            if (!script.empty()) scriptedHeights.push_back(player->vertical.y);
            for (const runtime::StudCollected& collected :
                 studs.update(dt, runtime::studPickupPoint(player->motion.x, player->vertical.y, player->motion.z,
                                                           runtime::headingRadians(player->motion.facing))))
                player->sounds.push_back(runtime::studCollectCall(collected, studs.studs()[collected.index].centre));
            for (const runtime::SoundCall& call : player->sounds) emit(call);
            if (hud)
                for (const runtime::StudBanked& banked : studs.takeBanked()) hud->studsBanked(unsigned(banked.kind), banked.total);
        }
        if (player) {
            const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
            std::vector<runtime::DoorSoundEvent> doorSounds;
            for (DoorObject& door : doors) {
                doorSounds.clear();
                runtime::doorUpdate(door.state, door.params, runtime::doorZoneContains(door.placement.data(), playerPosition, door.params), doorSounds);
                poseDoor(scene, door);
                for (const runtime::DoorSoundEvent& event : doorSounds) {
                    if (event.id == 0) continue;
                    if (event.action == runtime::DoorSoundEvent::Action::Play) emit({event.id, door.position, door.soundOwner});
                    else if (audio) audio->stop(event.id, door.soundOwner);
                }
            }
        }
        levelScripts.update(dt);
        if (player) {
            const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
            levelScripts.checkTriggers(playerPosition);
        }
        reportUnhandled();
        for (const runtime::ScriptObjectMessage& sent : levelScripts.takeObjectMessages())
            for (DoorObject& door : doors)
                if (lowerCase(door.instance) == sent.instance)
                    runtime::doorMessage(door.state, door.params,
                                         sent.message == runtime::kMessageTrigger ? runtime::DoorMessage::Open : runtime::DoorMessage::Close);
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
            if (follow->locked() && (pad.slot(kSlotLeftShoulder).value != 0.0f || pad.slot(kSlotRightShoulder).value != 0.0f))
                cameraLockIcon.trigger();
            cameraLockIcon.update(dt);
            if (hud) hud->setCameraLockAlpha(cameraLockIcon.alpha());
            if (audio) {
                const float* eye = follow->eye();
                const float* look = follow->lookAt();
                audio->setListener({eye[0], eye[1], eye[2]}, {look[0] - eye[0], look[1] - eye[1], look[2] - eye[2]});
                audio->update();
            }
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
        const game_hud::Screens screens = game_hud::screensFor(width, height);
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, width, height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        glScissor(screens.top.x, screens.top.y, screens.top.width, screens.top.height);
        glViewport(screens.top.x, screens.top.y, screens.top.width, screens.top.height);
        glClearColor(0.16f, 0.17f, 0.2f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        beginFrame(program, 1.0f);
        const float* eye = follow ? follow->eye() : camera.eye;
        const float viewYaw = follow ? follow->yaw() : camera.yaw, viewPitch = follow ? follow->pitch() : camera.pitch;
        const float fieldOfView = follow ? follow->verticalFieldOfView() : 1.0f;
        const Mat4 viewMatrix = chase::render::view(eye, viewYaw, viewPitch);
        Mat4 mvp = chase::render::multiply(
            perspective(fieldOfView, float(fe::kTopWidth) / float(fe::kScreenHeight), radius * 0.002f, radius * 20), viewMatrix);
        scene.draw(program, mvp);
        std::vector<StudSprite> sprites;
        for (const runtime::Stud& stud : studs.studs())
            if (stud.state == runtime::StudState::Idle || stud.state == runtime::StudState::Collecting)
                sprites.push_back({{stud.centre[0], stud.centre[1], stud.centre[2]}, runtime::kStudTint[std::size_t(stud.kind)]});
        studSprites->draw(program, viewMatrix, mvp, sprites, runtime::studSpriteFrame(std::uint32_t(provisionalStudClockSeconds * double(runtime::kClockTicksPerSecond))),
                         runtime::kStudHalfSize);
        glDisable(GL_SCISSOR_TEST);
        if (hud) {
            hudCanvas.clearTransparent();
            hud->draw(hudCanvas, provisionalStudClockSeconds);
            fe::MinimapView mapView;
            mapView.x = player->motion.x;
            mapView.z = player->motion.z;
            mapView.cameraYaw = follow->yaw();
            mapView.playerYaw = runtime::headingRadians(player->motion.facing);
            fe::drawMinimap(mapCanvas, hudResources, levelName, mapView);
        } else {
            hudCanvas.clearTransparent();
            mapCanvas.clear(0, 0, 0);
        }
        overlay->draw(hudCanvas, mapCanvas, screens, width, height);

        const bool scriptDone = !script.empty() && updates >= scriptLength;
        if (scriptDone) {
            const float moveX = player->motion.x - player->startX, moveZ = player->motion.z - player->startZ;
            std::cout << "scripted: " << updates << " updates, player moved " << std::hypot(moveX, moveZ)
                      << " units, facing " << player->motion.facing << ", run timer " << player->motion.runTimer << ", height " << player->vertical.y << '\n';
            std::cout << "studs: counter " << studs.counter() << ", " << studs.count(runtime::StudState::Collecting) << " collecting, "
                      << studs.count(runtime::StudState::Done) << " collected, " << studs.count(runtime::StudState::Idle) << " left\n";
            if (hud) std::cout << "hud: " << hud->hearts() << " hearts, stud display " << hud->studDisplay() << ", bar state "
                               << int(hud->studBarState()) << '\n';
            std::cout << "doors:";
            for (const DoorObject& door : doors)
                if (door.state.state != 1) std::cout << ' ' << door.instance << '=' << door.state.state << '/' << int(door.state.timer);
            std::cout << '\n';
            std::cout << "sounds:";
            for (const auto& [update, id] : soundLog) std::cout << ' ' << update << ':' << id;
            std::cout << '\n';
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
    overlay.reset();
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

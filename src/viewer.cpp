// chase-viewer: development model/level viewer (plan step S4b). Not gameplay.
// Loads .bfnmdl models straight from the FUSE archive and draws them with
// their textures. Controls come from config/controls.cfg (3DS button names):
// D-pad moves, L/R lower/raise, X/Y turn, left-drag looks, Escape quits.
#include "assets/assets.hpp"
#include "audio/mixer.hpp"
#include "input.hpp"
#include "pad_sdl.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "render/window.hpp"
#include "runtime/cutscene_camera.hpp"
#include "strict_parse.hpp"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;
using namespace chase::render;
struct AudioOutput {
    chase::audio::Mixer mixer{48000};
    std::mutex lock;
    SDL_AudioDeviceID device = 0;
    static void callback(void* user, Uint8* stream, int bytes) {
        auto* output = static_cast<AudioOutput*>(user);
        std::lock_guard<std::mutex> guard(output->lock);
        output->mixer.mix(reinterpret_cast<float*>(stream), std::size_t(bytes) / (2 * sizeof(float)));
    }
    bool open() {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
        SDL_AudioSpec want{}, have{};
        want.freq = mixer.outputRate();
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = callback;
        want.userdata = this;
        device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (device) SDL_PauseAudioDevice(device, 0);
        return device != 0;
    }
    void close() {
        if (device) SDL_CloseAudioDevice(device);
        device = 0;
    }
    ~AudioOutput() { close(); }
};

// Navigation cells as flat quads, lifted 0.25 above the fitted height (which sits up to
// ~0.1 under the floor): flag bit 0 set (edges, obstacles) is orange, clear is cyan. Heights
// use the data-fitted mapping documented with NavRoom.
ModelPart navOverlay(const NavRoom& room, const NavMesh& nav) {
    ModelPart part;
    part.flags = AttrColor;
    for (const auto& grid : nav.grids) {
        float y = room.origin[1] + grid.height * room.heightUnit / 256 + 0.25f;
        for (std::size_t gz = 0; gz < grid.depth; ++gz)
            for (std::size_t gx = 0; gx < grid.width; ++gx) {
                std::uint8_t cell = grid.cells[gz * grid.width + gx];
                if (part.vertices.size() + 4 > 0xffff) return part;
                auto base = std::uint16_t(part.vertices.size());
                for (int corner = 0; corner < 4; ++corner) {
                    Vertex v;
                    v.position[0] = room.origin[0] + room.cellSize * float(grid.x + gx + (corner & 1) * 0.9f + 0.05f);
                    v.position[1] = y;
                    v.position[2] = room.origin[2] + room.cellSize * float(grid.z + gz + (corner >> 1) * 0.9f + 0.05f);
                    const std::uint8_t orange[4] = {255, 140, 0, 255}, cyan[4] = {0, 200, 255, 255};
                    std::copy_n(cell & 1 ? orange : cyan, 4, v.color);
                    part.vertices.push_back(v);
                }
                for (int k : {0, 2, 1, 1, 2, 3}) part.indices.push_back(std::uint16_t(base + k));
            }
    }
    return part;
}

int resolveKey(const std::string& name) {
    std::string spaced = name;
    std::replace(spaced.begin(), spaced.end(), '_', ' ');
    return SDL_GetKeyFromName(spaced.c_str());
}


struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib";
    std::string names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string config = "config/controls.cfg";
    // cull: faces with this winding are front. CCW culls room ceilings seen from above
    // (observed on lvl_00_policestation room3), so it is the default.
    std::string screenshot, room, cull = "ccw";
    std::string skeleton, animation;
    // Cutscene folder (csdata/models/<id>). Actors start together and hold their last frame;
    // the game's alignment of actor and camera animations of different length is not recovered.
    std::string cutscene;
    bool nav = false;
    bool characters = false;
    double frame = -1;
    std::optional<double> fps;
    float brightness = 1.0f;  // inspection aid only; the game's lighting/combiner setup is not recovered
    std::vector<std::string> exclude, models, levels;
    float yaw = 35, pitch = -25;
    std::vector<float> eye;
};

double positiveReal(const std::string& text) {
    const double value = strict::real(text, -strict::kAnyReal, strict::kAnyReal);
    if (value <= 0) strict::reject("expected above 0", text);
    return value;
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() { return strict::nextValue(argc, argv, i); };
        strict::forOption(arg, [&] {
            if (arg == "--fib") o.fib = value();
            else if (arg == "--names") o.names = value();
            else if (arg == "--config") o.config = value();
            else if (arg == "--screenshot") o.screenshot = value();
            else if (arg == "--exclude") o.exclude.push_back(lower(value()));
            else if (arg == "--level") o.levels.push_back(value());
            else if (arg == "--room") o.room = lower(value());
            else if (arg == "--nav") o.nav = true;
            else if (arg == "--characters") o.characters = true;
            else if (arg == "--cull") {
                o.cull = value();
                if (o.cull != "none" && o.cull != "cw" && o.cull != "ccw") strict::reject("expected none, cw or ccw", o.cull);
            }
            else if (arg == "--brightness") o.brightness = float(strict::real(value(), -strict::kAnyFloat, strict::kAnyFloat));
            else if (arg == "--skeleton") o.skeleton = value();
            else if (arg == "--anim") o.animation = value();
            else if (arg == "--cutscene") o.cutscene = value();
            else if (arg == "--frame") o.frame = strict::real(value(), -strict::kAnyReal, strict::kAnyReal);
            else if (arg == "--fps") o.fps = positiveReal(value());
            else if (arg == "--yaw") o.yaw = float(strict::real(value(), -strict::kAnyFloat, strict::kAnyFloat));
            else if (arg == "--pitch") o.pitch = float(strict::real(value(), -strict::kAnyFloat, strict::kAnyFloat));
            else if (arg == "--eye") {
                const std::string text = value();
                std::string spaced = text;
                std::replace(spaced.begin(), spaced.end(), ',', ' ');
                const std::vector<std::string> fields = strict::words(spaced);
                if (fields.size() != 3) strict::reject("expected X,Y,Z", text);
                for (const std::string& field : fields) o.eye.push_back(float(strict::real(field, -strict::kAnyFloat, strict::kAnyFloat)));
            }
            else if (!arg.empty() && arg[0] == '-') strict::reject("unknown option", arg);
            else o.models.push_back(arg);
        });
    }
    if (o.models.empty() && o.levels.empty() && o.cutscene.empty())
        throw std::runtime_error("usage: chase-viewer [--fib F] [--names CSV] [--config CFG] [--exclude WORD]... "
                                 "[--level levels/.../x.blvl]... [--room NAME] [--nav] [--characters] [--cull none|cw|ccw] [--brightness F] [--screenshot OUT.bmp] [--yaw DEG] [--pitch DEG] [--eye X,Y,Z] "
                                 "[--skeleton S.bfnskl --anim A.bfnanm [--frame N] [--fps F]] [--cutscene csdata/models/ID] "
                                 "[models/<dir>/<model>.bfnmdl...]");
    if (o.skeleton.empty() != o.animation.empty()) throw std::runtime_error("--skeleton and --anim go together");
    if (!o.skeleton.empty() && o.models.empty()) throw std::runtime_error("--anim needs a model argument to pose");
    return o;
}

double playbackRate(const std::optional<double>& override, const Animation& animation) {
    return override ? *override : double(animationFramesPerSecond(animation));
}

int run(const Options& options) {
    std::ifstream config(options.config);
    if (!config) throw std::runtime_error("cannot open controls: " + options.config);
    chase::InputState input(chase::readBindings(config, resolveKey));
    FuseArchive archive(options.fib);
    archive.loadNames(options.names);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    PadDevice pad;
    int width = 960, height = 600;
    SDL_Window* window = SDL_CreateWindow("The Chase Begins - development model viewer (not gameplay)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) throw std::runtime_error(SDL_GetError());
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) throw std::runtime_error(SDL_GetError());
    SDL_GL_SetSwapInterval(1);

    Scene scene(archive, options.exclude);
    std::vector<std::string> models = options.models;
    if (models.empty())
        for (const auto& levelPath : options.levels) {
            auto found = levelModels(archive, levelPath, options.room);
            models.insert(models.end(), found.begin(), found.end());
        }
    for (const auto& model : models)
        if (!scene.addModel(model)) throw std::runtime_error("not in archive: " + model);
    std::size_t placed = 0, unresolved = 0;
    std::vector<LightObject> lights;
    const ClassLookup lightClasses = archiveClassLookup(archive);
    for (const auto& levelPath : options.levels) {
        const FuseEntry* entry = archive.find(levelPath);
        if (!entry) throw std::runtime_error("not in archive: " + levelPath);
        const Level parsedLevel = parseLevel(archive.read(*entry));
        const std::vector<LightObject> levelLights = lightObjects(parsedLevel, lightClasses);
        lights.insert(lights.end(), levelLights.begin(), levelLights.end());
        DrawableObjects drawable = drawableObjects(archive, parsedLevel);
        for (const auto& placement : drawable.objects) {
            if (!options.room.empty() && lower(placement.room) != options.room) continue;
            if (scene.addModel(meshModelPath(placement.mesh), placementMatrix(placement))) ++placed;
            else ++unresolved;
        }
        drawable.characters.erase(std::remove_if(drawable.characters.begin(), drawable.characters.end(),
                                                 [&](const CharacterObject& character) {
                                                     return !options.room.empty() && lower(character.placement.room) != options.room;
                                                 }),
                                  drawable.characters.end());
        if (options.characters) placed += addLevelCharacters(scene, archive, drawable.characters);
    }
    Skeleton skeleton;
    Animation animation;
    bool animated = !options.skeleton.empty();
    if (animated) {
        const FuseEntry* skl = archive.find(options.skeleton);
        const FuseEntry* anm = archive.find(options.animation);
        if (!skl || !anm) throw std::runtime_error("skeleton or animation not in archive");
        skeleton = parseSkeleton(archive.read(*skl));
        animation = parseAnimation(archive.read(*anm));
        std::cout << "posing " << scene.skinnedParts(options.models.front()) << " skinned part(s) with "
                  << skeleton.joints.size() << " joints, " << animation.frames << " frames\n";
    }
    double posedFrame = -1;
    const double animationRate = playbackRate(options.fps, animation);
    AudioOutput audio;
    std::shared_ptr<const Sound> cutsceneSound;
    struct Actor {
        std::string model;
        std::size_t instance = 0;
        std::vector<std::ptrdiff_t> nodes;
        CutsceneCast cast;
    };
    std::vector<Actor> actors;
    Animation cameraAnimation;
    bool cutscene = !options.cutscene.empty();
    if (cutscene) {
        CutsceneContent content = loadCutscene(archive, options.cutscene.substr(options.cutscene.rfind('/') + 1));
        for (auto& cast : content.actors) {
            Actor actor;
            actor.model = cast.model;
            actor.cast = std::move(cast);
            if (actor.cast.skeleton.joints.empty()) {
                actor.nodes = scene.addModelNodes(actor.model, chase::render::identity());
                if (actor.nodes.empty()) throw std::runtime_error("not in archive: " + actor.model);
            } else {
                if (!scene.addModel(actor.model)) throw std::runtime_error("not in archive: " + actor.model);
                actor.instance = scene.instances() - 1;
            }
            actors.push_back(std::move(actor));
        }
        cameraAnimation = std::move(content.camera);
        std::cout << "cutscene: " << actors.size() << " actor(s), " << cameraAnimation.frames << " camera frames\n";
        if (!content.soundPath.empty() && options.screenshot.empty() && options.frame < 0) {
            cutsceneSound = std::make_shared<Sound>(decodeSound(archive.read(*archive.find(content.soundPath))));
            if (!audio.open()) std::cout << "no audio device: " << SDL_GetError() << '\n';
        }
    }
    const double cutsceneRate = playbackRate(options.fps, cameraAnimation);
    double cutscenePeriod = cameraAnimation.frames / cutsceneRate;
    if (cutsceneSound) cutscenePeriod = std::max(cutscenePeriod, double(cutsceneSound->frames) / cutsceneSound->rate);
    Uint32 cutsceneStart = 0;
    int cutsceneVoice = 0;
    auto normalised = [](const Animation& animation, double frame) { return cutsceneNormalised(animation, frame); };
    if (options.nav)
        for (const auto& levelPath : options.levels) {
            Level level = parseLevel(archive.read(*archive.find(levelPath)));
            for (const auto& room : level.navRooms) {
                if (!options.room.empty() && lower(room.room) != options.room) continue;
                std::string navPath = levelPath.substr(0, levelPath.size() - 5) + "_" + lower(room.room) + ".bfnnav";
                const FuseEntry* entry = archive.find(navPath);
                if (!entry) continue;
                scene.addGeometry(navPath, navOverlay(room, parseNav(archive.read(*entry))));
            }
        }
    float centre[3], radius;
    scene.bounds(centre, radius);
    std::cout << "loaded " << scene.instances() << " instance(s) (" << placed << " placed, " << unresolved
              << " unresolved meshnames), " << scene.triangles() << " triangles, " << scene.missingTextures()
              << " missing textures\n";

    float yaw = options.yaw * float(M_PI) / 180, pitch = options.pitch * float(M_PI) / 180;
    float eye[3];
    auto placeCamera = [&]() {
        float d = radius * 2.2f;
        float forward[3], right[3];
        chase::render::cameraAxes(yaw, pitch, forward, right);
        for (int i = 0; i < 3; ++i) eye[i] = centre[i] - d * forward[i];
    };
    placeCamera();
    if (!options.eye.empty()) std::copy_n(options.eye.begin(), 3, eye);
    chase::render::setDepthTest(true);
    if (options.cull != "none") {
        chase::render::setCullFace(true);
        chase::render::setFrontFace(options.cull != "cw");
    }
    bool running = true, dragging = false;
    Uint32 last = SDL_GetTicks();
    int frames = 0;
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
                if (event.button.button == SDL_BUTTON_LEFT) dragging = event.type == SDL_MOUSEBUTTONDOWN;
                break;
            case SDL_MOUSEMOTION:
                if (dragging) {
                    yaw += event.motion.xrel * 0.005f;
                    pitch = std::clamp(pitch - event.motion.yrel * 0.005f, -1.5f, 1.5f);
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) { input.reset(); dragging = false; }
                break;
            default: pad.handle(event, input); break;
            }
        }
        Uint32 now = SDL_GetTicks();
        float dt = std::min(0.1f, (now - last) / 1000.0f);
        last = now;
        float speed = radius * 0.5f * dt;
        float turn = 1.5f * dt;
        yaw += turn * float(input.active(chase::Action::Y) - input.active(chase::Action::X));
        float forward = float(input.vertical()), strafe = float(input.horizontal());
        float rise = float(input.active(chase::Action::R) - input.active(chase::Action::L));
        float axisForward[3], axisRight[3];
        chase::render::cameraAxes(yaw, pitch, axisForward, axisRight);
        for (int i = 0; i < 3; ++i) eye[i] += speed * (forward * axisForward[i] + strafe * axisRight[i]);
        eye[1] += speed * rise;

        if (animated) {
            double frame = options.frame >= 0 ? options.frame : now * animationRate / 1000.0;
            if (frame != posedFrame) {
                scene.pose(options.models.front(), skinMatrices(skeleton, animation, frame));
                posedFrame = frame;
            }
        }
        Mat4 eyeView = view(eye, yaw, pitch);
        float fovY = 1.0f, nearZ = radius * 0.002f, farZ = radius * 20;
        if (cutscene) {
            if (!cutsceneStart || (now - cutsceneStart) / 1000.0 >= cutscenePeriod) {
                cutsceneStart = now;
                if (cutsceneSound && audio.device) {
                    std::lock_guard<std::mutex> guard(audio.lock);
                    audio.mixer.stop(cutsceneVoice);
                    cutsceneVoice = audio.mixer.play(cutsceneSound);
                }
            }
            double elapsed = std::min((now - cutsceneStart) * cutsceneRate / 1000.0, double(cameraAnimation.frames - 1));
            double frame = options.frame >= 0 ? options.frame : elapsed;
            for (const auto& actor : actors) {
                const std::optional<CutsceneClipFrame> started = cutsceneClipAt(actor.cast, frame);
                const std::optional<CutsceneClipFrame> clip = started ? started : cutsceneRestingClip(actor.cast);
                if (!clip) continue;
                const Matrix world = actorMatrix(actorPose(*clip->animation, float(normalised(*clip->animation, clip->frame))));
                if (!actor.nodes.empty()) {
                    const std::vector<Matrix> matrices = cutsceneNodeMatrices(actor.cast, *clip, world);
                    for (std::size_t node = 0; node < actor.nodes.size() && node < matrices.size(); ++node)
                        if (actor.nodes[node] >= 0) scene.setTransform(std::size_t(actor.nodes[node]), matrices[node]);
                    continue;
                }
                scene.setTransform(actor.instance, world);
                scene.pose(actor.model, skinMatrices(actor.cast.skeleton, *clip->animation, clip->frame));
            }
            CutsceneCamera camera = cutsceneCamera(cameraAnimation, float(normalised(cameraAnimation, frame)));
            eyeView = chase::render::cutsceneView(camera);
            fovY = chase::runtime::cutsceneVerticalFieldOfView(camera);
            nearZ = 0.1f;
            farZ = 4000;
        }
        SDL_GL_GetDrawableSize(window, &width, &height);
        chase::render::setViewport({0, 0, width, height});
        chase::render::clearWindow({0.16f, 0.17f, 0.2f, 1}, true);
        beginFrame(options.brightness);
        Mat4 mvp = chase::render::multiply(perspective(fovY, float(width) / float(std::max(height, 1)), nearZ, farZ), eyeView);
        const Mat4 viewToWorld = chase::render::rigidInverse(eyeView);
        scene.setLighting(chase::runtime::levelLights(lights, 0), &viewToWorld[12]);
        scene.draw(mvp);
        if (!options.screenshot.empty() && ++frames >= 2) {
            chase::render::waitForGpu();
            saveScreenshot(options.screenshot, width, height);
            std::cout << "saved " << options.screenshot << '\n';
            running = false;
        }
        SDL_GL_SwapWindow(window);
    }
    audio.close();
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
        std::cerr << "chase-viewer: " << error.what() << '\n';
        return 1;
    }
}

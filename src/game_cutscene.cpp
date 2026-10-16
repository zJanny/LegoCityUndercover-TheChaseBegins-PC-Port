#include "game_cutscene.hpp"
#include "frontend/flow.hpp"
#include "frontend/pages.hpp"
#include "runtime/cutscene_cast.hpp"
#include <algorithm>
#include <iostream>

namespace game_cutscene {
using namespace chase::assets;
namespace render = chase::render;
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

InEngine::InEngine(FuseArchive& archive, render::Scene& scene, std::optional<GameAudio>& audio)
    : archive_(archive), scene_(scene), audio_(audio) {}

InEngine::~InEngine() {
    if (audio_) audio_->stopBuffer(voice_);
}

namespace {
constexpr unsigned kFigureCheckUpdate = 10;
// MOVIE-S31: a standalone actor before its first clip keeps an unposed node; the identity transform is assumed.
const render::Mat4 kUnstartedActorMatrix = render::identity();

void setVisible(render::Scene& scene, const render::CharacterInstance& instance, bool visible) {
    scene.setInstanceVisible(instance.bodyInstance, visible);
    scene.setInstanceVisible(instance.headInstance, visible);
    if (instance.hatInstance) scene.setInstanceVisible(*instance.hatInstance, visible);
}

const StandIn* standInFor(const Roster& roster, const CutsceneCast& cast) {
    const std::size_t fileStart = cast.model.rfind('/'), folderStart = cast.model.rfind('/', fileStart - 1) + 1;
    const std::string file = cast.model.substr(fileStart + 1), folder = cast.model.substr(folderStart, fileStart - folderStart);
    std::vector<std::string> names;
    for (const StandIn& character : roster.characters) names.push_back(character.cutsceneName);
    const runtime::CastTarget target = runtime::resolveCastTarget(folder, file, roster.player.has_value(), names);
    if (target.kind == runtime::CastTargetKind::Player) return &*roster.player;
    if (target.kind == runtime::CastTargetKind::LevelCharacter) return &roster.characters[target.characterIndex];
    return nullptr;
}
}

Roster makeRoster(FuseArchive& archive, const std::vector<CharacterObject>& characters,
                  const std::vector<std::optional<render::CharacterInstance>>& placed, const CharacterAppearance& playerLook,
                  const render::CharacterInstance& playerInstance) {
    Roster roster;
    roster.base = render::readBaseAppearance(archive);
    for (std::size_t i = 0; i < characters.size(); ++i)
        if (placed[i]) roster.characters.push_back({characters[i].cutsceneName, characters[i].appearance, characters[i].hat, *placed[i]});
    roster.player = StandIn{"player1", playerLook, "", playerInstance};
    return roster;
}

std::optional<float> InEngine::load(const std::string& name) {
    if (const auto found = loaded_.find(name); found != loaded_.end()) {
        const Loaded& cutscene = found->second;
        return float(std::max<double>(cutscene.content.camera.frames / cutscene.rate, cutscene.sound ? double(cutscene.sound->frames) / cutscene.sound->rate : 0));
    }
    Loaded cutscene;
    try {
        cutscene.content = loadCutscene(archive_, name);
        for (const CutsceneCast& cast : cutscene.content.actors) {
            Actor actor;
            if (const StandIn* standIn = standInFor(roster_, cast)) {
                Driven driven;
                driven.rig = render::loadCharacterRig(archive_, meshModelPath(standIn->look.body));
                driven.copy = render::addCharacter(scene_, roster_.base, standIn->look, standIn->hat, "cutscene|" + name + "|" + cast.model,
                                                   render::identity());
                driven.original = standIn->original;
                setVisible(scene_, driven.copy, false);
                actor.driven = std::move(driven);
            } else {
                if (!scene_.addModel(cast.model)) throw std::runtime_error("not in archive: " + cast.model);
                actor.standalone = scene_.instances() - 1;
                scene_.setInstanceVisible(*actor.standalone, false);
            }
            cutscene.actors.push_back(std::move(actor));
        }
        if (!cutscene.content.soundPath.empty())
            cutscene.sound = std::make_shared<Sound>(decodeSound(archive_.read(*archive_.find(cutscene.content.soundPath))));
    } catch (const std::exception& error) {
        std::cout << "cutscene " << name << " not loaded: " << error.what() << '\n';
        return std::nullopt;
    }
    cutscene.rate = animationFramesPerSecond(cutscene.content.camera);
    loaded_[name] = std::move(cutscene);
    return load(name);
}

void InEngine::show(const std::string& name, bool visible) {
    const auto found = loaded_.find(name);
    if (found == loaded_.end()) return;
    for (const Actor& actor : found->second.actors) {
        if (actor.standalone) scene_.setInstanceVisible(*actor.standalone, visible);
        if (!actor.driven) continue;
        setVisible(scene_, actor.driven->copy, visible);
        setVisible(scene_, actor.driven->original, !visible);
    }
}

std::size_t InEngine::visibleOriginals(const std::string& name) const {
    std::size_t count = 0;
    for (const Actor& actor : loaded_.at(name).actors)
        if (actor.driven && scene_.instanceVisible(actor.driven->original.bodyInstance)) ++count;
    return count;
}

void InEngine::pose(const Loaded& cutscene, double seconds) {
    const double frame = std::min(seconds * cutscene.rate, double(cutscene.content.camera.frames - 1));
    for (std::size_t i = 0; i < cutscene.content.actors.size(); ++i) {
        const CutsceneCast& actor = cutscene.content.actors[i];
        const std::optional<CutsceneClipFrame> clip = cutsceneClipAt(actor, frame);
        if (const auto& driven = cutscene.actors[i].driven) {
            setVisible(scene_, driven->copy, clip.has_value());
            setVisible(scene_, driven->original, !clip);
            if (!clip) continue;
            const render::Mat4 world = actorMatrix(actorPose(*clip->animation, float(cutsceneNormalised(*clip->animation, clip->frame))));
            const std::vector<Matrix> skin = render::poseBody(scene_, driven->rig, driven->copy, *clip->animation, clip->frame);
            render::placeCharacter(scene_, driven->rig, driven->copy, skin, world);
            continue;
        }
        if (!clip) {
            scene_.setTransform(*cutscene.actors[i].standalone, kUnstartedActorMatrix);
            continue;
        }
        scene_.setTransform(*cutscene.actors[i].standalone, actorMatrix(actorPose(*clip->animation, float(cutsceneNormalised(*clip->animation, clip->frame)))));
        if (!actor.skeleton.joints.empty()) scene_.pose(actor.model, skinMatrices(actor.skeleton, *clip->animation, clip->frame));
    }
}

void InEngine::update(runtime::CutsceneNativeState& state) {
    state.takeRequests();
    const runtime::CutscenePlaying* playing = state.shown() ? state.playing() : nullptr;
    const std::string name = playing ? playing->name : std::string();
    if (name != shown_) {
        show(shown_, false);
        if (audio_) audio_->stopBuffer(voice_);
        voice_ = 0;
        shown_ = name;
        shownUpdates_ = 0;
        if (!name.empty()) {
            show(name, true);
            if (audio_ && loaded_.at(name).sound) voice_ = audio_->playBuffer(loaded_.at(name).sound);
        }
    }
    if (!playing) return;
    pose(loaded_.at(name), playing->elapsed);
    if (++shownUpdates_ == kFigureCheckUpdate)
        std::cout << "cutscene " << name << ": " << visibleOriginals(name) << " level figures drawn beside their cast copies\n";
}

std::optional<CameraView> InEngine::camera(const runtime::CutsceneNativeState& state) const {
    const runtime::CutscenePlaying* playing = state.shown() ? state.playing() : nullptr;
    if (!playing) return std::nullopt;
    const auto found = loaded_.find(playing->name);
    if (found == loaded_.end()) return std::nullopt;
    const Loaded& cutscene = found->second;
    const double frame = std::min(playing->elapsed * cutscene.rate, double(cutscene.content.camera.frames - 1));
    const CutsceneCamera camera = cutsceneCamera(cutscene.content.camera, float(cutsceneNormalised(cutscene.content.camera, frame)));
    return CameraView{render::cutsceneView(camera), camera.fieldOfView * float(M_PI) / 180};
}

Movies::Movies(FuseArchive& archive, fe::Resources& resources, const std::string& directory, int language)
    : archive_(archive), resources_(resources), host_(directory), suffix_(fe::localisationSuffix(language)) {}

Movies::~Movies() = default;

bool Movies::open(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (!host_.open(lower, suffix_)) return false;
    if (!sound_) sound_ = std::make_unique<MovieSound>();
    cues_.clear();
    const std::string base = "cutscenes/" + lower + "_" + suffix_;
    for (const std::string& path : {base + ".srt", base + "_" + suffix_ + ".srt"}) {
        if (!resources_.has(path)) continue;
        cues_ = parseSubtitles(resources_.bytes(path), resources_.extendedCharacters());
        break;
    }
    playing_ = true;
    return true;
}

bool Movies::advance(float dt) {
    if (!playing_) return false;
    if (!host_.advance(dt)) {
        playing_ = false;
        return false;
    }
    sound_->queue(host_.takeAudio());
    return true;
}

void Movies::draw(fe::Canvas& top, fe::Canvas& bottom) {
    top.clear(0, 0, 0);
    bottom.clear(0, 0, 0);
    if (host_.frame().width > 0) {
        const Image& frame = host_.frame();
        top.draw(frame, {0, 0, std::min(frame.width, fe::kTopWidth), std::min(frame.height, fe::kScreenHeight)}, 0, 0);
    }
    std::vector<std::string> lines;
    const auto milliseconds = std::int64_t(host_.frameSeconds() * 1000);
    for (const auto& cue : cues_)
        if (milliseconds >= std::int64_t(cue.startMs) && milliseconds < std::int64_t(cue.endMs)) lines = cue.lines;
    fe::drawSubtitleBubble(bottom, resources_, lines);
}

}

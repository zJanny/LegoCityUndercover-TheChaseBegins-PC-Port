#include "game_cutscene.hpp"
#include "frontend/flow.hpp"
#include "frontend/pages.hpp"
#include "game_doors.hpp"
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

void setVisible(render::Scene& scene, const render::CharacterInstance& instance, bool visible) {
    scene.setInstanceVisible(instance.bodyInstance, visible);
    scene.setInstanceVisible(instance.headInstance, visible);
    if (instance.hatInstance) scene.setInstanceVisible(*instance.hatInstance, visible);
}

runtime::CastTarget castTarget(const Roster& roster, const CutsceneCast& cast) {
    const std::size_t fileStart = cast.model.rfind('/'), folderStart = cast.model.rfind('/', fileStart - 1) + 1;
    const std::string file = cast.model.substr(fileStart + 1), folder = cast.model.substr(folderStart, fileStart - folderStart);
    std::vector<std::pair<std::size_t, runtime::CastEntity>> ordered;
    for (std::size_t i = 0; i < roster.characters.size(); ++i) ordered.push_back({roster.characters[i].order, {roster.characters[i].cutsceneName, false, i}});
    for (std::size_t i = 0; i < roster.props.size(); ++i) ordered.push_back({roster.props[i].order, {roster.props[i].cutsceneName, true, i}});
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<runtime::CastEntity> entities;
    for (const auto& entry : ordered) entities.push_back(entry.second);
    return runtime::resolveCastTarget(folder, file, roster.player.has_value(), entities);
}
}

std::vector<PropStandIn> levelProps(const Level& level, const ClassLookup& classOf, const std::vector<Placement>& placed,
                                    const std::vector<std::size_t>& sceneIndex, const std::vector<runtime::AnimatedProp>& animated) {
    std::vector<PropStandIn> props;
    for (std::size_t i = 0; i < placed.size(); ++i) {
        const std::string name = objectCutsceneCharacter(level.objects[placed[i].objectIndex], *classOf(placed[i].className));
        if (name.empty()) continue;
        PropStandIn prop{name, placed[i].objectIndex, sceneIndex[i], ""};
        for (const runtime::AnimatedProp& candidate : animated)
            if (candidate.instance == game_doors::lowerCase(placed[i].instance)) prop.key = runtime::propSceneKey(candidate.modelPath, candidate.instance);
        props.push_back(std::move(prop));
    }
    return props;
}

Roster makeRoster(FuseArchive& archive, const std::vector<CharacterObject>& characters,
                  const std::vector<std::optional<render::CharacterInstance>>& placed, const CharacterAppearance& playerLook,
                  const render::CharacterInstance& playerInstance, std::vector<PropStandIn> props) {
    Roster roster;
    roster.base = render::readBaseAppearance(archive);
    for (std::size_t i = 0; i < characters.size(); ++i)
        if (placed[i])
            roster.characters.push_back({characters[i].cutsceneName, characters[i].placement.objectIndex, characters[i].appearance, characters[i].hat, *placed[i]});
    roster.player = StandIn{"player1", 0, playerLook, "", playerInstance};
    roster.props = std::move(props);
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
            const runtime::CastTarget target = castTarget(roster_, cast);
            const StandIn* standIn = target.kind == runtime::CastTargetKind::Player          ? &*roster_.player
                                     : target.kind == runtime::CastTargetKind::LevelCharacter ? &roster_.characters[target.index]
                                                                                              : nullptr;
            const bool poseLevelProp = target.kind == runtime::CastTargetKind::LevelProp && !cast.skeleton.joints.empty() && !roster_.props[target.index].key.empty();
            if (poseLevelProp) {
                actor.levelProp = target.index;
            } else if (standIn) {
                Driven driven;
                driven.rig = render::loadCharacterRig(archive_, meshModelPath(standIn->look.body));
                driven.copy = render::addCharacter(scene_, roster_.base, standIn->look, standIn->hat, "cutscene|" + name + "|" + cast.model,
                                                   render::identity());
                driven.original = standIn->original;
                setVisible(scene_, driven.copy, false);
                actor.driven = std::move(driven);
            } else if (cast.skeleton.joints.empty()) {
                actor.nodes = scene_.addModelNodes(cast.model, render::identity());
                if (actor.nodes.empty()) throw std::runtime_error("not in archive: " + cast.model);
                for (const std::ptrdiff_t node : actor.nodes)
                    if (node >= 0) scene_.setInstanceVisible(std::size_t(node), false);
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
        for (const std::ptrdiff_t node : actor.nodes)
            if (node >= 0) scene_.setInstanceVisible(std::size_t(node), visible);
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

// STAND-IN MOVIE-S33: a model of its own is drawn from the update its first clip starts; the rule that hides it before is not read.
void InEngine::showOwnModel(const Actor& actor, bool started) {
    if (actor.standalone) scene_.setInstanceVisible(*actor.standalone, started);
    for (const std::ptrdiff_t node : actor.nodes)
        if (node >= 0) scene_.setInstanceVisible(std::size_t(node), started);
}

std::size_t InEngine::visibleOwnModels(const std::string& name) const {
    std::size_t count = 0;
    for (const Actor& actor : loaded_.at(name).actors) {
        bool shown = actor.standalone && scene_.instanceVisible(*actor.standalone);
        for (const std::ptrdiff_t node : actor.nodes)
            if (node >= 0 && scene_.instanceVisible(std::size_t(node))) shown = true;
        if (shown) ++count;
    }
    return count;
}

std::size_t InEngine::drivenProps(const std::string& name) const {
    return std::size_t(std::count_if(loaded_.at(name).actors.begin(), loaded_.at(name).actors.end(), [](const Actor& actor) { return actor.levelProp.has_value(); }));
}

void InEngine::pose(const Loaded& cutscene, double seconds) {
    const double frame = std::min(seconds * cutscene.rate, double(cutscene.content.camera.frames - 1));
    for (std::size_t i = 0; i < cutscene.content.actors.size(); ++i) {
        const CutsceneCast& actor = cutscene.content.actors[i];
        const std::optional<CutsceneClipFrame> clip = cutsceneClipAt(actor, frame);
        if (const auto& prop = cutscene.actors[i].levelProp) {
            if (!clip) continue;
            const PropStandIn& level = roster_.props[*prop];
            scene_.setTransform(level.instance, actorMatrix(actorPose(*clip->animation, float(cutsceneNormalised(*clip->animation, clip->frame)))));
            scene_.pose(level.key, skinMatrices(actor.skeleton, *clip->animation, clip->frame));
            continue;
        }
        if (const auto& driven = cutscene.actors[i].driven) {
            setVisible(scene_, driven->copy, clip.has_value());
            setVisible(scene_, driven->original, !clip);
            if (!clip) continue;
            const render::Mat4 world = actorMatrix(actorPose(*clip->animation, float(cutsceneNormalised(*clip->animation, clip->frame))));
            const std::vector<Matrix> skin = render::poseBody(scene_, driven->rig, driven->copy, *clip->animation, clip->frame);
            render::placeCharacter(scene_, driven->rig, driven->copy, skin, world);
            continue;
        }
        const std::vector<std::ptrdiff_t>& nodes = cutscene.actors[i].nodes;
        showOwnModel(cutscene.actors[i], clip.has_value());
        if (!clip) continue;
        const std::optional<CutsceneClipFrame>& shown = clip;
        const render::Mat4 world = actorMatrix(actorPose(*shown->animation, float(cutsceneNormalised(*shown->animation, shown->frame))));
        if (!nodes.empty()) {
            const std::vector<Matrix> matrices = cutsceneNodeMatrices(actor, *shown, world);
            for (std::size_t node = 0; node < nodes.size() && node < matrices.size(); ++node)
                if (nodes[node] >= 0) scene_.setTransform(std::size_t(nodes[node]), matrices[node]);
            continue;
        }
        scene_.setTransform(*cutscene.actors[i].standalone, world);
        scene_.pose(actor.model, skinMatrices(actor.skeleton, *shown->animation, shown->frame));
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
        std::cout << "cutscene " << name << ": " << visibleOriginals(name) << " level figures drawn beside their cast copies, " << visibleOwnModels(name)
                  << " models of their own drawn, " << drivenProps(name) << " level props posed\n";
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

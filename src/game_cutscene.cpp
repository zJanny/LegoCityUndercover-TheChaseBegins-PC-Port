#include "game_cutscene.hpp"
#include "frontend/flow.hpp"
#include "frontend/pages.hpp"
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

std::optional<float> InEngine::load(const std::string& name) {
    if (const auto found = loaded_.find(name); found != loaded_.end()) {
        const Loaded& cutscene = found->second;
        return float(std::max<double>(cutscene.content.camera.frames / cutscene.rate, cutscene.sound ? double(cutscene.sound->frames) / cutscene.sound->rate : 0));
    }
    Loaded cutscene;
    try {
        cutscene.content = loadCutscene(archive_, name);
        for (const CutsceneCast& cast : cutscene.content.actors) {
            if (!scene_.addModel(cast.model)) throw std::runtime_error("not in archive: " + cast.model);
            cutscene.instances.push_back(scene_.instances() - 1);
            scene_.setInstanceVisible(cutscene.instances.back(), false);
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
    for (const std::size_t instance : found->second.instances) scene_.setInstanceVisible(instance, visible);
}

void InEngine::pose(const Loaded& cutscene, double seconds) {
    const double frame = std::min(seconds * cutscene.rate, double(cutscene.content.camera.frames - 1));
    for (std::size_t i = 0; i < cutscene.content.actors.size(); ++i) {
        const CutsceneCast& actor = cutscene.content.actors[i];
        const double local = std::min(frame, double(actor.animation.frames ? actor.animation.frames - 1 : 0));
        scene_.setTransform(cutscene.instances[i], actorMatrix(actorPose(actor.animation, float(cutsceneNormalised(actor.animation, local)))));
        if (!actor.skeleton.joints.empty()) scene_.pose(actor.model, skinMatrices(actor.skeleton, actor.animation, local));
    }
}

void InEngine::update(runtime::CutsceneNativeState& state) {
    state.takeRequests();
    const runtime::CutscenePlaying* playing = state.playing();
    const std::string name = playing ? playing->name : std::string();
    if (name != shown_) {
        show(shown_, false);
        if (audio_) audio_->stopBuffer(voice_);
        voice_ = 0;
        shown_ = name;
        if (!name.empty()) {
            show(name, true);
            if (audio_ && loaded_.at(name).sound) voice_ = audio_->playBuffer(loaded_.at(name).sound);
        }
    }
    if (playing) pose(loaded_.at(name), playing->elapsed);
}

std::optional<CameraView> InEngine::camera(const runtime::CutsceneNativeState& state) const {
    const runtime::CutscenePlaying* playing = state.playing();
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

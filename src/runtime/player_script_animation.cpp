#include "runtime/player_script_animation.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::runtime {
namespace {

constexpr const char* kCharacterModelFolder = "models/char_minifig/";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

AnimationLoader archiveAnimationLoader(assets::FuseArchive& archive) {
    return [&archive](const std::string& path) -> std::optional<assets::Animation> {
        const assets::FuseEntry* entry = archive.find(path);
        if (!entry) return std::nullopt;
        return assets::parseAnimation(archive.read(*entry));
    };
}

std::string PlayerScriptAnimation::clipPath(const std::string& classFolder, const std::string& name) {
    return lower(kCharacterModelFolder + (classFolder.empty() ? std::string() : classFolder + "/") + name + ".bfnanm");
}

const PlayerScriptAnimation::Clip* PlayerScriptAnimation::clipOf(const std::string& path) {
    const auto found = clips_.find(path);
    if (found != clips_.end()) return &found->second;
    if (!loader_) return nullptr;
    std::optional<assets::Animation> animation = loader_(path);
    if (!animation) return nullptr;
    Clip clip;
    clip.animation = std::move(*animation);
    clip.info.clip = std::uint32_t(clips_.size());
    clip.info.length = assets::animationLength(clip.animation);
    clip.info.rate = std::uint16_t(assets::animationFramesPerSecond(clip.animation));
    clip.info.loops = assets::animationLoops(clip.animation);
    const Clip& placed = clips_.emplace(path, std::move(clip)).first->second;
    byId_[placed.info.clip] = &placed;
    return &placed;
}

void PlayerScriptAnimation::play(const AnimationPlay& play) {
    if (play.instance != kPlayerInstance || !play.request.character) return;
    const Clip* clip = clipOf(clipPath(play.request.classFolder, play.request.name));
    if (!clip) return;
    const AnimationSlot* current = slots_.current();
    const bool loaded = scriptLoaded_ && current && current->clip == clip->info.clip;
    playCharacterAnimation(slots_, scriptLoaded_, clip->info, loaded, play.request.loop, clock_);
    requested_ = kUserAnimState;
}

void PlayerScriptAnimation::stop(const std::string& instance) {
    if (instance == kPlayerInstance) slots_.stop(clock_);
}

std::optional<bool> PlayerScriptAnimation::currentNearEnd(const std::string& instance) const {
    if (instance != kPlayerInstance || !slots_.current()) return std::nullopt;
    if (state_ != kUserAnimState && requested_ != kUserAnimState) return std::nullopt;
    return slots_.currentNearEnd(clock_);
}

bool PlayerScriptAnimation::update(float dt) {
    clock_ += std::llround(double(dt) * double(kAnimationClockTicksPerSecond));
    if (requested_) {
        state_ = *requested_;
        requested_.reset();
    }
    if (state_ != kUserAnimState) return false;
    if (slots_.kind(clock_) == kAnimationStopped) requested_ = kLocomotionState;
    return true;
}

const assets::Animation* PlayerScriptAnimation::animation() const {
    const AnimationSlot* slot = slots_.current();
    if (!slot) return nullptr;
    const auto found = byId_.find(slot->clip);
    return found == byId_.end() ? nullptr : &found->second->animation;
}

double PlayerScriptAnimation::frame() const {
    const AnimationSlot* slot = slots_.current();
    const assets::Animation* clip = animation();
    if (!slot || !clip) return 0.0;
    const double position = AnimationSlots::position(*slot, clock_).frame;
    return std::clamp(position, 0.0, double(clip->frames > 0 ? clip->frames - 1 : 0));
}

}

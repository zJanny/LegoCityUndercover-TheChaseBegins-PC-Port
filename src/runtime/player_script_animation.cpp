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
    lastRoot_.reset();
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
    entered_.reset();
    const bool updated = state_ == kUserAnimState;
    if (updated && slots_.kind(clock_) == kAnimationStopped) requested_ = kLocomotionState;
    if (requested_) {
        if (*requested_ != state_) {
            state_ = *requested_;
            entered_ = state_;
        }
        requested_.reset();
    }
    return updated;
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

namespace chase::runtime {
namespace {
constexpr float kHeadingUnitsPerRadian = 10430.378f;
}

std::optional<ScriptRootStep> PlayerScriptAnimation::rootMotion(std::int16_t facing) {
    const AnimationSlot* slot = slots_.current();
    const assets::Animation* clip = animation();
    if (!slot || !clip) return std::nullopt;
    const std::optional<RootSample> root = rootSample(*clip, frame());
    if (!root) return std::nullopt;
    const bool continued = lastRoot_ && lastRootStart_ == slot->startId;
    const RootSample previous = continued ? *lastRoot_ : *root;
    lastRoot_ = root;
    lastRootStart_ = slot->startId;
    if (!continued) return std::nullopt;
    if (!lastFacing_ || *lastFacing_ != facing) nodeHeading_ = float(facing) / kHeadingUnitsPerRadian;
    assets::Placement placement;
    placement.orientation[1] = nodeHeading_ * 57.29578f;
    const assets::Matrix node = assets::placementMatrix(placement);
    const RootSample delta = rootDelta(previous, *root);
    const assets::Matrix moved = applyRootDelta(node, delta);
    ScriptRootStep step;
    step.dx = moved[12];
    step.dy = moved[13];
    step.dz = moved[14];
    lastFacing_ = facing;
    if (delta.yaw != 0.0f) {
        nodeHeading_ = std::atan2(moved[8], moved[10]);
        const std::int16_t turned = std::int16_t(std::uint16_t(int(nodeHeading_ * kHeadingUnitsPerRadian) & 0xffff));
        step.facing = turned;
        lastFacing_ = turned;
    }
    return step;
}

void applyScriptStateEnter(unsigned state, VerticalState& vertical) {
    vertical.fallSpeed = 0;
    if (state == kLocomotionState) vertical.fallDistance = 0;
}

void applyScriptRootStep(const ScriptRootStep& step, LocomotionState& motion, VerticalState& vertical, const CollisionWorld& world, float dt) {
    if (step.facing) motion.facing = *step.facing;
    motion.lastIntent = motion.facing;
    playerRootStepUpdate(motion, vertical, world, step.dx, step.dz, dt, step.dy);
}

}

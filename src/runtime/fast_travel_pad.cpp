#include "runtime/fast_travel_pad.hpp"

#include "assets/object_classes.hpp"
#include "runtime/prop_animation.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::int16_t turnTowards(std::int16_t facing, std::int16_t target, int step) {
    if (facing == target) return facing;
    int d = int(target) - int(facing);
    if (d < -32768) d += 65536;
    if (d > 32768) d -= 65536;
    d = std::clamp(d, -step, step);
    return std::int16_t(std::uint16_t(int(facing) + d));
}

bool isPad(const assets::LevelObject& object, const assets::ClassLookup& classOf) {
    const assets::Level* cls = classOf(object.className);
    return cls && assets::objectKind(object, *cls) == kFastTravelPadKind;
}

void setZone(FastTravelPad& pad, const assets::Level& cls) {
    const auto bound = std::find_if(cls.bounds.begin(), cls.bounds.end(), [](const assets::LevelBound& b) { return lower(b.name) == "usebound"; });
    if (bound == cls.bounds.end()) return;
    std::copy_n(bound->centre, 3, pad.zoneCentre);
    std::copy_n(bound->half, 3, pad.zoneHalf);
    pad.hasZone = true;
}

std::size_t clipIndex(FastTravelClip clip) { return std::size_t(clip) - 1; }

constexpr std::array<const char*, 3> kHelicopterClipNames = {"land", "idle", "takeoff"};  // 0x1bad3c, 0x1bad44, 0x1bad4c

}

std::optional<FastTravelHelicopterModel> loadFastTravelHelicopter(assets::FuseArchive& archive, const std::string& mesh) {
    FastTravelHelicopterModel model;
    model.modelPath = assets::meshModelPath(mesh);
    const assets::FuseEntry* skeleton = archive.find(model.modelPath.substr(0, model.modelPath.rfind('.')) + ".bfnskl");
    if (!skeleton) return std::nullopt;
    model.skeleton = assets::parseSkeleton(archive.read(*skeleton));
    for (std::size_t i = 0; i < kHelicopterClipNames.size(); ++i) {
        const assets::FuseEntry* entry = archive.find(PropAnimations::animationPath(mesh, kHelicopterClipNames[i]));
        if (!entry) return std::nullopt;
        model.animations[i] = assets::parseAnimation(archive.read(*entry));
        model.clips[i].clip = std::uint32_t(i);
        model.clips[i].length = assets::animationLength(model.animations[i]);
        model.clips[i].rate = std::uint16_t(assets::animationFramesPerSecond(model.animations[i]));
        model.clips[i].loops = assets::animationLoops(model.animations[i]);
    }
    return model;
}

std::vector<assets::Matrix> fastTravelHelicopterPose(const FastTravelHelicopterModel& model, const FastTravelHelicopter& helicopter, std::int64_t clock) {
    const AnimationSlot* slot = helicopter.slots.current();
    if (!slot || slot->clip >= model.animations.size()) return {};
    const assets::Animation& animation = model.animations[slot->clip];
    const double last = double(animation.frames > 0 ? animation.frames - 1 : 0);
    const double frame = std::clamp(double(AnimationSlots::position(*slot, clock).frame), 0.0, last);
    return assets::skinMatrices(model.skeleton, animation, frame);
}

std::int16_t FastTravelPads::heading(const assets::Matrix& node) {
    return std::int16_t(std::int32_t(std::atan2(node[8], node[10]) * kFastTravelHeadingScale));
}

void FastTravelPads::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    pads_.clear();
    helicopters_.clear();
    state_ = requested_ = 0;
    pad_.reset();
    for (const assets::LevelObject& object : level.objects) {
        const std::optional<assets::Placement> start = assets::objectStart(level, object.instance);
        const assets::Matrix holderMatrix = assets::placementMatrix(start.value_or(assets::Placement{}));
        if (isPad(object, classOf)) {
            FastTravelPad pad;
            pad.instance = lower(object.instance);
            pad.world = holderMatrix;
            setZone(pad, *classOf(object.className));
            add(std::move(pad));
            continue;
        }
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        std::string helicopterMesh;
        assets::Matrix helicopterWorld{};
        for (const assets::Placement& child : assets::objectPlacements(*cls, classOf))
            if (lower(child.instance) == lower(kFastTravelHelicopterChild)) {
                helicopterMesh = child.mesh;
                helicopterWorld = assets::multiply(assets::placementMatrix(child), holderMatrix);
            }
        for (const assets::LevelObject& child : cls->objects) {
            if (!isPad(child, classOf)) continue;
            FastTravelPad pad;
            pad.holder = lower(object.instance);
            pad.instance = pad.holder + "." + lower(child.instance);
            const std::optional<assets::Placement> local = assets::objectStart(*cls, child.instance);
            pad.world = assets::multiply(assets::placementMatrix(local.value_or(assets::Placement{})), holderMatrix);
            if (!helicopterMesh.empty()) {
                pad.helicopter = pad.holder + "." + lower(kFastTravelHelicopterChild);
                pad.helicopterMesh = helicopterMesh;
                pad.helicopterWorld = helicopterWorld;
            }
            setZone(pad, *classOf(child.className));
            add(std::move(pad));
        }
    }
}

void FastTravelPads::add(FastTravelPad pad, FastTravelHelicopter helicopter) {
    pads_.push_back(std::move(pad));
    helicopters_.push_back(std::move(helicopter));
}

void FastTravelPads::setHelicopterClips(std::size_t index, const std::array<AnimationClipInfo, 3>& clips) { helicopters_[index].clips = clips; }

bool FastTravelPads::inZone(const FastTravelPad& pad, const float player[3]) {
    if (!pad.hasZone) return false;
    float offset[3];
    for (int a = 0; a < 3; ++a) offset[a] = player[a] - pad.world[std::size_t(12 + a)];
    for (int a = 0; a < 3; ++a) {
        const float* axis = pad.world.data() + 4 * a;
        const float local = offset[0] * axis[0] + offset[1] * axis[1] + offset[2] * axis[2];
        if (!(std::fabs(local - pad.zoneCentre[a]) < pad.zoneHalf[a])) return false;
    }
    return true;
}

void FastTravelPads::play(std::size_t index, FastTravelClip clip, unsigned mode) {
    FastTravelHelicopter& helicopter = helicopters_[index];
    helicopter.slots.start(helicopter.clips[clipIndex(clip)], mode, 0, kAnimationFrameDefault, 1.0f, 0.0f, clock_);
}

FastTravelClip FastTravelPads::currentClip(std::size_t index) const {
    const FastTravelHelicopter& helicopter = helicopters_[index];
    const AnimationSlot* slot = helicopter.slots.current();
    if (!slot) return FastTravelClip::None;
    for (FastTravelClip clip : {FastTravelClip::Land, FastTravelClip::Idle, FastTravelClip::Takeoff})
        if (slot->clip == helicopter.clips[clipIndex(clip)].clip) return clip;
    return FastTravelClip::None;
}

bool FastTravelPads::current(std::size_t index, FastTravelClip clip, bool finished) const {
    if (currentClip(index) != clip) return false;
    return (helicopters_[index].slots.kind(clock_) == kAnimationStopped) == finished;
}

bool FastTravelPads::mapReady() const {
    if (state_ == kPlayerStateFastTravelEnter) return false;
    if (state_ == kPlayerStateFastTravelActive) return idleStarted_;
    return true;
}

void FastTravelPads::enter(std::uint16_t next, FastTravelPlayer& player, std::vector<FastTravelEvent>& events) {
    state_ = next;
    const std::size_t index = *pad_;
    if (next == kPlayerStateFastTravelEnter) {
        approaching_ = true;
        approach_ = 0;
        player.resetRunTimer = true;
        target_ = heading(pads_[index].world);
        player.lastIntent = target_;
        if (!pads_[index].helicopter.empty()) {
            play(index, FastTravelClip::Land, kAnimationOnce);
            helicopters_[index].shown = true;
        }
        events.push_back({FastTravelEventKind::Entered, index});
    } else {
        idleStarted_ = false;
        events.push_back({FastTravelEventKind::Active, index});
    }
}

std::vector<FastTravelEvent> FastTravelPads::update(const assets::PlacedObjectState& state, const FastTravelInput& input, FastTravelPlayer& player) {
    std::vector<FastTravelEvent> events;
    player = FastTravelPlayer{};
    clock_ += std::llround(double(input.dt) * double(kAnimationClockTicksPerSecond));

    for (std::size_t i = 0; i < pads_.size(); ++i) {
        if (pads_[i].helicopter.empty() || state.isDisabled(pads_[i].instance)) continue;
        pads_[i].busy = current(i, FastTravelClip::Takeoff, false);
        if (current(i, FastTravelClip::Takeoff, true) && helicopters_[i].shown) {
            helicopters_[i].shown = false;
            events.push_back({FastTravelEventKind::HelicopterHidden, i});
        }
    }

    if (requested_ != 0 && requested_ != state_) enter(requested_, player, events);
    requested_ = 0;

    if (state_ == kPlayerStateFastTravelEnter) {
        const std::size_t index = *pad_;
        if (approaching_) {
            const std::int16_t facing = turnTowards(input.facing, target_, kFastTravelTurnStep);
            player.turned = true;
            player.facing = facing;
            if (approach_ < 1.0f) {
                const float* to = pads_[index].world.data() + 12;
                for (int a = 0; a < 3; ++a) player.position[a] = input.player[a] + (to[a] - input.player[a]) * approach_;
                player.moved = true;
                target_ = heading(pads_[index].world);
                player.lastIntent = target_;
                approach_ += input.dt * kFastTravelApproachRate;
            } else {
                approaching_ = false;
            }
        }
        if (!approaching_) requested_ = kPlayerStateFastTravelActive;
    } else if (state_ == kPlayerStateFastTravelActive) {
        const std::size_t index = *pad_;
        if (!pads_[index].helicopter.empty() && current(index, FastTravelClip::Land, true)) {
            play(index, FastTravelClip::Idle, kAnimationLoop);
            idleStarted_ = true;
            events.push_back({FastTravelEventKind::HelicopterIdle, index});
        }
    }

    if (state_ == 0 && input.playerFree && input.actionPressed)
        for (std::size_t i = 0; i < pads_.size(); ++i) {
            const FastTravelPad& pad = pads_[i];
            if (state.isDisabled(pad.instance) || (!pad.holder.empty() && state.isDisabled(pad.holder)) || !inZone(pad, input.player)) continue;
            if (!pad.busy) {
                pad_ = i;
                requested_ = kPlayerStateFastTravelEnter;
            }
            break;
        }
    return events;
}

}

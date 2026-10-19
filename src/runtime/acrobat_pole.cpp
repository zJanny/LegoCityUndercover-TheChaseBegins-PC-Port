#include "runtime/acrobat_pole.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr float kHeadingScale = 10430.378f;  // 0x1e47f4 / 0x275138
constexpr std::uint32_t kOneBits = 0x3f800000;
constexpr int kArrivalState = 88;

using Vec = std::array<float, 3>;

Vec vec(const float v[3]) { return {v[0], v[1], v[2]}; }

float radiansFromFacing(std::int16_t facing) { return float(std::uint16_t(facing)) / kHeadingScale; }

std::int16_t facingFromRadians(float heading) {
    return std::int16_t(std::uint16_t(int(std::lround(double(heading) * kHeadingScale))));
}

Vec facingRow(std::int16_t facing) {
    const float heading = radiansFromFacing(facing);
    return {std::sin(heading), 0.0f, std::cos(heading)};
}

std::uint32_t floatBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    return bits;
}

const assets::AnimTrack* findTrack(const assets::Animation& animation, std::uint32_t hash) {
    for (const auto& sub : animation.subAnimations)
        for (const assets::AnimTrack& track : sub)
            if (track.hash == hash) return &track;
    return nullptr;
}

Vec rootAt(const std::vector<std::array<float, 3>>& root, float frame) {
    if (root.empty()) return {0, 0, 0};
    const float last = float(root.size() - 1);
    frame = std::max(0.0f, std::min(frame, last));
    const std::size_t index = std::size_t(frame);
    if (index + 1 >= root.size()) return root.back();
    const float f = frame - float(index);
    Vec out;
    for (std::size_t a = 0; a < 3; ++a) out[a] = root[index][a] + (root[index + 1][a] - root[index][a]) * f;
    return out;
}

// The root motion delta of the slot (0x2327b4): xz times anim+0x20, y unscaled, turned into world by the player's facing.
Vec rootStep(const AcrobatPoleClips& clips, float before, float after, float scale, std::int16_t facing) {
    const Vec a = rootAt(clips.jumpRoot, before), b = rootAt(clips.jumpRoot, after);
    const float x = (b[0] - a[0]) * scale, y = b[1] - a[1], z = (b[2] - a[2]) * scale;
    const float heading = radiansFromFacing(facing);
    const float s = std::sin(heading), c = std::cos(heading);
    return {c * x + s * z, y, -s * x + c * z};
}

const AcrobatPoleClip& clipOf(const AcrobatPoleClips& clips, int state) {
    if (state == kAcrobatPoleIdleState) return clips.idle;
    if (state == kAcrobatPoleFlipState) return clips.flip;
    return clips.jump;
}

int animationOf(int state) {
    switch (state) {
        case kAcrobatPoleFlipState: return kAnimationAcrobatPoleFlip;
        case kAcrobatPoleJumpState: case kAcrobatPoleJumpAgainState: return kAnimationAcrobatPoleJump;
        case kAcrobatPoleJumpToFallState: case kAcrobatPoleJumpToLandState: return kAnimationAcrobatPoleJumpOff;
        default: return kAnimationAcrobatPoleIdle;
    }
}

bool isJumpState(int state) { return state >= kAcrobatPoleJumpState && state <= kAcrobatPoleJumpToLandState; }

struct Update {
    AcrobatPoleRider& rider;
    const ClimbBarSet& set;
    const AcrobatPoleClips& clips;
    const AcrobatPoleInput& input;
    Vec position;
    std::int16_t facing;
    std::vector<ClimbBarSound> sounds;

    const ClimbBar* poleAt(int index) const { return index >= 0 && std::size_t(index) < set.bars.size() ? &set.bars[std::size_t(index)] : nullptr; }

    void sound(std::uint16_t id) {
        if (id) sounds.push_back({id});
    }

    void restartClip() {
        rider.clipTime = 0.0f;
        rider.rootTime = 0.0f;
        rider.clipFinished = false;
    }

    // 0x1f80d4 with the hang points of 0x1fb8d8: states 88, 93, 95 and 96 sample the hand joint while ctrl+0x28c is 0 (TRAV-S24: the
    // sample of the jump clip is the measured frame-0 offset, the idle clip keeps TRAV-S21), the others use ctrl+0x2e8 as stored.
    void place(int stateForOffset) {
        const ClimbBar* pole = poleAt(rider.pole);
        if (!pole) return;
        const bool samples = stateForOffset == kArrivalState || stateForOffset == kAcrobatPoleIdleState || stateForOffset == kAcrobatPoleJumpState ||
                             stateForOffset == kAcrobatPoleJumpAgainState;
        if (samples && !rider.offsetSampled) {
            const bool jumpClip = isJumpState(rider.state);
            rider.hangOffsetY = jumpClip ? kAcrobatPoleJumpHangOffsetY : kAcrobatPoleHangOffsetY;
            rider.hangOffsetZ = jumpClip ? kAcrobatPoleJumpHangOffsetZ : kAcrobatPoleHangOffsetZ;
            rider.offsetSampled = true;
        }
        position = acrobatPoleCentre(*pole, rider.hangOffsetY, rider.hangOffsetZ);
        facing = acrobatPoleFacing(*pole, facing);
    }

    bool flipTest() const { return acrobatPoleFlipTest(facing, input.stickHeld, input.stickHeading, input.cameraYaw); }

    // 0x211b88(player, pole).
    void jump(int pole) {
        rider.from = pole;
        const ClimbBar* bar = poleAt(pole);
        if (!bar) {
            rider.pending = kAcrobatPoleFallState;
            return;
        }
        const AcrobatPoleJump choice = acrobatPoleJump(*bar, facing, rider.state, input.stickHeld, input.stickHeading, input.linkEnabled);
        rider.target = {};
        if (choice.link >= 0) {
            const ClimbBarLink& link = bar->links[std::size_t(choice.link)];
            rider.target.present = true;
            rider.target.bar = link.bar;
            rider.target.objectType = link.objectType;
            const Vec point = link.bar >= 0 ? climbBarGrabCentre(set.bars[std::size_t(link.bar)]) : vec(link.position);
            std::copy(point.begin(), point.end(), rider.target.point);
        }
        rider.pending = choice.state;
    }

    // 0x31c20c.
    void enterIdle() {
        rider.state = kAcrobatPoleIdleState;
        rider.jumpWanted = false;
        restartClip();
    }

    // 0x31f31c.
    void enterFlip(int stateForOffset) {
        rider.pole = rider.from;
        rider.state = kAcrobatPoleFlipState;
        restartClip();
        place(stateForOffset);
    }

    // 0x31f3bc.
    void enterJump(int state, int stateForOffset) {
        rider.pole = rider.from;
        rider.state = state;
        restartClip();
        rider.phase = 0.0f;
        rider.offsetSampled = false;
        rider.rootScale = 1.0f;
        const ClimbBar* pole = poleAt(rider.pole);
        if (!pole) return;
        place(stateForOffset);
        if (!rider.target.present) return;
        const Vec from = climbBarGrabCentre(*pole);
        const float height = rider.target.point[1] - from[1];
        const float dx = rider.target.point[0] - from[0], dz = rider.target.point[2] - from[2];
        const float distance = std::sqrt(dx * dx + dz * dz);
        if (clips.jumpTravel > 0.0f) {
            rider.rootScale = distance / clips.jumpTravel;
            rider.verticalSpeed = height / (clips.jumpPhaseFrames / clips.jump.rate);
        }
        if (distance > 0.0f) facing = facingFromRadians(std::atan2(dx, dz));
    }

    void enter(int state) {
        const int previous = rider.state;
        if (state == kAcrobatPoleIdleState) enterIdle();
        else if (state == kAcrobatPoleFlipState) enterFlip(previous);
        else enterJump(state, previous);
    }

    // Flip Done 0x334680.
    void flipDone() {
        if (const ClimbBar* pole = poleAt(rider.pole)) sound(pole->swingSound);
        facing = std::int16_t(std::uint16_t(std::uint16_t(facing) + 0x8000u));
        if (input.bHeld) jump(rider.pole);
        else if (flipTest()) enterFlip(rider.state);
        else rider.pending = kAcrobatPoleIdleState;
    }

    // Jump Done 0x334794. TRAV-S25: the occupant of the target pole (comp+0x1c) is taken as free; a target bar without AcrobatPole falls instead of
    // the grab 0x2158a4.
    void jumpDone() {
        const ClimbBar* target = poleAt(rider.target.bar);
        if (!target) {
            rider.pending = kAcrobatPoleFallState;
            return;
        }
        rider.from = rider.target.bar;
        sound(target->swingSound);
        sound(target->grabSound);
        if (input.bHeld) jump(rider.target.bar);
        else if (flipTest()) rider.pending = kAcrobatPoleFlipState;
        else if (target->acrobatPole) rider.pending = kAcrobatPoleIdleState;
        else rider.pending = kAcrobatPoleFallState;
    }

    // Event type 1 to the listeners of the current state: the clip-finished event of the state list pump 0x2eaee0, and 0x31f6e4 at
    // phase 2 without a pole target.
    void eventOne() {
        switch (rider.state) {
            case kAcrobatPoleFlipState: flipDone(); break;
            case kAcrobatPoleJumpState: case kAcrobatPoleJumpAgainState: jumpDone(); break;
            case kAcrobatPoleJumpToFallState: case kAcrobatPoleJumpToLandState: rider.pending = kAcrobatPoleFallState; break;
            default: break;
        }
    }

    // 0x31c2dc. TRAV-S25: the swing sound plays without the test 0x237acc.
    void idleUpdate() {
        place(kAcrobatPoleIdleState);
        const ClimbBar* pole = poleAt(rider.pole);
        if (!pole) return;
        if (input.bHeld) rider.jumpWanted = true;
        if (pole->immediateJump && input.bHeld) {
            jump(rider.pole);
            return;
        }
        if (clips.idle.length - 1.0f > rider.clipTime) return;
        sound(pole->swingSound);
        if (input.bHeld || rider.jumpWanted) jump(rider.pole);
        else if (flipTest()) rider.pending = kAcrobatPoleFlipState;
    }

    // 0x215804 (clip time up to 5) / 0x20cd30 (TRAV-S26 for both): the step without collision, then the ground below the end point stops the fall.
    void groundMove(const Vec& step) {
        const float before = position[1];
        position[0] += step[0];
        position[2] += step[2];
        const float drop = step[1];
        if (input.ground) {
            const float probe[3] = {position[0], before, position[2]};
            if (const std::optional<float> floor = input.ground(probe, std::max(-drop, 0.0f))) {
                if (*floor >= before + drop) {
                    position[1] = *floor;
                    return;
                }
            }
        }
        position[1] = before + drop;
    }

    // 0x31f6e4.
    void jumpUpdate() {
        const float time = rider.clipTime;
        const float before = rider.rootTime;
        rider.rootTime = time;
        float vertical = 0.0f;
        if (rider.target.present) {
            if (rider.phase > 1.0f) {
                rider.pole = rider.target.bar;
                if (rider.target.bar >= 0) place(rider.state);
            } else if (0.0f < rider.phase) {
                vertical = rider.verticalSpeed * input.dt;
            }
        }
        Vec step = rootStep(clips, before, time, rider.rootScale, facing);
        step[1] += vertical;
        const bool freeMove = rider.target.present && !(rider.target.objectType == kPointObjectType && kAcrobatPoleTargetMoveFrame < time);
        if (freeMove) {
            for (int a = 0; a < 3; ++a) position[std::size_t(a)] += step[std::size_t(a)];
        } else {
            groundMove(step);
        }
        if ((!rider.target.present || rider.target.bar < 0) && rider.phase > 1.0f) eventOne();
    }

    // The slot clock after the state update (TRAV-S25), then the Jump Anim listener of 95, 96 and 98 for each value change of the track.
    void advance() {
        const AcrobatPoleClip& clip = clipOf(clips, rider.state);
        const float before = rider.clipTime;
        rider.clipTime += input.dt * clip.rate;
        if (rider.state == kAcrobatPoleIdleState) {
            if (clip.length > 0.0f) rider.clipTime = std::fmod(rider.clipTime, clip.length);
        } else if (rider.clipTime >= clip.length) {
            rider.clipTime = clip.length;
            rider.clipFinished = true;
        }
        if (rider.state == kAcrobatPoleJumpState || rider.state == kAcrobatPoleJumpAgainState || rider.state == kAcrobatPoleJumpToLandState)
            for (const float frame : clips.jumpEventFrames)
                if (before < frame && frame <= rider.clipTime) rider.phase += 1.0f;
    }
};

}

AcrobatPoleClips acrobatPoleClips(const assets::Animation& idle, const assets::Animation& flip, const assets::Animation& jump) {
    AcrobatPoleClips clips;
    clips.idle = {float(assets::animationLength(idle)), assets::animationFramesPerSecond(idle)};
    clips.flip = {float(assets::animationLength(flip)), assets::animationFramesPerSecond(flip)};
    clips.jump = {float(assets::animationLength(jump)), assets::animationFramesPerSecond(jump)};
    if (const std::optional<assets::RootMotion> root = assets::rootMotion(jump)) {
        clips.jumpRoot = root->translation;
        clips.jumpTravel = root->maximum[2];
    }
    float start = 0.0f, end = clips.jump.length;
    if (const assets::AnimTrack* track = findTrack(jump, kAcrobatPoleJumpTrackHash)) {
        for (std::size_t i = 1; i < track->keys.size(); ++i)
            if (track->keys[i].value != track->keys[i - 1].value) clips.jumpEventFrames.push_back(track->keys[i].time * clips.jump.length);
        bool first = true;
        for (const assets::CurveKey& key : track->keys) {
            if (first) {
                if (key.value > 0.0f) {
                    start = key.time * clips.jump.length;
                    first = false;
                }
            } else if (std::int32_t(floatBits(key.value)) < std::int32_t(kOneBits)) {
                end = key.time * clips.jump.length;
                break;
            }
        }
    }
    clips.jumpPhaseFrames = end - start;
    return clips;
}

void acrobatPoleStart(AcrobatPoleRider& rider, int pole) {
    rider = AcrobatPoleRider{};
    rider.pending = kAcrobatPoleIdleState;
    rider.state = kArrivalState;
    rider.pole = rider.from = pole;
}

bool acrobatPoleFlipTest(std::int16_t facing, bool stickHeld, std::int16_t stickHeading, float cameraYaw) {
    if (!stickHeld) return false;
    const Vec forward = facingRow(facing);
    const float side = forward[0] * std::cos(cameraYaw) + forward[2] * -std::sin(cameraYaw);
    const std::uint32_t h = (std::uint32_t(std::uint16_t(stickHeading)) - std::uint32_t(std::int32_t(cameraYaw * kHeadingScale))) & 0xffffu;
    if (h - 0x2001u < 0x3fffu) return side <= 0.0f;
    if (h - 0xa001u < 0x3fffu) return side >= 0.0f;
    return false;
}

AcrobatPoleStep acrobatPoleUpdate(AcrobatPoleRider& rider, const ClimbBarSet& set, const AcrobatPoleClips& clips, const AcrobatPoleInput& input) {
    Update update{rider, set, clips, input, vec(input.position), input.facing, {}};
    if (rider.clipFinished) update.eventOne();
    AcrobatPoleStep step;
    if (rider.pending) {
        const int next = rider.pending;
        rider.pending = 0;
        if (next == kAcrobatPoleFallState) {
            rider.state = 0;
            step.riding = false;
            step.state = kAcrobatPoleFallState;
            std::copy(update.position.begin(), update.position.end(), step.position);
            step.facing = update.facing;
            step.sounds = update.sounds;
            return step;
        }
        update.enter(next);
    }
    if (rider.state == kAcrobatPoleIdleState) update.idleUpdate();
    else if (isJumpState(rider.state)) update.jumpUpdate();
    step.state = rider.state;
    step.animation = animationOf(rider.state);
    step.clipTime = rider.clipTime;
    update.advance();
    std::copy(update.position.begin(), update.position.end(), step.position);
    step.facing = update.facing;
    step.sounds = update.sounds;
    return step;
}

}

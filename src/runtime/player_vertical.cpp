#include "runtime/player_vertical.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
void gravityStep(VerticalState& state, float dt) {
    state.fallSpeed = std::min(kTerminalFallSpeed, state.fallSpeed + kGravity * kGravityRate * dt);
    state.y -= state.fallSpeed;
    state.fallDistance += state.fallSpeed;
    state.posture = Posture::Falling;
}

GroundProbe probeFor(const VerticalState& state, const VerticalInput& input) {
    GroundProbe probe;
    probe.x = input.x;
    probe.y = state.y;
    probe.z = input.z;
    probe.forwardX = input.forwardX;
    probe.forwardZ = input.forwardZ;
    probe.rootDelta = input.rootDelta;
    probe.dt = input.dt;
    if (state.posture == Posture::Sliding) probe.minimumDistance = std::fabs(state.fallSpeed);
    if (state.posture == Posture::Falling) probe.distance = std::max(std::fabs(state.fallSpeed), kGroundMinimumDistance);
    return probe;
}

std::optional<GroundContact> probeGround(VerticalState& state, const CollisionWorld& world, const GroundProbe& probe) {
    const std::optional<GroundContact> contact = groundContact(world, probe);
    state.groundProbed = true;
    state.groundFound = contact.has_value();
    if (!contact) return std::nullopt;
    std::copy(contact->normal, contact->normal + 3, state.groundNormal);
    state.groundSurface = contact->surface;
    state.groundRecord = contact->record;
    return contact;
}

// 0x2b24bc at 0x2b2d14: 0 < normal.y < *0x376910 is the slope branch. Surface 6 sends flat ground to the slope branch too
// (a counter in ext+8 delays it); that is not replicated (docs/re/player.md, slide entry).
bool steepNormal(const float normal[3]) {
    return normal[1] > 0 && normal[1] < kFloorNormalMin;
}

// 0x22f928: a vector pointing into the plane loses its component along the normal.
void projectOntoPlane(const float normal[3], float vector[3]) {
    const float into = normal[0] * vector[0] + normal[1] * vector[1] + normal[2] * vector[2];
    if (!(into < 0)) return;
    for (int a = 0; a < 3; ++a) vector[a] -= normal[a] * into;
}

// Branch of 0x2b24bc that sets state 8: ctrl+0x2b8 = 0.3 (0.5 on surface 6), ext+0x5c = (0, -2b8, 0) projected onto the
// slope, the node stays where it is.
void startSlide(VerticalState& state, VerticalStep& step) {
    state.fallSpeed = state.groundSurface == kSlipperySurface ? kSlideSpeedSlipperySurface : kSlideSpeed;
    state.slide[0] = 0;
    state.slide[1] = -state.fallSpeed;
    state.slide[2] = 0;
    projectOntoPlane(state.groundNormal, state.slide);
    state.posture = Posture::Sliding;
    state.slideFall = false;
    step.slideStarted = true;
}

// Event 0x26 in state 8 (no ground in reach): state 5 and the gravity step on top of the slide speed 0.3.
void leaveSlideByFalling(VerticalState& state, float dt) {
    gravityStep(state, dt);
    state.slideFall = true;
    state.slideFallTicks = 0;
}

void land(VerticalState& state, const VerticalInput& input, VerticalStep& step, bool fromJump);

// State 8 update, 0x2b24bc at 0x2b2e50: the node moves along the slope by the slide speed minus what the probe
// already lowered it by, the horizontal part is slid along walls (no surface exclusions), and a second probe snaps the
// node to ground below. STAND-IN: that second probe (flags 1, 0x22a7a4) is not replicated; it found nothing in any recording, the
// replica's box probe would have found the slope itself.
void slideStep(VerticalState& state, const CollisionWorld& world, const VerticalInput& input, VerticalStep& step) {
    const GroundProbe probe = probeFor(state, input);
    const std::optional<GroundContact> contact = world.empty() ? std::nullopt : probeGround(state, world, probe);
    if (!contact) {
        leaveSlideByFalling(state, input.dt);
        return;
    }
    const float firstHit = contact->height;
    if (!(state.fallSpeed > 0)) state.fallSpeed = state.groundSurface == kSlipperySurface ? kSlideSpeedSlipperySurface : kSlideSpeed;
    const float lowered = (state.y - firstHit) - state.fallSpeed;
    state.y = firstHit;
    float vector[3] = {0, lowered, 0};
    projectOntoPlane(state.groundNormal, vector);
    normaliseVector(vector);
    for (float& component : vector) component *= -lowered;
    const float vertical = vector[1];
    float dx = vector[0], dz = vector[2];
    slideAlongWalls(world, input.x, state.y, input.z, input.forwardX, input.forwardZ, dx, dz, PlayerBox(), 0);
    const float drop = vertical;
    state.y += drop;
    state.slide[0] = dx;
    state.slide[1] = drop;
    state.slide[2] = dz;
    step.moveX = dx;
    step.moveZ = dz;
    if (steepNormal(state.groundNormal)) return;
    state.y = firstHit;
    land(state, input, step, false);
}

void land(VerticalState& state, const VerticalInput& input, VerticalStep& step, bool fromJump) {
    step.landed = true;
    step.hard = state.fallDistance >= kHardLandingDistance;
    const bool stun = step.hard || (!input.moveRequested && (fromJump || state.fallDistance > kGravity));
    const bool afterSlide = state.steeringLocked();
    state.fallSpeed = 0;
    state.fallDistance = 0;
    state.slideFall = false;
    if (stun) {
        state.posture = Posture::Landing;
        state.landingTicks = 0;
        state.hardLanding = step.hard;
        state.landingAnimation = step.hard ? state.landingAnimations.hard : state.landingAnimations.soft;
        if (afterSlide && !step.hard) state.landingAnimation = kSlideLandingStunStandIn;
    } else {
        state.posture = Posture::Ground;
    }
}
}

RootMotionSpeeds jumpSpeeds(const RootMotionSpeeds& ground) {
    return {ground.walk * kJumpHorizontalScale, ground.run * kJumpHorizontalScale, ground.sprintScale};
}

float JumpProfile::duration() const {
    return offsets.size() < 2 ? 0.0f : float(offsets.size() - 1) / framesPerSecond;
}

float JumpProfile::offsetAt(float elapsed) const {
    if (offsets.empty()) return 0.0f;
    const float last = float(offsets.size() - 1);
    float position = elapsed * framesPerSecond;
    if (position >= last) return offsets.back();
    if (position < 0) position = 0;
    const std::size_t frame = std::size_t(position);
    const float fraction = position - float(frame);
    return offsets[frame] + (offsets[frame + 1] - offsets[frame]) * fraction;
}

JumpProfile makeJumpProfile(const assets::Animation& jump, float height) {
    JumpProfile profile;
    profile.framesPerSecond = assets::animationFramesPerSecond(jump);
    const std::optional<assets::RootMotion> motion = assets::rootMotion(jump);
    if (!motion) return profile;
    const float top = motion->maximum[1] == 0 ? 1.0f : motion->maximum[1];
    profile.heightScale = height / (top - motion->minimum[1]);
    for (const auto& translation : motion->translation) profile.offsets.push_back(profile.heightScale * translation[1]);
    return profile;
}

StunAnimation stunAnimation(const assets::Animation& animation) {
    return {animation.frames > 1 ? float(animation.frames - 1) : 0.0f, assets::animationFramesPerSecond(animation)};
}

bool stunFinished(const StunAnimation& animation, std::uint32_t elapsedTicks) {
    const float position = float(elapsedTicks) * animation.framesPerSecond / float(kClockFrequency);
    return position >= animation.lastFrame;
}

LandingAnimations landingAnimations(const assets::Animation& land, const assets::Animation& fallLand) {
    return {stunAnimation(land), stunAnimation(fallLand)};
}

namespace {
VerticalStep verticalUpdateStep(VerticalState& state, const CollisionWorld& world, const VerticalInput& input) {
    VerticalStep step;
    if (state.posture != Posture::Falling) state.bouncing = state.bounceSettling = false;
    if (state.bouncing) {
        if (!(state.fallSpeed > 0)) {
            state.y -= state.fallSpeed;
            state.fallSpeed += kBounceRiseStep;
            return step;
        }
        state.bouncing = false;
        state.bounceSettling = true;
        gravityStep(state, input.dt);
        return step;
    }
    if (state.bounceSettling) {
        state.bounceSettling = false;
        return step;
    }
    if (state.posture == Posture::Sliding) {
        slideStep(state, world, input, step);
        return step;
    }
    if (state.posture == Posture::Landing) {
        state.landingTicks += input.ticks;
        if (stunFinished(state.landingAnimation, state.landingTicks)) {
            state.posture = Posture::Ground;
            step.stunEnded = true;
        }
        step.stunned = true;
        return step;
    }
    if (state.posture == Posture::Jumping) {
        state.jumpElapsed += input.dt;
        const float previousY = state.y;
        state.y = state.jumpBase + state.jump->offsetAt(state.jumpElapsed);
        const float climb = state.y - previousY;
        std::optional<GroundContact> ground;
        if (climb <= 0 && !world.empty()) {
            VerticalInput probeInput = input;
            probeInput.rootDelta = std::sqrt(input.rootDelta * input.rootDelta + climb * climb);
            GroundProbe probe = probeFor(state, probeInput);
            probe.distance.reset();
            ground = probeGround(state, world, probe);
        }
        if (ground && steepNormal(state.groundNormal)) {
            state.jump = nullptr;
            startSlide(state, step);
        } else if (ground) {
            state.y = ground->height;
            state.fallDistance = 0;
            land(state, input, step, true);
            state.jump = nullptr;
        } else if (state.jumpElapsed >= state.jump->duration()) {
            state.posture = Posture::Falling;
            state.fallSpeed = 0;
            state.fallDistance = 0;
            state.jump = nullptr;
            step.jumpEnded = true;
        }
        return step;
    }
    if (state.posture == Posture::Ground && input.startJump) {
        state.posture = Posture::Jumping;
        state.jump = input.startJump;
        state.jumpElapsed = 0;
        state.jumpBase = state.y;
        step.jumpStarted = true;
        return step;
    }
    // STAND-IN SCRIPT-S310: the rise has no ceiling test, and a raised probe that finds nothing keeps the posture.
    if (state.posture == Posture::Ground && input.rise > 0) {
        state.y += input.rise;
        if (!input.horizontalMove) {
            state.groundKept = true;
            return step;
        }
        const std::optional<GroundContact> raised = world.empty() ? std::nullopt : probeGround(state, world, probeFor(state, input));
        if (raised && steepNormal(state.groundNormal)) startSlide(state, step);
        else if (raised) state.y = raised->height;
        return step;
    }
    if (state.posture == Posture::Ground && input.rootDelta <= 0) return step;
    const std::optional<GroundContact> ground = world.empty() ? std::nullopt : probeGround(state, world, probeFor(state, input));
    if (ground && steepNormal(state.groundNormal)) {
        startSlide(state, step);
        return step;
    }
    if (ground) {
        state.y = ground->height;
        if (state.posture == Posture::Falling) land(state, input, step, false);
        return step;
    }
    if (!world.empty()) gravityStep(state, input.dt);
    return step;
}
}

VerticalStep verticalUpdate(VerticalState& state, const CollisionWorld& world, const VerticalInput& input) {
    state.groundProbed = state.groundFound = state.groundKept = false;
    VerticalStep step = verticalUpdateStep(state, world, input);
    if (state.posture == Posture::Falling && state.slideFall) state.slideFallTicks += input.ticks;
    locomotionStepEnded(state.placement);
    return step;
}

}

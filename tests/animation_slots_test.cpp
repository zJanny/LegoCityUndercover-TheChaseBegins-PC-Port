// The animation player of an entity (docs/re/npc-animation.md): slots, modes, offsets, stop and the WaitForCurrent test.
// Provenance: the rules are transcribed from 0x245b40, 0x24cbd4, 0x22f560, 0x225c5c / 0x24f8fc, 0x22c2fc and 0x2a06a8
// (REA/Ghidra pseudo-code, llvm-objdump). The recorded slot is 1-1_AI_DonutLady's in oracle state m11-3-repair-van-marker
// (`ctrace 0x24cbd4 1` for the slot pointers, `read` of slot 0x08a6f460): +4 = 0x2401 (mode 1, bit 13), first 0, end 42,
// rate 15, rate x speed 15.0, for postboxstuck_idle_minifig (42 frames, joint table bit 29 set).
#include "runtime/animation_slots.hpp"
#include "runtime/natives_animation.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

constexpr std::int64_t kTicks = kAnimationClockTicksPerSecond;
constexpr AnimationClipInfo kStuck{7, 42, 15, true};
constexpr AnimationClipInfo kOpen{8, 30, 15, false};

void recordedDonutLadySlot() {
    AnimationSlots slots;
    bool scriptLoaded = false;
    playCharacterAnimation(slots, scriptLoaded, kStuck, false, true, 1000);
    const AnimationSlot& slot = *slots.current();
    require(slot.mode == kAnimationLoop && slot.wrap && slot.first == 0 && slot.end == 42 && slot.rate == 15 && slot.rateSpeed == 15.0f,
            "slot fields of the recorded PlayCharAnim");
    require(near(slot.blend, 0.1f) && slot.start == 1000 && slot.stop == -1 && scriptLoaded, "blend 0.1, start clock, loaded by the call");
}

void loopWithoutDataLoopBitGetsOneMoreFrame() {
    AnimationSlots slots;
    slots.start(kOpen, kAnimationLoop, 0, kAnimationFrameDefault, 1.0f, 0.0f, 0);
    require(slots.current()->wrap && slots.current()->end == 31, "mode 1 sets bit 13 and end + 1 (0x245e68)");
    slots.start(kOpen, kAnimationOnce, 0, kAnimationFrameDefault, 1.0f, 0.0f, 0);
    require(!slots.current()->wrap && slots.current()->end == 30, "mode 0 keeps the length");
}

void onceClampsAndFinishes() {
    AnimationSlots slots;
    slots.start(kOpen, kAnimationOnce, 0, kAnimationFrameDefault, 2.0f, 0.1f, 0);
    require(near(AnimationSlots::position(*slots.current(), kTicks / 2).frame, 15.0f), "rate x speed x time");
    require(slots.kind(kTicks / 2) == kAnimationOnce, "still playing");
    const AnimationPosition end = AnimationSlots::position(*slots.current(), kTicks);
    require(end.finished && near(end.frame, 30.0f), "p >= span finishes at the span");
    require(slots.kind(kTicks) == kAnimationStopped && slots.currentNearEnd(kTicks), "kind 6 when finished, the wait passes");
    require(!slots.currentNearEnd(kTicks / 2), "a playing mode 0 animation blocks the wait");
}

void loopWithWrapFrame() {
    AnimationSlots slots;
    slots.start(kStuck, kAnimationLoop, 0, kAnimationFrameDefault, 1.0f, 0.1f, 0);
    const auto at = [&](float frames) { return AnimationSlots::position(*slots.current(), std::int64_t(frames * kTicks / 15)); };
    require(near(at(10).frame, 10.0f) && at(10).loops == 0, "first pass");
    require(near(at(42.5f).frame, 42.5f) && at(42.5f).loops == 0, "the second pass maps [0, 1) to [span, span + 1)");
    require(near(at(50).frame, 8.0f) && at(50).loops == 1, "then wraps");
    require(slots.kind(100 * kTicks) == kAnimationLoop, "never finishes");
    require(slots.currentNearEnd(std::int64_t(41.5 * kTicks / 15)) && !slots.currentNearEnd(std::int64_t(30 * kTicks / 15)),
            "the wait passes within 1 frame of the length");
}

void reverseAndPingPong() {
    AnimationSlots slots;
    slots.start(kOpen, kAnimationReverse, 0, kAnimationFrameDefault, 1.0f, 0.0f, 0);
    require(near(AnimationSlots::position(*slots.current(), kTicks).frame, 15.0f), "mode 2 runs backwards");
    require(slots.kind(3 * kTicks) == kAnimationStopped, "mode 2 finishes at 0");
    slots.start(kOpen, kAnimationPingPong, 0, kAnimationFrameDefault, 1.0f, 0.0f, 0);
    const AnimationPosition back = AnimationSlots::position(*slots.current(), 3 * kTicks);
    require(near(back.frame, 15.0f) && back.loops == 0, "mode 3 comes back over the period 2 x span");
    slots.start(kOpen, kAnimationReverseLoop, 0, kAnimationFrameDefault, 1.0f, 0.0f, 0);
    require(slots.current()->end == 31 && near(AnimationSlots::position(*slots.current(), kTicks).frame, 16.0f), "mode 5 reversed loop");
}

void stopFreezesThePose() {
    AnimationSlots slots;
    slots.start(kStuck, kAnimationLoop, 0, kAnimationFrameDefault, 1.0f, 0.1f, 0);
    slots.stop(kTicks);
    require(slots.current()->mode == kAnimationStopped && slots.current()->stop == kTicks, "0x225c5c: pause, mode 6");
    require(near(AnimationSlots::position(*slots.current(), 2 * kTicks).frame, 15.0f), "the position stays at the stop clock");
    require(slots.kind(2 * kTicks) == kAnimationStopped && slots.currentNearEnd(2 * kTicks), "kind 6");
}

void offsetMovesTheStart() {
    AnimationSlots slots;
    AnimationRequest request;
    request.mode = kAnimationOnce;
    request.speed = 1.0f;
    request.blend = 0.0f;
    request.offset = 12;
    playObjectAnimation(slots, kOpen, request, 5000);
    require(slots.current()->start == 5000 - 12 * kTicks / 15, "0x22c2fc: start = now - ticks x offset / rate x speed");
    require(near(AnimationSlots::position(*slots.current(), 5000).frame, 12.0f), "the animation starts at the offset");
    request.mode = kAnimationReverse;
    request.offset = 10;
    playObjectAnimation(slots, kOpen, request, 5000);
    require(near(AnimationSlots::position(*slots.current(), 5000).frame, 10.0f), "a reversed animation starts at the offset too");
}

void samePlayRestartsWithoutANewSlot() {
    AnimationSlots slots;
    bool scriptLoaded = false;
    playCharacterAnimation(slots, scriptLoaded, kStuck, false, true, 0);
    playCharacterAnimation(slots, scriptLoaded, kStuck, true, false, 3000);
    require(slots.count() == 1 && slots.current()->mode == kAnimationOnce && slots.current()->start == 3000,
            "0x295a8c: the current script animation gets the mode and starts again at frame 0");
    playCharacterAnimation(slots, scriptLoaded, kOpen, false, false, 4000);
    require(slots.count() == 2 && slots.current()->clip == kOpen.clip && scriptLoaded, "another animation takes a new slot");
}

void fourSlotsNewestFirst() {
    AnimationSlots slots;
    for (std::uint32_t k = 0; k < 6; ++k) slots.start({k, 10, 15, true}, kAnimationOnce, 0, kAnimationFrameDefault, 1.0f, 0.1f, k);
    require(slots.count() == AnimationSlots::kCapacity, "four slots");
    require(slots.slot(0).clip == 5 && slots.slot(3).clip == 2 && slots.slot(0).startId == 6, "newest first, the oldest dropped, start ids count up");
}

void emptyPlayerIsFinished() {
    AnimationSlots slots;
    require(slots.kind(0) == kAnimationStopped && slots.currentNearEnd(0), "no slot: kind 6, the wait passes");
}

}

int main() {
    try {
        recordedDonutLadySlot();
        loopWithoutDataLoopBitGetsOneMoreFrame();
        onceClampsAndFinishes();
        loopWithWrapFrame();
        reverseAndPingPong();
        stopFreezesThePose();
        offsetMovesTheStart();
        samePlayRestartsWithoutANewSlot();
        fourSlotsNewestFirst();
        emptyPlayerIsFinished();
    } catch (const std::exception& error) {
        std::cerr << "animation-slots-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "animation-slots-test ok\n";
    return 0;
}

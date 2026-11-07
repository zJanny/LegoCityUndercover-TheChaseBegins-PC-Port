// The gate, the sleep branch and the return to the spawn point of the idle decision (src/runtime/goon_idle_return.*, applied by
// src/runtime/goon_conduct.*) on made-up characters: the rules are those read from game.arm.elf by 20261107-traffic-E (2026-11-07, llvm-objdump
// of 0x2a9874, 0x241098, 0x22b62c, 0x2bba40, 0x2c5cac and 0x1f5c3c; docs/re/scripts-missions-1-4-spawners-conduct.md), not measured in the original.
#include "runtime/goon_conduct.hpp"
#include "runtime/goon_idle_return.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

AiConductEntity goon(const std::string& name, float x, float z) {
    AiConductEntity entity;
    entity.name = name;
    entity.params.badGuy = true;
    entity.params.viewDistance = 50;
    entity.position = {x, 0.0f, z};
    return entity;
}

AiConductEntity thePlayer(float x, float z) {
    AiConductEntity entity;
    entity.name = "player1";
    entity.player = true;
    entity.position = {x, 0.0f, z};
    return entity;
}

const AiConductBoundTest kNoBound = [](std::size_t, const std::array<float, 3>&) { return false; };

std::size_t countKind(const std::vector<AiConductEvent>& events, AiConductEventKind kind) {
    std::size_t n = 0;
    for (const AiConductEvent& event : events) n += event.kind == kind;
    return n;
}

void gate() {
    require(!aiIdleGate(0) && aiIdleGate(1) && aiIdleGate(0xffffffffu), "0x2a9898: the decision ends when 0x241098 answers 0");
    EngagedTable table;
    AiConduct conduct;
    conduct.reset(table, {goon("goon", 0.0f, 0.0f), thePlayer(10.0f, 0.0f)});
    conduct.setClock(0);
    conduct.update(kNoBound);
    require(conduct.takeEvents().empty() && conduct.entities()[0].aiState == 0, "with the clock at 0 the goon neither engages nor sees");
    conduct.setClock(2);
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == kAiCombatState, "with the clock running it sees the player");
}

void sleepSteps() {
    require(aiSleepClipName("chars/goon/sleep01") == "sleep01" && aiSleepClipName("sleep02") == "sleep02", "0x2a99d4: the text after the last '/'");
    int asked = 0;
    const auto seen = [&] { ++asked; return true; };
    const auto unseen = [&] { ++asked; return false; };

    AiSleepStep step = aiSleepStep({kAiSleepNotYet, 1, false, true, false}, unseen);
    require(!step.awake && step.bits == kAiSleepAsleep && step.clip == AiSleepClip::Sleep && asked == 1, "not seen: the sleep clip looped, bits 3, no search");
    step = aiSleepStep({kAiSleepNotYet, 1, false, true, false}, seen);
    require(!step.awake && step.bits == kAiSleepWaking && step.clip == AiSleepClip::Wake, "seen with a wake clip: bits 1 and the wake clip once");
    step = aiSleepStep({kAiSleepNotYet, 0, false, false, false}, seen);
    require(step.awake && step.bits == kAiSleepAwake && step.clip == AiSleepClip::None, "seen without a wake clip: awake at once (0x2a9a20)");
    asked = 0;
    step = aiSleepStep({kAiSleepNotYet, 1, true, true, false}, seen);
    require(!step.awake && step.bits == kAiSleepAsleep && step.clip == AiSleepClip::Sleep && asked == 0, "a friendly character is not tested and sleeps");
    step = aiSleepStep({kAiSleepNotYet, 2, false, true, false}, seen);
    require(step.awake && step.bits == kAiSleepAwake && step.clip == AiSleepClip::None && asked == 0, "locomotion state above 1: awake (0x2a9934)");
    step = aiSleepStep({kAiSleepWaking, 1, false, true, false}, seen);
    require(!step.awake && step.bits == kAiSleepWaking && asked == 0, "waking while the clip plays: no search");
    step = aiSleepStep({kAiSleepWaking, 1, false, true, true}, seen);
    require(!step.awake && step.bits == kAiSleepAwake, "the wake clip ended (0x22f560 answers 6): bits 2, the search from the next update");
    step = aiSleepStep({kAiSleepAsleep, 1, false, true, false}, seen);
    require(step.awake && step.bits == kAiSleepAsleep && step.clip == AiSleepClip::None && asked == 0, "bits 3: the decision goes on with the search (0x2a98fc)");
    step = aiSleepStep({kAiSleepAwake, 1, false, true, false}, seen);
    require(step.awake && step.bits == kAiSleepAwake && asked == 0, "bits 2: awake");
}

void sleeper() {
    EngagedTable table;
    AiConduct conduct;
    AiConductEntity sleeping = goon("goon", 0.0f, 0.0f);
    sleeping.params.idleReturn.sleepAnim = "chars/goon/sleep";
    sleeping.params.idleReturn.sleepAnimWake = "chars/goon/wake";
    conduct.reset(table, {sleeping, thePlayer(100.0f, 0.0f)});
    conduct.update(kNoBound);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(events.size() == 2 && events[0].kind == AiConductEventKind::SleepClip && events[0].clip == "sleep" && events[0].loop &&
                events[1].kind == AiConductEventKind::Engaged,
            "the player out of sight: the sleep clip, then step 5 takes a slot");
    conduct.entities()[1].position = {10.0f, 0.0f, 0.0f};
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == kAiCombatState && conduct.entities()[0].seenBits == kAiSeenBits,
            "asleep (bits 3) the search runs and the sighting sets bits 2");

    conduct.reset(table, {sleeping, thePlayer(10.0f, 0.0f)});
    conduct.update(kNoBound);
    events = conduct.takeEvents();
    require(countKind(events, AiConductEventKind::SleepClip) == 1 && events[0].clip == "wake" && !events[0].loop &&
                countKind(events, AiConductEventKind::Sighted) == 0 && conduct.entities()[0].seenBits == kAiSleepWaking,
            "the player in sight: the wake clip and no search in that update");
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == 0 && conduct.entities()[0].seenBits == kAiSleepAwake, "the clip ended: awake, still no search");
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == kAiCombatState, "awake: the search sees the player");
}

void returnSteps() {
    const std::array<float, 3> here{0.0f, 0.0f, 0.0f};
    require(!aiReturnRequested(false, true, here, std::array<float, 3>{1.9f, 0.0f, 0.0f}), "nearer than 2.0: no request (0x2a9d38)");
    require(aiReturnRequested(false, true, here, std::array<float, 3>{2.0f, 0.0f, 0.0f}), "at 2.0: AI state 15");
    require(!aiReturnRequested(false, true, here, std::array<float, 3>{1.0f, 50.0f, 1.0f}), "0x22b62c measures x and z only");
    require(!aiReturnRequested(false, false, here, std::array<float, 3>{5.0f, 0.0f, 0.0f}), "without ReturnToSpawnPoint: no request");
    require(aiReturnRequested(true, false, here, here), "ctrl+0xd4 set: AI state 15 wherever the character stands (0x2a9d10)");

    AiIdleReturnParams params;
    params.waitTime = 40;
    require(aiReturnCountdown(false, params) == 15 && aiReturnCountdown(true, params) == 40, "0x2bba40: 15, or Wait_Time with ctrl+0xd4");

    std::uint32_t countdown = 2;
    require(!aiReturnWaitStep(false, true, countdown) && countdown == 1, "0x2c5cd8: counts down");
    require(aiReturnWaitStep(false, true, countdown) && countdown == 0, "at 0: AI state 14");
    countdown = 0;
    require(aiReturnWaitStep(true, false, countdown) && countdown == 0xffffffffu, "signed: from 0 to -1, still at or below 0");
    countdown = 5;
    require(!aiReturnWaitStep(false, false, countdown) && countdown == 5, "neither ctrl+0xd4 nor ReturnToSpawnPoint: no count");
}

void returnToSpawn() {
    EngagedTable table;
    AiConduct conduct;
    conduct.reset(table, {goon("goon", 0.0f, 0.0f), thePlayer(100.0f, 0.0f)});
    conduct.update(kNoBound);
    require(conduct.entities()[0].engaged, "at its spawn point the goon takes a slot");
    conduct.takeEvents();
    conduct.entities()[0].position = {3.0f, 0.0f, 0.0f};
    conduct.update(kNoBound);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(events.size() == 2 && events[0].kind == AiConductEventKind::ReturnToSpawn && events[1].kind == AiConductEventKind::Released,
            "engaged, shown and 3.0 away: AI state 15, whose enter releases the slot");
    require(conduct.entities()[0].aiState == kAiReturnToSpawnState && conduct.entities()[0].countdown == 15 && !conduct.entities()[0].engaged,
            "ctrl+0x84 = 15");
    for (int i = 0; i < 14; ++i) conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == kAiReturnToSpawnState && conduct.entities()[0].countdown == 1, "fourteen decisions of state 15");
    conduct.update(kNoBound);
    events = conduct.takeEvents();
    require(conduct.entities()[0].aiState == kAiReturnedState && countKind(events, AiConductEventKind::Returned) == 1, "the fifteenth requests AI state 14");

    conduct.reset(table, {goon("goon", 0.0f, 0.0f), thePlayer(100.0f, 0.0f)});
    conduct.update(kNoBound);
    conduct.entities()[0].position = {3.0f, 0.0f, 0.0f};
    conduct.update(kNoBound);
    conduct.takeEvents();
    conduct.entities()[1].position = {40.0f, 0.0f, 0.0f};
    conduct.update(kNoBound);
    events = conduct.takeEvents();
    require(events.size() == 1 && events[0].kind == AiConductEventKind::Sighted && events[0].combat && conduct.entities()[0].aiState == kAiCombatState &&
                conduct.entities()[0].target == "player1" && !conduct.entities()[0].engaged,
            "the watch 0x1f5c3c: a target in sight starts the fight with ctrl+0xd8 bit 5 set by the watch, no slot taken");

    AiConductEntity outside = goon("goon", 0.0f, 0.0f);
    outside.aiState = kAiReturnToSpawnState;
    conduct.reset(table, {outside, thePlayer(100.0f, 0.0f)});
    conduct.update(kNoBound);
    require(conduct.entities()[0].countdown == 14, "a state 15 set from outside runs the enter (15) before the first decision");
}

}

int main() {
    try {
        gate();
        sleepSteps();
        sleeper();
        returnSteps();
        returnToSpawn();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "goon-idle-return: ok\n";
    return 0;
}

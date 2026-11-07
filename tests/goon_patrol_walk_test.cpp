// AI state 14, the walk to a patrol node or back to the spawn point, and the watch of states 14 and 15 with its angle test
// (src/runtime/ai_patrol_walk.*, applied by src/runtime/goon_conduct.*) on made-up characters. The rules are those read from game.arm.elf by
// 20261107-traffic2-C (2026-11-07, llvm-objdump of 0x2a11a0, 0x2b68e8, 0x1fae48, 0x1f5c3c, 0x15c1b0, 0x1f8d5c, 0x23cee8,
// docs/re/scripts-missions-1-4-spawners-conduct.md), not measured in the original.
#include "runtime/ai.hpp"
#include "runtime/goon_conduct.hpp"

#include <cmath>
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

struct FakeWalk {
    bool pathExists = true;
    std::vector<std::array<float, 3>> starts;
    std::vector<int> codes;
    std::size_t followed = 0;
};

void connect(AiConduct& conduct, FakeWalk& walk) {
    AiConductWalker walker;
    walker.start = [&walk](const AiConductEntity&, const std::array<float, 3>& destination) {
        walk.starts.push_back(destination);
        return walk.pathExists;
    };
    walker.follow = [&walk](const AiConductEntity&) {
        const int code = walk.followed < walk.codes.size() ? walk.codes[walk.followed] : 1;
        ++walk.followed;
        return code;
    };
    conduct.setWalker(walker);
}

void pureSteps() {
    require(aiFollowOutcome(0) == AiFollowOutcome::Arrived && aiFollowOutcome(1) == AiFollowOutcome::Blocked && aiFollowOutcome(2) == AiFollowOutcome::Blocked &&
                aiFollowOutcome(3) == AiFollowOutcome::Moving && aiFollowOutcome(4 | 0x1200) == AiFollowOutcome::Moving && aiFollowOutcome(0x0100) == AiFollowOutcome::Arrived,
            "0x2b6964: the low byte decides: 0 arrived, 1 and 2 blocked, else moving; the heading byte above is ignored");

    AiPatrolEnterInput input;
    input.returnToSpawnPoint = true;
    AiPatrolEnterStep step = aiPatrolEnter(input);
    require(step.source == AiPatrolSource::SpawnPoint && !step.walk && step.patrolObject == 0, "0x2a1280: no node, ReturnToSpawnPoint: back to the spawn point, a run");
    input.dontRun = true;
    require(aiPatrolEnter(input).walk, "0x2a1208: AIDontRun makes the walk back a walk");
    input.returnToSpawnPoint = false;
    input.dontRun = false;
    step = aiPatrolEnter(input);
    require(step.source == AiPatrolSource::None, "no node and no ReturnToSpawnPoint: no destination (0x2a1288)");
    input.pathParameter = 77;
    step = aiPatrolEnter(input);
    require(step.source == AiPatrolSource::PatrolObject && step.patrolObject == 77 && step.walk && step.patrolling,
            "0x2a11cc: ctrl+0xd4 takes AIPatrolPath, the walk bit is set, 'Patrolling to'");
    input.patrolObject = 5;
    input.returnToSpawnPoint = true;
    step = aiPatrolEnter(input);
    require(step.source == AiPatrolSource::PatrolObject && step.patrolObject == 5 && !step.walk && !step.patrolling,
            "0x2a11b8: with ctrl+0xd4 set the parameter is not read and the walk bit stays");

    AiPatrolArrival arrival = aiPatrolArrival(0, true, 9);
    require(arrival.idle, "0x2b69a0: no node, ReturnToSpawnPoint: AI state 0");
    arrival = aiPatrolArrival(5, true, 9);
    require(!arrival.idle && arrival.patrolObject == 9, "0x2b69e8: a node: ctrl+0xd4 takes next_Node, AI state 15");
    arrival = aiPatrolArrival(5, false, 0);
    require(!arrival.idle && arrival.patrolObject == 0, "the last node of a path: ctrl+0xd4 becomes 0, AI state 15");

    std::uint32_t countdown = 2;
    require(!aiPatrolBlockedStep(countdown, 7) && countdown == 1, "0x2b6a40: above 0 it counts down");
    countdown = 0;
    require(aiPatrolBlockedStep(countdown, 7) && countdown == 17, "0x2b6ac8: at 0 the search runs and the count restarts at 10 + random % 10");
    countdown = 0xffffffffu;
    require(aiPatrolBlockedStep(countdown, 23) && countdown == 13, "signed: -1 is at or below 0");
}

void angleSteps() {
    const float angle = aiWatchAngle(kAiWatchAngleDegrees);
    require(std::fabs(angle - 0.785398f) < 1e-5f, "45 degrees x pi x 1/180 (0x1f8d6c..0x1f8d94)");
    const std::array<float, 3> viewer{0, 0, 0}, forward{0, 0, 1};
    require(aiAngleTest(viewer, forward, {0, 0, 10}, angle), "straight ahead passes");
    require(aiAngleTest(viewer, forward, {9, 0, 10}, angle), "42 degrees to the side passes");
    require(!aiAngleTest(viewer, forward, {10, 0, 9}, angle), "48 degrees to the side fails");
    require(!aiAngleTest(viewer, forward, {0, 0, -10}, angle), "behind fails");
    require(aiAngleTest(viewer, forward, {0, 0, 0}, angle), "a target on the viewer normalises to (0, 0, 1)");
}

void walkBack() {
    EngagedTable table;
    AiConduct conduct;
    FakeWalk walk;
    connect(conduct, walk);
    conduct.reset(table, {goon("goon", 0.0f, 0.0f), thePlayer(500.0f, 0.0f)});
    conduct.update(kNoBound);
    conduct.takeEvents();
    conduct.entities()[0].position = {3.0f, 0.0f, 0.0f};
    conduct.update(kNoBound);
    for (int i = 0; i < 15; ++i) conduct.update(kNoBound);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(conduct.entities()[0].aiState == kAiReturnedState && countKind(events, AiConductEventKind::Returned) == 1 &&
                countKind(events, AiConductEventKind::PatrolWalk) == 1,
            "the wait of state 15 ends: AI state 14, whose enter plans the walk");
    require(walk.starts.size() == 1 && walk.starts[0] == std::array<float, 3>({0.0f, 0.0f, 0.0f}), "the destination is the spawn point");
    require(conduct.entities()[0].engaged, "the enter engages a character without a path record (0x2a1230)");

    walk.codes = {4, 4, 0};
    conduct.update(kNoBound);
    conduct.update(kNoBound);
    require(conduct.entities()[0].aiState == kAiReturnedState && walk.followed == 2, "moving codes keep the state");
    conduct.update(kNoBound);
    events = conduct.takeEvents();
    require(conduct.entities()[0].aiState == 0 && countKind(events, AiConductEventKind::WalkEnded) == 1 && countKind(events, AiConductEventKind::Idle) == 1,
            "arrived without a node: AI state 0 (0x2b69b0..0x2b6a2c)");
    require(countKind(events, AiConductEventKind::Released) == 1 && !conduct.entities()[0].engaged, "the enter of state 0 releases the slot (0x289650)");
    for (const AiConductEvent& event : events) require(event.kind != AiConductEventKind::WalkEnded || !event.toWait, "no wait after the walk back");
}

void blocked() {
    EngagedTable table;
    AiConduct conduct;
    FakeWalk walk;
    walk.pathExists = false;
    connect(conduct, walk);
    AiConductEntity entity = goon("goon", 0.0f, 0.0f);
    entity.aiState = kAiReturnedState;
    conduct.reset(table, {entity, thePlayer(500.0f, 0.0f)});
    conduct.update(kNoBound);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(countKind(events, AiConductEventKind::CantGetThere) == 1, "0x2a12c0: the path request answered 1: 'I can't get there'");
    require(conduct.entities()[0].aiState == 0 && countKind(events, AiConductEventKind::Idle) == 1,
            "blocked at count 0 with nothing in sight: idle (0x2b69a0 .. 0x2246c0), the count restarted at 10..19");
    require(countKind(events, AiConductEventKind::Released) == 1 && !conduct.entities()[0].engaged, "the enter of state 0 releases the slot (0x289650)");
    require(conduct.entities()[0].countdown >= 10 && conduct.entities()[0].countdown <= 19, "ctrl+0x84 = 10 + random % 10");

    entity.aiState = kAiReturnedState;
    entity.countdown = 2;
    conduct.reset(table, {entity, thePlayer(40.0f, 0.0f)});
    conduct.update(kNoBound);
    conduct.takeEvents();
    require(conduct.entities()[0].aiState == kAiReturnedState, "blocked with the count above 0: it counts down and waits");
}

void blockedSighting() {
    EngagedTable table;
    AiConduct conduct;
    FakeWalk walk;
    connect(conduct, walk);
    AiConductEntity entity = goon("goon", 0.0f, 0.0f);
    entity.aiState = kAiReturnedState;
    conduct.reset(table, {entity, thePlayer(40.0f, 0.0f)});
    conduct.update(kNoBound);
    const std::vector<AiConductEvent> events = conduct.takeEvents();
    require(countKind(events, AiConductEventKind::Sighted) == 1 && conduct.entities()[0].aiState == kAiCombatState,
            "blocked at count 0 with the player in sight: the sighting 0x21102c (AI state 2) instead of idle");
}

void patrol() {
    EngagedTable table;
    AiConduct conduct;
    FakeWalk walk;
    connect(conduct, walk);
    conduct.setPatrolNodes([](std::uint32_t hash) -> std::optional<AiPatrolNode> {
        if (hash == 1) return AiPatrolNode{{10.0f, 0.0f, 0.0f}, 2};
        if (hash == 2) return AiPatrolNode{{20.0f, 0.0f, 0.0f}, 0};
        return std::nullopt;
    });
    AiConductEntity entity = goon("goon", 0.0f, 0.0f);
    entity.params.patrolPath = true;
    entity.params.patrolPathHash = 1;
    entity.aiState = aiStartRequest(entity.params);
    conduct.reset(table, {entity, thePlayer(500.0f, 0.0f)});
    require(entity.aiState == kAiReturnedState, "AIPatrolPath: 0x1c6e84 requests AI state 14");
    walk.codes = {0};
    conduct.update(kNoBound);
    std::vector<AiConductEvent> events = conduct.takeEvents();
    require(walk.starts.size() == 1 && walk.starts[0] == std::array<float, 3>({10.0f, 0.0f, 0.0f}) && conduct.entities()[0].walk,
            "the enter walks to the node AIPatrolPath names, at a walk");
    require(countKind(events, AiConductEventKind::WalkEnded) == 1 && conduct.entities()[0].aiState == kAiReturnToSpawnState &&
                conduct.entities()[0].patrolObject == 2,
            "arrived at a node: next_Node becomes the node and AI state 15 follows");
    require(conduct.entities()[0].countdown == 15, "the wait of state 15 with a node is Wait_Time (default 15)");
    for (int i = 0; i < 15; ++i) conduct.update(kNoBound);
    require(walk.starts.size() == 2 && walk.starts[1] == std::array<float, 3>({20.0f, 0.0f, 0.0f}), "the wait ends: AI state 14 walks to next_Node");
}

void watchEvent() {
    EngagedTable table;
    AiConduct conduct;
    FakeWalk walk;
    connect(conduct, walk);
    AiConductEntity entity = goon("goon", 0.0f, 0.0f);
    entity.aiState = kAiReturnedState;
    conduct.reset(table, {entity, thePlayer(500.0f, 0.0f)});
    walk.codes = {4, 4};
    conduct.update(kNoBound);
    conduct.takeEvents();
    conduct.entities()[1].position = {0.0f, 0.0f, 40.0f};
    conduct.update(kNoBound);
    const std::vector<AiConductEvent> events = conduct.takeEvents();
    require(events.size() == 2 && events[0].kind == AiConductEventKind::PlayerCaught && events[1].kind == AiConductEventKind::Sighted && events[1].combat &&
                conduct.entities()[0].aiState == kAiCombatState,
            "moving: the watch raises PlayerCaught and starts the fight (0x1f5c3c)");
}

void angleFlag() {
    for (const bool flag : {false, true}) {
        EngagedTable table;
        AiConduct conduct;
        FakeWalk walk;
        connect(conduct, walk);
        AiConductEntity entity = goon("goon", 0.0f, 0.0f);
        entity.aiState = kAiReturnedState;
        entity.params.patrolAlertOnLos = flag;
        entity.forward = {0.0f, 0.0f, 1.0f};
        conduct.reset(table, {entity, thePlayer(0.0f, -40.0f)});
        walk.codes = {4};
        conduct.update(kNoBound);
        const std::vector<AiConductEvent> events = conduct.takeEvents();
        require(countKind(events, AiConductEventKind::PlayerCaught) == (flag ? 0u : 1u),
                flag ? "AIPatrolAlertOnLOS: a player behind the goon is not seen (0x15c1b0)" : "without the flag the angle is not tested");
    }
    EngagedTable table;
    AiConduct conduct;
    FakeWalk walk;
    connect(conduct, walk);
    AiConductEntity entity = goon("goon", 0.0f, 0.0f);
    entity.aiState = kAiReturnedState;
    entity.params.patrolAlertOnLos = true;
    conduct.reset(table, {entity, thePlayer(0.0f, 40.0f)});
    walk.codes = {4};
    conduct.update(kNoBound);
    require(countKind(conduct.takeEvents(), AiConductEventKind::PlayerCaught) == 1, "AIPatrolAlertOnLOS: a player in front is seen");
}

void coreWalk() {
    AiCore ai;
    require(ai.startPatrolWalk("goon", {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, true), "without nav grids the request has a straight path (STAND-IN SCRIPT-S31)");
    require(ai.character("goon")->state == AiState::Patrol, "the state is 14");
    const float dt = 1.0f / kAiUpdatesPerSecond;
    int steps = 0;
    while (ai.followPatrolWalk("goon", true, dt) == 4 && steps < 100) ++steps;
    require(steps == 10 && (*ai.character("goon")->position)[0] == 1.0f, "a walk of 0.11 per update covers 1.0 in ten steps, the next call answers 0");
    require(ai.followPatrolWalk("goon", true, dt) == 0, "arrived stays arrived");
    ai.startPatrolWalk("goon", {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, false);
    ai.followPatrolWalk("goon", false, dt);
    require(std::fabs((*ai.character("goon")->position)[0] - 0.23f) < 1e-5f, "a run is 0.23 per update");
    ai.startConductState("goon", AiState::Idle);
    require(ai.followPatrolWalk("goon", true, dt) == 1, "another state ends the walk: the follower answers 1");
}

}

int main() {
    try {
        pureSteps();
        angleSteps();
        walkBack();
        blocked();
        blockedSighting();
        patrol();
        watchEvent();
        angleFlag();
        coreWalk();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "goon-patrol-walk: ok\n";
    return 0;
}

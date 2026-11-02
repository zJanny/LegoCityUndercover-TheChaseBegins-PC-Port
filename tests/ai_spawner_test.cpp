// The AI spawner class (kind 14) on made-up spawners: the rules transcribed from the post-link 0x19862c, the handler 0x1b4fb0, the updates 0x288f18 /
// 0x288fbc and the state change 0x268978 (llvm-objdump of game.arm.elf, 20261102-mission14-A, 2026-11-02). The values are hand-evaluated from that code,
// not measured in the original.
#include "runtime/ai_spawner.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
using runtime::AiSpawner;
using runtime::AiSpawnerSet;
using runtime::AiSpawnerState;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct FakeWorld : runtime::AiSpawnerWorld {
    std::set<std::string> heldSet, disabledSet;
    std::vector<std::string> log;
    bool idle = true;
    bool releaseWorks = true;
    bool inside = true;
    bool held(const std::string& c) const override { return heldSet.count(c) != 0; }
    bool disabled(const std::string& c) const override { return disabledSet.count(c) != 0; }
    void hold(const std::string& c) override { heldSet.insert(c), disabledSet.insert(c), log.push_back("hold " + c); }
    void disable(const std::string& o) override { disabledSet.insert(o), log.push_back("disable " + o); }
    void restore(const std::string& c) override { heldSet.erase(c), disabledSet.erase(c), log.push_back("restore " + c); }
    bool controllerAllowsSpawn(const std::string&) const override { return true; }
    bool inView(const assets::Matrix&) const override { return false; }
    void startState(const AiSpawner& s, std::size_t slot) override { log.push_back("start state " + std::to_string(s.start.index) + " " + s.slots[slot].character); }
    bool release(const std::string& c, const assets::Matrix& m) override {
        log.push_back("release " + c + " at " + std::to_string(int(m[12])));
        if (!releaseWorks) return false;
        heldSet.erase(c), disabledSet.erase(c);
        return true;
    }
    void aiSpawned(const std::string& o) override { log.push_back("AISpawned " + o); }
    void activateReference(const std::string& s) override { idle = false, log.push_back("fire " + s); }
    void deactivateReference(const std::string& s) override { idle = true, log.push_back("unfire " + s); }
    bool referenceIdle(const std::string&) const override { return idle; }
    void reveal(const std::string& o, const std::string& c) override { disabledSet.erase(o), log.push_back("reveal " + o + " at " + c); }
    bool playerInside(const AiSpawner&) const override { return inside; }
    std::size_t count(const std::string& prefix) const {
        std::size_t n = 0;
        for (const std::string& line : log) n += line.rfind(prefix, 0) == 0;
        return n;
    }
};

AiSpawner spawner(std::vector<std::string> characters) {
    AiSpawner s;
    s.instance = "spawner";
    s.matrix[12] = 7;
    s.params.startActive = false;
    s.params.startSpawned = false;
    s.params.maxSpawn = -1;
    s.params.waitForDead = true;
    s.state = s.requested = AiSpawnerState::Idle;
    s.maxSpawn = s.params.maxSpawn;
    for (std::string& c : characters) s.slots.push_back({std::move(c), 0, {}});
    return s;
}

void heldUntilTriggeredAndChainOnAllDone() {
    AiSpawnerSet set;
    set.add(spawner({"a", "b"}));
    FakeWorld world;
    set.postLink(world);
    const AiSpawner& s = set.spawners()[0];
    require(world.held("a") && world.held("b") && world.disabled("a"), "StartSpawned 0 holds every slot at the post-link");
    require(s.maxSpawn == 2 && s.spawned == 0 && s.heldMask == 3, "MaxSpawn -1 becomes the slot count");
    for (int i = 0; i < 5; ++i) set.update(world);
    require(world.held("a"), "an idle spawner spawns nothing");
    require(set.message("spawner", runtime::kAiSpawnerMessageOn, world), "message 255 reaches the spawner");
    require(s.requested == AiSpawnerState::Active && s.state == AiSpawnerState::Idle, "message 255 requests state 1");
    set.update(world);
    require(s.state == AiSpawnerState::Active && world.held("a"), "update 1: the state change to 1 only");
    set.update(world);
    require(!world.held("a") && world.held("b") && s.spawned == 1 && s.state == AiSpawnerState::Spawn, "update 2: slot 0 spawns");
    require(world.log[world.log.size() - 4] == "release a at 7", "SpawnFromSpawner puts the character at the spawner's node");
    require(world.log[world.log.size() - 3] == "start state 0 a" && world.log[world.log.size() - 2] == "AISpawned spawner",
            "0x268978: the StartState spawn word +0xc runs after the release and before the AISpawned events");
    set.update(world);
    require(s.state == AiSpawnerState::Active, "update 3: back to state 1 (WaitForDead)");
    set.update(world);
    require(!world.held("b") && s.spawned == 2, "update 4: slot 1 spawns (no stagger)");
    for (int i = 0; i < 5; ++i) set.update(world);
    require(world.count("fire") == 0, "no fire while a character is out");
    world.heldSet.insert("a");
    set.update(world);
    require(world.count("fire") == 0 && s.lastHeld == 0 && (s.heldMask & 1), "one character done");
    world.heldSet.insert("b");
    set.update(world);
    require(world.count("fire") == 1 && s.requested == AiSpawnerState::Idle, "WaitForDead: the reference fires when every slot is held again");
    for (int i = 0; i < 3; ++i) set.update(world);
    require(world.count("fire") == 1, "the reference fires once (kind no longer idle)");
    require(world.count("AISpawned ") == 4, "AISpawned for the spawner and the character per spawn");
}

void withoutWaitForDeadFiresAfterTheLastSpawn() {
    AiSpawnerSet set;
    AiSpawner s = spawner({"a", "b"});
    s.params.waitForDead = false;
    s.params.staggerFrames = 2;
    set.add(s);
    FakeWorld world;
    set.postLink(world);
    set.message("spawner", runtime::kAiSpawnerMessageOn, world);
    set.update(world);
    set.update(world);
    require(!world.held("a") && set.spawners()[0].stagger == 2, "slot 0 spawns and the stagger starts");
    set.update(world);
    set.update(world);
    require(world.held("b"), "stagger 2 then 1: slot 1 waits");
    set.update(world);
    require(world.held("b"), "stagger 0 in this update's loop is read before the decrement");
    set.update(world);
    require(!world.held("b"), "slot 1 spawns");
    set.update(world);
    require(world.count("fire") == 1, "the reference fires in state 2 once MaxSpawn are out");
}

void secondTriggerTurnsItOff() {
    AiSpawnerSet set;
    set.add(spawner({"a"}));
    FakeWorld world;
    set.postLink(world);
    set.message("spawner", runtime::kAiSpawnerMessageOn, world);
    set.message("spawner", runtime::kAiSpawnerMessageOn, world);
    require(set.spawners()[0].requested == AiSpawnerState::Idle, "255 on a requested state other than 0 calls 0x1b4f28: state 0");
    set.message("spawner", runtime::kAiSpawnerMessageOn, world);
    set.message("spawner", runtime::kAiSpawnerMessageOff, world);
    require(set.spawners()[0].requested == AiSpawnerState::Idle, "254 sets state 0");
    require(!set.message("other", runtime::kAiSpawnerMessageOn, world), "not a spawner");
}

void releaseRetriesUntilTheCharacterComesOut() {
    AiSpawnerSet set;
    set.add(spawner({"a"}));
    FakeWorld world;
    world.releaseWorks = false;
    set.postLink(world);
    set.message("spawner", runtime::kAiSpawnerMessageOn, world);
    set.update(world);
    set.update(world);
    set.update(world);
    require(set.spawners()[0].state == AiSpawnerState::Active && set.spawners()[0].spawned == 0 && world.count("release") == 2,
            "0x268978 returns before +2 is written when the character did not come out, and tries again");
    world.releaseWorks = true;
    set.update(world);
    require(set.spawners()[0].spawned == 1 && set.spawners()[0].state == AiSpawnerState::Spawn, "then the spawn completes");
}

void startSpawnedCountsAndRespawnCountdown() {
    AiSpawnerSet set;
    AiSpawner s = spawner({"a", "b", "c"});
    s.params.startSpawned = true;
    s.params.startActive = true;
    s.state = s.requested = AiSpawnerState::Active;
    s.params.maxSpawn = 2;
    s.maxSpawn = 2;
    s.params.respawnFrames = 9;
    s.params.revealObject = "key";
    s.revealPending = true;
    set.add(s);
    FakeWorld world;
    set.postLink(world);
    const AiSpawner& out = set.spawners()[0];
    require(!world.held("a") && !world.held("b") && !world.held("c"), "StartSpawned 1 holds nothing");
    require(out.spawned == 2 && out.slots[0].countdown == 9 && out.slots[1].countdown == 9 && out.slots[2].countdown == 0,
            "only MaxSpawn slots are counted; the third is neither held nor counted");
    require(world.disabled("key"), "the post-link disables the reveal object");
    world.heldSet = {"a", "b", "c"};
    set.update(world);
    require(world.count("reveal key at c") == 1 && world.count("fire") == 1, "all held: reveal at the last held slot, then the WaitForDead fire");
}

void maxOnScreenHoldsTheRest() {
    AiSpawnerSet set;
    AiSpawner s = spawner({"a", "b", "c"});
    s.params.startSpawned = true;
    s.params.maxOnScreen = 1;
    set.add(s);
    FakeWorld world;
    set.postLink(world);
    const AiSpawner& out = set.spawners()[0];
    require(!world.held("a") && world.held("b") && world.held("c"), "slots from MaxOnScreen on are held");
    require(out.onScreen == 1 && out.spawned == 1, "on screen 3 - 2 held");
}

void playerInsideBound() {
    AiSpawnerSet set;
    AiSpawner s = spawner({"a"});
    s.params.playerInsideBound = "bound";
    set.add(s);
    FakeWorld world;
    world.inside = false;
    set.postLink(world);
    set.message("spawner", runtime::kAiSpawnerMessageOn, world);
    set.update(world);
    require(set.spawners()[0].state == AiSpawnerState::PlayerOutside, "player outside the bound: state 3");
    world.inside = true;
    set.update(world);
    require(set.spawners()[0].state == AiSpawnerState::Active, "player inside again: state 1");
}

void resetAndCountHeld() {
    AiSpawnerSet set;
    AiSpawner s = spawner({"a", "b"});
    s.params.startSpawned = true;
    set.add(s);
    FakeWorld world;
    world.idle = false;
    set.postLink(world);
    world.heldSet = {"a"};
    set.message("spawner", runtime::kAiSpawnerMessageReset, world);
    require(world.count("unfire") == 1 && world.count("restore a") == 1 && set.spawners()[0].spawned == 2, "message 8 restores StartSpawned slots");
    world.heldSet = {"b"};
    set.message("spawner", runtime::kAiSpawnerMessageCountHeld, world);
    require(set.spawners()[0].maxSpawn == 3 && set.spawners()[0].requested == AiSpawnerState::Active, "message 33: MaxSpawn = spawned + held");
}

}

int main() {
    try {
        heldUntilTriggeredAndChainOnAllDone();
        withoutWaitForDeadFiresAfterTheLastSpawn();
        secondTriggerTurnsItOff();
        releaseRetriesUntilTheCharacterComesOut();
        startSpawnedCountsAndRespawnCountdown();
        maxOnScreenHoldsTheRest();
        playerInsideBound();
        resetAndCountHeld();
    } catch (const std::exception& error) {
        std::cerr << "ai-spawner-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-spawner-test: ok\n";
    return 0;
}

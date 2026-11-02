// The level object class `particles` (kind 1) and FallArrestBoundSystem on made-up objects, bounds and characters. The rules were read by
// 20261102-mission14b-C (2026-11-02) with REA (Ghidra 12.1.2 decompile of 0x1a7f30, 0x1b59f0, 0x1a8394, 0x307890, 0x307980, 0x22f650, 0x22c28c) and
// llvm-objdump of game.arm.elf (docs/re/particles-objects.md, docs/re/scripts-missions-1-4.md); every number below is made up and checked by hand against
// those rules. No game data needed.
#include "runtime/fall_arrest_bounds.hpp"
#include "runtime/particle_object.hpp"

#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelParam floatParam(const char* name, float value, std::uint16_t flags = 6) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = flags;
    param.type = 0;
    std::memcpy(&param.value, &value, 4);
    return param;
}

assets::LevelParam intParam(const char* name, std::uint32_t value, std::uint16_t type = 7, std::uint16_t flags = 6) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = flags;
    param.type = type;
    param.value = value;
    return param;
}

assets::LevelParam positionParam(float x, float y, float z) {
    assets::LevelParam param;
    param.hash = assets::nameHash("initialposition");
    param.flags = 6;
    param.type = 2;
    const float values[3] = {x, y, z};
    assets::Bytes block(12);
    std::memcpy(block.data(), values, 12);
    param.blocks.push_back(block);
    return param;
}

assets::Level particlesClass() {
    assets::Level cls;
    cls.classParams = {intParam("ObjectType", 1, 7, 5), intParam("ParticleDef", 0), floatParam("ReTriggerTime", 0), floatParam("TimeRandomiser", 0),
                       intParam("Instance", 0, 5), positionParam(0, 0, 0)};
    return cls;
}

assets::LevelObject particleObject(const std::string& name, std::uint32_t definition, float reTrigger, float x) {
    assets::LevelObject object;
    object.className = "particles";
    object.instance = name;
    object.params = {intParam("ParticleDef", definition), floatParam("ReTriggerTime", reTrigger), positionParam(x, 2, 3)};
    return object;
}

class TestWorld : public runtime::ParticleObjectWorld {
public:
    std::uint32_t start(std::int32_t definition, const std::array<float, 3>& position) override {
        if (refuse) return 0;
        started.push_back({definition, position});
        running_[++next_] = true;
        return next_;
    }
    void stop(std::uint32_t emitter, float fadeSeconds) override {
        stopped.push_back({emitter, fadeSeconds});
        running_[emitter] = false;
    }
    float random() override { return 0.5f; }
    bool running(std::uint32_t emitter) const override {
        const auto found = running_.find(emitter);
        return found != running_.end() && found->second;
    }
    void end(std::uint32_t emitter) { running_[emitter] = false; }

    bool refuse = false;
    std::vector<std::pair<std::int32_t, std::array<float, 3>>> started;
    std::vector<std::pair<std::uint32_t, float>> stopped;

private:
    std::uint32_t next_ = 0;
    std::map<std::uint32_t, bool> running_;
};

runtime::ParticleObjectDefinition definitionOf(std::int32_t index) {
    if (index == 5) return {0.5f, 0.25f};
    if (index == 9) return {100.0f, 1.0f};
    return {};
}

void particleObjectTrigger() {
    const assets::Level cls = particlesClass();
    assets::Level level;
    level.objects = {particleObject("Kick", 5, 0, 1), particleObject("Periodic", 5, 2.0f, 4), particleObject("Ambient", 9, 0, 7)};
    assets::LevelObject prop;
    prop.className = "props/prop_prop";
    prop.instance = "Prop";
    level.objects.push_back(prop);
    const assets::Level empty;
    const assets::ClassLookup classOf = [&](const std::string& name) -> const assets::Level* { return name == "particles" ? &cls : &empty; };
    TestWorld world;
    runtime::ParticleObjects objects;
    objects.load(level, classOf, definitionOf, world);
    require(objects.objects().size() == 3, "the three kind 1 objects, not the prop");
    const runtime::ParticleObject* kick = objects.find("kick");
    require(kick && kick->params.definition == 5 && kick->timer == 0.5f && !kick->endless, "ParticleDef, the timer at the duration (0x1a7f30)");
    require(kick->position == std::array<float, 3>({1, 2, 3}), "initialposition");
    require(objects.find("ambient")->endless, "a duration above 98 sets flag 4");

    objects.message("kick", runtime::kParticleObjectMessageTrigger, world);
    require(world.started.size() == 1 && world.started[0].first == 5 && world.started[0].second == std::array<float, 3>({1, 2, 3}),
            "0xff starts the definition at the node (0x1b5b40)");
    objects.message("kick", runtime::kParticleObjectMessageTrigger, world);
    require(world.started.size() == 1, "0xff with a running emitter starts no second one (0x23619c(e, 0))");
    objects.message("periodic", runtime::kParticleObjectMessageTrigger, world);
    require(world.started.size() == 1, "ReTriggerTime above 0 ignores the trigger messages (0x1b5a34)");
    std::vector<runtime::ParticleObjectEvent> events = objects.takeEvents();
    require(events.size() == 1 && events[0].kind == runtime::ParticleObjectEventKind::Started && events[0].instance == "kick", "one start event");

    objects.update(0.25f, world);
    require(objects.find("kick")->enabled && objects.find("kick")->timer == 0.25f, "0x1a8394: the timer runs down by dt");
    objects.update(0.25f, world);
    require(objects.find("kick")->enabled && objects.find("kick")->timer == 0.0f, "a timer of 0 is not below 0 yet");
    objects.update(0.25f, world);
    require(objects.find("kick")->enabled, "the test comes before the countdown");
    objects.update(0.25f, world);
    require(!objects.find("kick")->enabled && world.stopped.empty(), "below 0 the object is disabled (0x2428c0), its emitter runs on");
    events = objects.takeEvents();
    require(events.size() == 1 && events[0].kind == runtime::ParticleObjectEventKind::Disabled, "the disable event");

    world.end(1);
    objects.message("kick", runtime::kParticleObjectMessageTrigger, world);
    require(world.started.size() == 2 && objects.find("kick")->enabled && objects.find("kick")->timer == 0.5f,
            "after the emitter ended a trigger starts a new one, enables the object and resets the timer");
    objects.message("kick", runtime::kParticleObjectMessageUntrigger, world);
    require(world.stopped.size() == 1 && world.stopped[0] == std::pair<std::uint32_t, float>(2, 0.25f),
            "0xfe ends the emitter over the definition's +0x80 (0x1b5cb4)");
    require(!objects.find("kick")->enabled && objects.find("kick")->emitter == 0, "and disables the object");

    world.refuse = true;
    objects.message("ambient", runtime::kParticleObjectMessageTrigger, world);
    events = objects.takeEvents();
    require(events.back().kind == runtime::ParticleObjectEventKind::Refused && objects.find("ambient")->emitter == 0, "a refused start keeps +0x20 at 0");
}

assets::LevelBound box(const std::string& name, float cx, float half, std::uint8_t node = 0xff) {
    assets::LevelBound bound;
    bound.name = name;
    bound.centre[0] = cx;
    bound.half[0] = bound.half[1] = bound.half[2] = half;
    bound.shape = 3;
    bound.node = node;
    return bound;
}

void fallArrest() {
    assets::Level level;
    level.bounds = {box("fallarrestbound01", 0, 1), box("1-4_FallArrestBoundsON01", 0, 50), box("FallArrest", 10, 1, 0), box("FallArres", 20, 1)};
    level.boundMatrices = {{0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1}};
    const std::vector<std::size_t> bounds = runtime::fallArrestBounds(level);
    require(bounds == std::vector<std::size_t>({0, 2}), "the prefix of ten characters, case ignored (0x22c28c)");
    std::vector<std::uint8_t> enabled = {1, 1, 1, 1};
    std::vector<runtime::FallArrestCharacter> engaged(7);
    for (std::size_t i = 0; i < engaged.size(); ++i) {
        engaged[i].name = "c" + std::to_string(i);
        engaged[i].onGround = false;
        engaged[i].position = {10.5f, 0, 0};
    }
    engaged[0].onGround = true;
    engaged[1].arrestMark = 1;
    engaged[2].enabled = false;
    engaged[4].position = {0.5f, 0, 0};
    engaged[5].position = {5.0f, 0, 0};
    engaged[6].position = {0.5f, 0, 0};
    std::vector<runtime::FallArrestHit> hits = runtime::fallArrestUpdate(level, bounds, enabled, engaged);
    require(hits.size() == 2 && hits[0].character == "c3" && hits[0].bound == 2 && hits[1].character == "c4" && hits[1].bound == 0,
            "in the air, enabled, unmarked, inside: the first matching bound; the node matrix is not applied (0x22f650 with no scene); six slots");
    require(engaged[3].arrestMark == 2 && engaged[4].arrestMark == 2 && engaged[5].arrestMark == 0 && engaged[6].arrestMark == 0, "the mark 2 (bits 6..7 = 0x80)");
    hits = runtime::fallArrestUpdate(level, bounds, enabled, engaged);
    require(hits.empty(), "a marked character is not tested again");
    engaged[5].position = {0.5f, 0, 0};
    enabled[0] = 0;
    hits = runtime::fallArrestUpdate(level, bounds, enabled, engaged);
    require(hits.empty(), "a bound switched off by EnableBound (+0xa) catches nobody");
}

}

int main() {
    try {
        particleObjectTrigger();
        fallArrest();
    } catch (const std::exception& error) {
        std::cerr << "particle-object-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "particle-object-test: ok\n";
    return 0;
}

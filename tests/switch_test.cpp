// The switch base (src/runtime/switch_base.cpp) and the lever (src/runtime/switch_set.cpp) on a synthetic level. Every expected value comes from the code
// read by 20261025-garage-A on 2026-10-25 (llvm-objdump of game.arm.elf): the stable update 0x21435c, the state change 0x22c6d0, the base handler
// 0x23aac0, the loader 0x23e84c, the post-link 0x242494, the lever handler 0x17f6f4 and update 0x175338 (docs/re/switches.md).
#include "runtime/switch_set.hpp"
#include "assets/object_classes.hpp"

#include <cctype>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;
using chase::assets::Level;
using chase::assets::LevelObject;
using chase::assets::LevelParam;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

LevelParam floatParam(const char* name, float value, std::uint16_t flags = 2) {
    LevelParam param;
    param.hash = chase::assets::nameHash(name);
    param.flags = flags;
    param.type = 0;
    std::memcpy(&param.value, &value, 4);
    return param;
}

LevelParam wordParam(const char* name, std::uint32_t value, std::uint16_t flags = 2) {
    LevelParam param;
    param.hash = chase::assets::nameHash(name);
    param.flags = flags;
    param.type = 7;
    param.value = value;
    return param;
}

LevelParam vectorParam(const char* name, float x, float y, float z) {
    LevelParam param;
    param.hash = chase::assets::nameHash(name);
    param.flags = 2;
    param.type = 2;
    chase::assets::Bytes block(12);
    const float values[3] = {x, y, z};
    std::memcpy(block.data(), values, 12);
    param.blocks.push_back(block);
    return param;
}

LevelParam referenceParam(const std::string& target) {
    LevelParam param;
    param.hash = chase::assets::nameHash("TriggerObject");
    param.flags = 2;
    param.type = 4;
    chase::assets::Bytes block(8, 0);
    const std::uint32_t hash = chase::assets::nameHash(target);
    std::memcpy(block.data() + 4, &hash, 4);
    param.blocks.push_back(block);
    return param;
}

struct World {
    Level level;
    Level leverClass;
    Level lightClass;
    Level targetClass;
};

void addObject(Level& level, const std::string& className, const std::string& name, float x, const std::string& target, std::vector<LevelParam> extra = {}) {
    LevelObject object;
    object.className = className;
    object.instance = name;
    object.params = {vectorParam("initialposition", x, 0, 0), vectorParam("initialorientation", 0, 0, 0)};
    if (!target.empty()) object.params.push_back(referenceParam(target));
    for (LevelParam& param : extra) object.params.push_back(param);
    level.objects.push_back(object);
}

void addTrigger(Level& level, const std::string& event, const std::string& object, const std::string& script, const std::string& variable) {
    chase::assets::LevelTrigger trigger;
    trigger.nameHash = chase::assets::nameHash(event + object);
    trigger.event = event;
    trigger.objectName = object;
    trigger.scriptName = script;
    level.triggers.push_back(trigger);
    chase::assets::LevelScript made;
    made.nameHash = chase::assets::nameHash(script);
    made.calls = {{"=", {variable, "1"}}};
    level.scripts.push_back(made);
}

World makeWorld() {
    World world;
    world.leverClass.classParams = {wordParam("ObjectType", 58, 5), floatParam("ResetTime", 1.0f, 6), wordParam("TriggerBeforeAnim", 1, 6),
                                    wordParam("AlwaysReset", 0, 6), wordParam("StartOn", 0, 6), wordParam("ATTR_SFX_ON", 0, 6)};
    world.leverClass.defaults = {wordParam("TriggerObject", 0), wordParam("LeverAbility", 0), wordParam("StartOn", 0), wordParam("AlwaysReset", 0),
                                 floatParam("ResetTime", 1.0f), wordParam("ATTR_SFX_ON", 0), wordParam("Strong", 0)};
    chase::assets::LevelBound bound;
    bound.name = "UseBound";
    bound.centre[1] = 0.5f;
    bound.half[0] = bound.half[1] = bound.half[2] = 1.0f;
    world.leverClass.bounds.push_back(bound);
    world.lightClass.classParams = {wordParam("ObjectType", 38, 5)};
    world.lightClass.defaults = {wordParam("TriggerObject", 0)};
    world.targetClass.classParams = {wordParam("ObjectType", 0, 5)};
    addObject(world.level, "test/lever", "Key", 0, "Door");
    addObject(world.level, "test/lever", "JoinA", 10, "Gate");
    addObject(world.level, "test/lever", "JoinB", 20, "Gate");
    addObject(world.level, "test/lever", "Timed", 30, "Lamp", {wordParam("AlwaysReset", 1), wordParam("ATTR_SFX_ON", 77)});
    addObject(world.level, "test/lever", "Started", 40, "Bell", {wordParam("StartOn", 1)});
    addObject(world.level, "test/lever", "Strongbox", 50, "Vault", {wordParam("Strong", 1)});
    addObject(world.level, "test/light", "Light", 60, "Lamp2");
    addObject(world.level, "test/lever", "Pull", 70, "Pump", {wordParam("TriggerBeforeAnim", 0), floatParam("ResetTime", -1.0f)});
    for (const char* name : {"Door", "Gate", "Lamp", "Bell", "Vault", "Lamp2", "Pump"}) addObject(world.level, "test/target", name, 100, "");
    addTrigger(world.level, "ObjectTriggered", "Door", "door_script", "DoorOpen");
    addTrigger(world.level, "ObjectTriggered", "Gate", "gate_script", "GateOpen");
    addTrigger(world.level, "ObjectSwitched", "JoinA", "joina_script", "JoinASwitched");
    addTrigger(world.level, "ObjectUnswitched", "JoinA", "joina_off_script", "JoinAUnswitched");
    return world;
}

// key_interact (clip 185, models/char_minifig/class_generic): 72 frames at 30 per second, event track 0x98d31b7e up at 0.3195 of the clip; read by
// 20261026-mission13-A on 2026-10-26 through chase_assets (CTest lever-use-real-data pins the same values).
const LeverClipTiming kKeyInteractClip{2.4f, 0.319495f * 2.4f};
// RPC 20261026-mission13-A-1 (recorded from save states at the key switch of mission 1-3 after placing the key and at the generator pull switch, New 3DS,
// OpenGL, dt 0.033 0.033, by the emulator servant on 2026-10-26; RPC 20261025-garage-A-1 gave the same counts at dt 0.0333333): the key switch's use bit
// (record +0x24 bit 0, hook 0x2ccc74) is set in update 31 and the player's state 76 ends in update 81, counted from the update of the A press (u0).
constexpr unsigned kOracleUseBitUpdate = 31;
// models/prop_switchpull/use.bfnanm: 24 frames at 15 per second (read by 20261026-mission13-A, 2026-10-26; CTest lever-use-real-data).
const chase::runtime::AnimationClipInfo kPullUseAnimation{0, 24, 15, false};
constexpr float kOracleDt = 0.033f;

struct Run {
    World world = makeWorld();
    chase::assets::PlacedObjectState state;
    ReferenceTriggers references;
    SwitchSet switches;
    ScriptRunner runner{world.level, state};

    Run() {
        const chase::assets::ClassLookup classOf = [this](const std::string& name) -> const Level* {
            if (name == "test/lever") return &world.leverClass;
            if (name == "test/light") return &world.lightClass;
            if (name == "test/target") return &world.targetClass;
            return nullptr;
        };
        for (const LevelObject& object : world.level.objects) {
            const Level* cls = classOf(object.className);
            const std::uint8_t kind = chase::assets::objectKind(object, *cls);
            if (kind != 58 && kind != 38) continue;
            std::string target;
            for (const LevelParam& param : object.params)
                if (param.hash == chase::assets::nameHash("TriggerObject"))
                    for (const LevelObject& other : world.level.objects)
                        if (chase::assets::nameHash(other.instance) == chase::assets::objectReferenceHash(param)) target = other.instance;
            for (char& c : target) c = char(std::tolower(static_cast<unsigned char>(c)));
            std::string owner = object.instance;
            for (char& c : owner) c = char(std::tolower(static_cast<unsigned char>(c)));
            references.add(owner, target);
        }
        switches.load(world.level, classOf, [](const LevelObject& object, const Level&) { return object.instance == "Pull"; });
        switches.link(references);
        switches.setUseAnimation(*switches.find("pull"), kPullUseAnimation);
        for (std::size_t i = 0; i < switches.objects().size(); ++i) switches.setLeverUse(i, kKeyInteractClip, std::nullopt);
    }
    std::vector<SwitchEvent> step() {
        std::vector<SwitchEvent> events = switches.update(state, kOracleDt, references, runner);
        const float far[3] = {1000, 1000, 1000};
        runner.checkTriggers(far);
        runner.update(kOracleDt);
        return events;
    }
    SwitchState stateOf(const std::string& name) const { return switches.objects()[*switches.find(name)].record.state; }
    bool use(float x, bool able = true) {
        SwitchUseInput input;
        input.player[0] = x;
        input.actionPressed = true;
        input.playerFree = true;
        input.playerHasAbility = [able](int ability) { return able || ability != 7; };
        const std::optional<SwitchUse> taken = switches.use(state, input, references);
        return taken && taken->answer == 1;
    }
};

bool has(const std::vector<SwitchEvent>& events, SwitchEffectKind kind) {
    for (const SwitchEvent& event : events)
        if (event.effect.kind == kind) return true;
    return false;
}

void singleReferenceStaysOn() {
    Run run;
    require(run.switches.objects().size() == 8, "seven levers and one light are loaded");
    const std::size_t key = *run.switches.find("key");
    require(run.switches.markerShown(key, run.references), "a usable lever in state 0 answers the marker's query (0x172254)");
    require(run.use(0.5f), "A in the zone of a lever in state 0 takes the press (message 3 returns 1)");
    require(run.switches.playerUsing(), "the player is busy with the switch");
    require(run.switches.markerShown(key, run.references), "during USE LERP the lever is not yet the entity the player uses");
    for (unsigned i = 1; i < kOracleUseBitUpdate; ++i) {
        run.step();
        if (i == 8) require(!run.switches.markerShown(key, run.references), "from state 76 the marker skips the lever the player uses (ctrl+0x130)");
    }
    require(!run.switches.objects()[*run.switches.find("key")].record.useRequested, "no use bit before update 31 (RPC 20261025-garage-A-1)");
    run.step();
    require(run.stateOf("key") == SwitchState::Off && run.switches.objects()[*run.switches.find("key")].record.useRequested,
            "after update 31 the use bit (+0x24 bit 0) is set, the state is still 0");
    run.step();
    require(run.stateOf("key") == SwitchState::Used, "0x21435c state 0 with the request: target 1, and 0x22c6d0 takes it in the same update");
    const std::vector<SwitchEvent> second = run.step();
    require(run.stateOf("key") == SwitchState::Held, "TriggerBeforeAnim: target 2; the fired join sends 0xff back and 0x23aac0 makes it 3 (AlwaysReset clear)");
    require(has(second, SwitchEffectKind::LightsOn) && has(second, SwitchEffectKind::StudDrop), "0x21936c(1) and the StudValue drop 0x22ca88");
    require(run.references.kind(*run.references.findOwner("key")) == ReferenceKind::Fired, "the record +0xc is Fired");
    run.step();
    require(run.runner.variable("DoorOpen") == 1, "the target's ObjectTriggered starts its script");
    for (int i = 0; i < 90; ++i) run.step();
    require(run.stateOf("key") == SwitchState::Held, "state 3 has no timer: the switch stays on");
    require(!run.switches.playerUsing(), "the player's use ended after update 81");
    require(!run.use(0.5f), "a used lever no longer takes the press (zone bit 4 cleared in states 1 and 3)");
}

void trigger(Run& run, const std::string& name) {
    std::vector<SwitchEvent> events;
    run.switches.message(name, kSwitchMessageOn, run.references, run.runner, events);
}

void joinWaitsAndResets() {
    Run run;
    trigger(run, "joina");
    run.step();
    run.step();
    require(run.stateOf("joina") == SwitchState::On, "another reference is idle: message 10 only, the switch stays in state 2");
    run.step();
    require(run.runner.variable("JoinASwitched") == 1 && run.runner.variable("GateOpen") == 0, "ObjectSwitched on the owner, the gate not yet");
    unsigned onUpdates = 1;
    while (run.stateOf("joina") == SwitchState::On && onUpdates < 100) {
        run.step();
        ++onUpdates;
    }
    require(onUpdates >= 30 && onUpdates <= 31, "ResetTime 1 s counts down by dt (1/30) in 0x22c6d0: about 30 updates in state 2");
    require(run.stateOf("joina") == SwitchState::SwitchingOff, "0x21435c state 2 with the timer at 0: target 5, the record is deactivated");
    run.step();
    require(run.stateOf("joina") == SwitchState::Off, "no use animation: state 5 goes back to 0");
    run.step();
    require(run.runner.variable("JoinAUnswitched") == 1, "ObjectUnswitched on the owner");
    require(run.switches.objects()[*run.switches.find("joina")].lever->usable, "state 5 made the zone usable again (bit 6 clear)");
}

void joinFiresWhenBothAreOn() {
    Run run;
    trigger(run, "joina");
    run.step();
    run.step();
    trigger(run, "joinb");
    run.step();
    run.step();
    require(run.stateOf("joinb") == SwitchState::Held, "the last reference fires: 0xff to both owners; the firing switch goes to 3 in the same change");
    require(run.stateOf("joina") == SwitchState::On && run.switches.objects()[*run.switches.find("joina")].record.target == SwitchState::Held,
            "the other owner's target is 3, its state follows in its next update");
    run.step();
    require(run.stateOf("joina") == SwitchState::Held, "both are on for good");
    require(run.runner.variable("GateOpen") == 1, "the gate fires once both are on");
}

void alwaysResetIsTimed() {
    Run run;
    trigger(run, "timed");
    run.step();
    const std::vector<SwitchEvent> on = run.step();
    require(run.stateOf("timed") == SwitchState::Resetting, "AlwaysReset: message 0xff in state 2 gives target 4 with the timer reset");
    bool sound = false;
    for (const SwitchEvent& event : on) sound |= event.effect.kind == SwitchEffectKind::SoundOn && event.effect.sound == 77;
    require(sound, "ATTR_SFX_ON is played on target 2");
    for (int i = 0; i < 31; ++i) run.step();
    require(run.stateOf("timed") == SwitchState::SwitchingOff || run.stateOf("timed") == SwitchState::Off, "state 4 ends with the timer");
}

void useAnimationTimesStateOne() {
    Run run;
    trigger(run, "pull");
    run.step();
    require(run.stateOf("pull") == SwitchState::Used, "message 0xff in state 0: state 1 and the use animation starts (0x22c6d0, NumberOfTurns 1)");
    unsigned updates = 0;
    while (run.stateOf("pull") == SwitchState::Used && updates < 100) {
        run.step();
        ++updates;
    }
    require(updates == 49, "without TriggerBeforeAnim state 1 waits for 0x245f84 == 6: 24 frames at 15 per second, 49 steps of 0.033 on the switches' "
                           "clock, got " + std::to_string(updates));
    require(run.stateOf("pull") == SwitchState::Held, "ResetTime -1: state 3");
}

void pullSwitchAgainstOracle() {
    Run run;
    const std::size_t pull = *run.switches.find("pull");
    run.switches.setLeverUse(pull, LeverClipTiming{24.0f / 15.0f, 0.00284608f * 23.0f / 15.0f}, std::nullopt);
    require(run.use(70.5f), "A at the pull switch takes the press");
    for (unsigned u = 1; u <= 58; ++u) {
        run.step();
        const SwitchObject& sw = run.switches.objects()[pull];
        if (u < 8) require(!sw.record.useRequested, "no use bit before update 8");
        if (u == 8) require(sw.record.useRequested && sw.record.state == SwitchState::Off, "the hook sets the use bit in update 8 (A-1: trap 0x2ccc74 +0x10 = 1.0, record +0x24 = 5 read before 9)");
        if (u == 9) require(sw.record.state == SwitchState::Used, "state 1 in update 9 (A-1: record +2 / +4 = 1 read before update 10)");
        if (u == 56) require(run.switches.playerUsing(), "the player is in state 76 until update 56");
        if (u == 57) require(!run.switches.playerUsing(), "state 1 of the player is entered in update 57 (A-1: trap 0x21941c r1 0x118, state read as 1 before 58)");
        if (u == 57) require(sw.record.state == SwitchState::Used, "the switch is still in state 1 in update 57");
        if (u == 58) require(sw.record.state == SwitchState::Held, "state 3 in update 58, 49 updates after state 1 (A-1: record +2 / +4 = 3 read before update 59)");
    }
}

void toggleUseableWritesTheZone() {
    Run run;
    const std::size_t key = *run.switches.find("key");
    run.switches.setZoneUsable(key, false);
    require(!run.use(0.5f) && !run.switches.markerShown(key, run.references), "0x236624(lever, 0): zone bit 4 cleared, message 3 answers 0");
    require(run.switches.objects()[key].lever->zoneBit6, "and bit 6 set: switching off (state 5) leaves the zone unusable");
    run.switches.setZoneUsable(key, true);
    require(run.use(0.5f), "0x236624(lever, 1) makes it usable again");
}

void startOnRefusesTheUse() {
    Run run;
    require(run.references.kind(*run.references.findOwner("started")) == ReferenceKind::Registered, "StartOn: 0x242494 makes the record Registered");
    require(!run.use(40.5f), "message 3 refuses while the record is Registered (0x17f7b4)");
}

void strongNeedsAbilitySeven() {
    Run run;
    require(!run.use(50.5f, false), "a Strong lever needs ability 7 (0x1f8b44)");
    require(run.use(50.5f, true), "with ability 7 it takes the press");
}

void messagesOfTheBaseHandler() {
    Run run;
    std::vector<SwitchEvent> events;
    require(run.switches.message("light", kSwitchMessageOn, run.references, run.runner, events), "the light is a switch");
    run.step();
    require(run.stateOf("light") == SwitchState::Used, "message 0xff in state 0 is a use request");
    run.step();
    require(run.stateOf("light") == SwitchState::Held, "and the single reference keeps it on");
    run.switches.message("light", kSwitchMessageOff, run.references, run.runner, events);
    run.step();
    require(run.stateOf("light") == SwitchState::SwitchingOff, "message 0xfe: target 5");
    run.step();
    require(run.stateOf("light") == SwitchState::Off, "and back to 0");
    run.switches.message("light", kSwitchMessageOnAtOnce, run.references, run.runner, events);
    require(run.stateOf("light") == SwitchState::Held && run.references.kind(*run.references.findOwner("light")) == ReferenceKind::Registered,
            "message 54: state and target 3, the record Registered, no event");
    run.switches.message("light", kSwitchMessageReset, run.references, run.runner, events);
    run.step();
    require(run.stateOf("light") == SwitchState::SwitchingOff, "message 8 with a Registered record: target 5");
    require(!run.switches.message("door", kSwitchMessageOn, run.references, run.runner, events), "a non-switch is not handled");
}

}

int main() {
    try {
        singleReferenceStaysOn();
        joinWaitsAndResets();
        joinFiresWhenBothAreOn();
        alwaysResetIsTimed();
        useAnimationTimesStateOne();
        pullSwitchAgainstOracle();
        toggleUseableWritesTheZone();
        startOnRefusesTheUse();
        strongNeedsAbilitySeven();
        messagesOfTheBaseHandler();
    } catch (const std::exception& error) {
        std::cerr << "switch-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "switch-test: ok\n";
    return 0;
}

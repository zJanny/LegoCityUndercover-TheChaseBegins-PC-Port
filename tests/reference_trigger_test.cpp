// Reference-trigger step 0x23211c (src/runtime/reference_trigger.*, reference_trigger_apply.*). Values: docs/re/buildit.md (oracle ctrace of
// 0x23cee8 over the build of 1-1_BI_DonutVanParts, save state in front of the donut van parts build-it of mission 1-1: the call 0x2321dc with the build-it as object and instigator, then the
// call 0x23243c with the target as object and the build-it as instigator) and the disassembly of 0x23211c, 0x244fc8, 0x242cf0 and the event
// registration 0x16bac4 (llvm-objdump, 20261025-mission13-B; docs/re/script-triggers.md entry of that round). The deactivate mode, message 10,
// message 0xff to the owners and the nav node call are read from the disassembly only.
#include "runtime/reference_trigger.hpp"
#include "runtime/reference_trigger_apply.hpp"
#include "runtime/script_runner.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool isEvent(const ReferenceEffect& effect, ReferenceEvent event, const std::string& object, const std::string& instigator) {
    return effect.kind == ReferenceEffectKind::Event && effect.event == event && effect.object == object && effect.instigator == instigator;
}

bool isMessage(const ReferenceEffect& effect, const std::string& object, std::uint8_t message, const std::string& instigator) {
    return effect.kind == ReferenceEffectKind::Message && effect.object == object && effect.message == message && effect.instigator == instigator;
}

void singleReferenceFiresItsTarget() {
    ReferenceTriggers refs;
    const std::size_t only = refs.add("build", "van_fixed");
    const ReferenceActivation first = refs.activate(only);
    require(first.ran && first.fired && first.target == "van_fixed" && first.owners.size() == 1, "a single reference fires its target");
    require(refs.kind(only) == ReferenceKind::Fired, "the reference becomes Fired");
    require(refs.activate(only).fired, "a Fired reference activates again (only kind 1 returns early)");
}

void activateOrderMatchesTheOriginal() {
    ReferenceTriggers refs;
    const std::size_t only = refs.add("build", "van_fixed");
    const ReferenceActivation result = refs.activate(only);
    require(result.effects.size() == 6, "six effects without a nav node");
    require(isEvent(result.effects[0], ReferenceEvent::ObjectSwitched, "build", "build"), "0x2321dc: *0x3775d8 on the owner, the owner as instigator (oracle)");
    require(isMessage(result.effects[1], "van_fixed", kReferenceMessageJoinWaiting, "build"), "0x232258: message 10 to the target");
    require(isMessage(result.effects[2], "build", kReferenceMessageOn, "build"), "0x2323f0: message 0xff to each owner, the owner as instigator");
    require(result.effects[3].kind == ReferenceEffectKind::Enable && result.effects[3].object == "van_fixed", "0x23240c: 0x242cf0 on the target");
    require(isMessage(result.effects[4], "van_fixed", kReferenceMessageOn, "build"), "0x23241c: message 0xff to the target");
    require(isEvent(result.effects[5], ReferenceEvent::ObjectTriggered, "van_fixed", "build"), "0x232438: *0x3775d0 on the target, the owner as instigator (oracle)");
}

void joinWaitsForTheLastReference() {
    ReferenceTriggers refs;
    const std::size_t a = refs.add("build_a", "gate");
    const std::size_t b = refs.add("build_b", "gate");
    const ReferenceActivation first = refs.activate(a);
    require(first.ran && !first.fired && first.joinWaiting, "the target waits for the other reference (0x232234)");
    require(refs.kind(a) == ReferenceKind::Registered, "the waiting reference is Registered");
    require(first.effects.size() == 2 && isEvent(first.effects[0], ReferenceEvent::ObjectSwitched, "build_a", "build_a") &&
                isMessage(first.effects[1], "gate", kReferenceMessageJoinWaiting, "build_a"),
            "a waiting reference raises its owner event and sends message 10 (0x232370)");
    const ReferenceActivation again = refs.activate(a);
    require(!again.ran && !again.fired && !again.joinWaiting && again.effects.empty(), "a Registered reference returns at once (0x2321b4)");
    const ReferenceActivation second = refs.activate(b);
    require(second.fired && second.owners.size() == 2, "the last reference fires the target and marks both Fired");
    require(isMessage(second.effects[2], "build_a", kReferenceMessageOn, "build_a") && isMessage(second.effects[3], "build_b", kReferenceMessageOn, "build_b"),
            "message 0xff goes to every owner in list order, each as its own instigator");
}

void referencesOfOtherTargetsDoNotJoin() {
    ReferenceTriggers refs;
    const std::size_t a = refs.add("door", "gate");
    refs.add("lever", "other");
    refs.add("lamp", "");
    require(refs.activate(a).fired, "an idle reference to another target or without a target does not hold the join");
}

void unsetReferenceFiresNothing() {
    ReferenceTriggers refs;
    const std::size_t unset = refs.add("build", "");
    const ReferenceActivation result = refs.activate(unset);
    require(result.ran && !result.fired && !result.joinWaiting, "an unset reference fires nothing");
    require(refs.kind(unset) == ReferenceKind::Registered, "an unset reference is still Registered");
    require(result.effects.size() == 1 && result.effects[0].event == ReferenceEvent::ObjectSwitched, "only the owner event is raised");
}

void deactivateUndoesAFiredJoin() {
    ReferenceTriggers refs;
    const std::size_t a = refs.add("build_a", "gate");
    const std::size_t b = refs.add("build_b", "gate");
    refs.activate(a);
    refs.activate(b);
    const ReferenceActivation off = refs.deactivate(a);
    require(off.ran && off.target == "gate", "a Fired reference deactivates");
    require(refs.kind(a) == ReferenceKind::Idle && refs.kind(b) == ReferenceKind::Registered, "the other reference goes back to Registered (0x2322d0), this one to Idle (0x232368)");
    require(off.effects.size() == 5, "five effects");
    require(isMessage(off.effects[0], "build_a", kReferenceMessageOff, "build_a") && isMessage(off.effects[1], "build_b", kReferenceMessageOff, "build_b"),
            "0x2322e4: message 0xfe to every owner of the target");
    require(isMessage(off.effects[2], "gate", kReferenceMessageOff, "build_a"), "0x232308: message 0xfe to the target");
    require(isEvent(off.effects[3], ReferenceEvent::ObjectUntriggered, "gate", "build_a"), "0x232324: *0x3775d4 on the target");
    require(isEvent(off.effects[4], ReferenceEvent::ObjectUnswitched, "build_a", "build_a"), "0x23235c: *0x3775dc on the owner");
    require(refs.activate(a).joinWaiting == false && refs.kind(a) == ReferenceKind::Fired, "a is the only idle-free one again: b is Registered, so a fires");
}

void deactivateOfRegisteredAndIdle() {
    ReferenceTriggers refs;
    const std::size_t a = refs.add("build_a", "gate");
    refs.add("build_b", "gate");
    require(!refs.deactivate(a).ran, "an Idle reference returns at once (0x232270)");
    refs.activate(a);
    const ReferenceActivation off = refs.deactivate(a);
    require(off.ran && off.effects.size() == 1 && isEvent(off.effects[0], ReferenceEvent::ObjectUnswitched, "build_a", "build_a"),
            "a Registered reference only raises its owner event and goes Idle");
    require(refs.kind(a) == ReferenceKind::Idle, "Idle again");
}

void navNodeFollowsTheMode() {
    ReferenceTriggers refs;
    const std::size_t unset = refs.add("lamp", "", 7);
    ReferenceActivation on = refs.activate(unset);
    require(on.effects.size() == 2 && on.effects[1].kind == ReferenceEffectKind::NavNode && on.effects[1].navNode == 7 && on.effects[1].navNodeActive,
            "0x232448: 0x22c470(*0x37754c, NavNode, 1) closes the activation, also without a target");
    const ReferenceActivation off = refs.deactivate(unset);
    require(off.effects.size() == 2 && off.effects[0].kind == ReferenceEffectKind::NavNode && !off.effects[0].navNodeActive &&
                off.effects[1].event == ReferenceEvent::ObjectUnswitched,
            "0x232340: 0x22c470(..., 0) runs before the owner event on deactivate");
    ReferenceTriggers joined;
    const std::size_t a = joined.add("a", "gate", 3);
    joined.add("b", "gate");
    for (const ReferenceEffect& effect : joined.activate(a).effects) require(effect.kind != ReferenceEffectKind::NavNode, "a waiting join returns before the nav node call (0x232380)");
    const std::size_t c = joined.add("c", "gate2", 255);
    for (const ReferenceEffect& effect : joined.activate(c).effects) require(effect.kind != ReferenceEffectKind::NavNode, "NavNode 255 means none");
}

void ownerSearchTakesTheLastMatch() {
    ReferenceTriggers refs;
    refs.add("lever", "first");
    refs.add("other", "second");
    const std::size_t last = refs.add("lever", "third");
    require(refs.findOwner("lever") == last, "0x23211c keeps the last entry of the owner");
    require(!refs.findOwner("missing"), "no entry");
    const ReferenceActivation fired = refs.activateOwner("lever");
    require(fired.fired && fired.target == "third", "ref = 0 activates the owner's record");
    require(!refs.activateOwner("missing").ran && !refs.deactivateOwner("missing").ran, "an owner without an entry does nothing (0x232178)");
    require(refs.deactivateOwner("lever").ran, "deactivate by owner");
}

chase::assets::LevelTrigger trigger(const std::string& event, const std::string& object, const std::string& script) {
    chase::assets::LevelTrigger made;
    made.nameHash = chase::assets::nameHash(event + object);
    made.event = event;
    made.objectName = object;
    made.scriptName = script;
    return made;
}

void addScript(chase::assets::Level& level, const std::string& name, const std::string& variable) {
    chase::assets::LevelScript script;
    script.nameHash = chase::assets::nameHash(name);
    script.calls = {{"=", {variable, "1"}}};
    level.scripts.push_back(std::move(script));
}

void effectsReachTheRunner() {
    chase::assets::Level level;
    level.triggers.push_back(trigger("ObjectSwitched", "Build", "switched"));
    level.triggers.push_back(trigger("ObjectTriggered", "Gate", "triggered"));
    level.triggers.push_back(trigger("ObjectUnswitched", "Build", "unswitched"));
    level.triggers.push_back(trigger("ObjectUntriggered", "Gate", "untriggered"));
    addScript(level, "switched", "Switched");
    addScript(level, "triggered", "Triggered");
    addScript(level, "unswitched", "Unswitched");
    addScript(level, "untriggered", "Untriggered");
    chase::assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    state.disabled.insert("gate");
    ReferenceTriggers refs;
    const std::size_t build = refs.add("build", "gate", 4);
    const std::vector<ReferenceNavNodeRequest> nav = applyReferenceActivation(refs.activate(build), runner);
    require(!state.isDisabled("gate"), "the enable effect clears the disabled bit");
    const std::vector<ScriptObjectMessage> sent = runner.takeObjectMessages();
    require(sent.size() == 3 && sent[0].instance == "gate" && sent[0].message == 10 && sent[1].instance == "build" && sent[1].message == 0xff &&
                sent[2].instance == "gate" && sent[2].message == 0xff,
            "messages 10, 0xff to the owner and 0xff to the target, in that order");
    require(nav.size() == 1 && nav[0].node == 4 && nav[0].active, "the nav node request is handed back");
    const float far[3] = {0, 0, 0};
    runner.checkTriggers(far);
    runner.update(0.034f);
    require(runner.variable("Switched") == 1 && runner.variable("Triggered") == 1, "ObjectSwitched on the owner and ObjectTriggered on the target start their scripts");
    require(runner.variable("Unswitched") == 0 && runner.variable("Untriggered") == 0, "no deactivate event yet");
    applyReferenceActivation(refs.deactivate(build), runner);
    runner.checkTriggers(far);
    runner.update(0.034f);
    require(runner.variable("Unswitched") == 1 && runner.variable("Untriggered") == 1, "the deactivate events start their scripts");
}

}

int main() {
    try {
        singleReferenceFiresItsTarget();
        activateOrderMatchesTheOriginal();
        joinWaitsForTheLastReference();
        referencesOfOtherTargetsDoNotJoin();
        unsetReferenceFiresNothing();
        deactivateUndoesAFiredJoin();
        deactivateOfRegisteredAndIdle();
        navNodeFollowsTheMode();
        ownerSearchTakesTheLastMatch();
        effectsReachTheRunner();
    } catch (const std::exception& error) {
        std::cerr << "reference-trigger-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "reference-trigger-test: ok\n";
    return 0;
}

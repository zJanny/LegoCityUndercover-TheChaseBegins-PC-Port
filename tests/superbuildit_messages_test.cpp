// Messages, save record and restore of the super build-it class (src/runtime/superbuildit_messages.cpp). Provenance: the handler 0x1bbec4 (messages 6, 8,
// 0xfe, 0xfc), the save callback 0x192608, the restore callback 0x192548 and 0x26d530 (DisableSuperBuiltItSave), read with llvm-objdump of
// artifacts/game/game.arm.elf by 20261027-mission13-F on 2026-10-27 (docs/re/super-buildit.md). The record layout {hash, size, payload} is the level
// state serialiser 0x1c26dc (docs/re/save-format-state.md).
#include "runtime/superbuildit_messages.hpp"
#include "save/level_state.hpp"

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

SuperBuilditObject plate(const std::string& instance) {
    SuperBuilditObject object;
    object.instance = instance;
    object.parent = "pad";
    object.value = 300;
    object.propToReveal = "pad.sbhelipadprop";
    object.useObject = "pad.sbfasttravel";
    object.buildCompleteTrigger = "pad.sbbuildcompletescript";
    object.hideObject = "pad.hidden";
    return object;
}

SuperBuilditVariable variables(std::map<std::string, float> values) {
    return [values](const std::string& name) -> std::optional<float> {
        const auto found = values.find(name);
        if (found == values.end()) return std::nullopt;
        return found->second;
    };
}

}

int main() {
    try {
        const SuperBuilditVariable none = variables({});
        {
            chase::assets::PlacedObjectState state;
            SuperBuilditObject object = plate("pad.sbplate");
            object.state = std::uint16_t(SuperBuilditPhase::Built);
            object.requested = std::uint16_t(SuperBuilditPhase::Built);
            object.zoneActive = false;
            state.disabled.insert("pad.sbplate");
            require(superBuilditMessage(object, kMessageReset, state, none).handled, "message 8 is handled");
            require(!state.isDisabled("pad.sbplate") && state.isDisabled("pad.sbhelipadprop"), "reset enables the plate and hides PropToReveal (0x1bc118)");
            require(object.zoneActive && object.state == 1 && object.requested == 2, "reset: zone on, state 1, requested 2 (0x1bc160)");
        }
        {
            chase::assets::PlacedObjectState state;
            SuperBuilditObject object = plate("pad.sbplate");
            object.state = 2;
            object.requested = 2;
            superBuilditMessage(object, kSuperBuilditMessageClose, state, none);
            require(object.requested == 1 && object.state == 2, "0xfe in state 2 requests state 1 (0x1bbf18)");
            object.state = 3;
            object.requested = 3;
            superBuilditMessage(object, kSuperBuilditMessageClose, state, none);
            require(object.requested == 3, "0xfe in another state does nothing");
            require(!superBuilditMessage(object, 0xff, state, none).handled, "message 0xff is not handled by the class");
            require(superBuilditMessage(object, kSuperBuilditMessageSounds, state, none).handled && object.requested == 3, "0xfc only hands the sound id out");
        }
        {
            chase::assets::PlacedObjectState state;
            SuperBuilditObject helipad = plate("1-3_helipad.sbplate");
            const SuperBuilditVariable both = variables({{kSuperBuilditStoryVariable, 3.0f}, {kSuperBuilditBuiltVariable, 1.0f}});
            const SuperBuilditMessageResult result = superBuilditMessage(helipad, kSuperBuilditMessageStory, state, both);
            require(result.plateValueCleared && helipad.value == 0 && result.completeBrick == "1-3_rainbow_02", "message 6 clears the plate value and completes the brick");
            SuperBuilditObject early = plate("1-3_helipad.sbplate");
            require(!superBuilditMessage(early, kSuperBuilditMessageStory, state, variables({{kSuperBuilditStoryVariable, 2.0f}, {kSuperBuilditBuiltVariable, 1.0f}})).plateValueCleared &&
                        early.value == 300,
                    "StoryProgress 2 does nothing");
            SuperBuilditObject notBuilt = plate("1-3_helipad.sbplate");
            require(!superBuilditMessage(notBuilt, kSuperBuilditMessageStory, state, variables({{kSuperBuilditStoryVariable, 3.0f}, {kSuperBuilditBuiltVariable, 0.0f}})).plateValueCleared,
                    "the built variable at 0 does nothing");
            SuperBuilditObject other = plate("1-1_other.sbplate");
            require(!superBuilditMessage(other, kSuperBuilditMessageStory, state, both).plateValueCleared && other.value == 300, "any other plate ignores message 6");
        }
        {
            SuperBuilditObject object = plate("pad.sbplate");
            require(superBuilditSaveByte(object) == 0, "not built: 0");
            object.built = true;
            require(superBuilditSaveByte(object) == 1, "built: 1 (0x192608: bit 1 with bit 3 clear)");
            disableSuperBuilditSave(object, true);
            require(superBuilditSaveByte(object) == 0, "DisableSuperBuiltItSave 1 writes 0 (bit 3)");
            disableSuperBuilditSave(object, false);
            require(superBuilditSaveByte(object) == 1, "DisableSuperBuiltItSave 0 writes the built bit again");
        }
        {
            SuperBuilditSet set;
            set.add(plate("pad.sbplate"));
            SuperBuilditObject nosave = plate("pad.other");
            nosave.saveState = false;
            set.add(nosave);
            set.findMutable("pad.sbplate")->built = true;
            const std::vector<SuperBuilditRecord> records = superBuilditRecords(set);
            require(records.size() == 1 && records[0].hash == chase::assets::nameHash("pad.sbplate") && records[0].payload == 1, "one record for the object with SaveState");

            chase::assets::Level level;
            chase::assets::LevelScript script;
            script.nameHash = chase::assets::nameHash("complete");
            script.calls.push_back({});
            level.scripts.push_back(script);
            chase::assets::LevelTrigger trigger;
            trigger.event = "ObjectTriggered";
            trigger.objectName = "pad.sbbuildcompletescript";
            trigger.scriptName = "complete";
            level.triggers.push_back(trigger);
            chase::assets::PlacedObjectState state;
            state.disabled = {"pad.sbhelipadprop", "pad.sbfasttravel"};

            SuperBuilditSet loaded;
            loaded.add(plate("pad.sbplate"));
            loaded.add(nosave);
            ScriptRunner runner(level, state);
            require(restoreSuperBuildits(loaded, {{chase::assets::nameHash("pad.sbplate"), 0}}, state, runner) == 0 && !loaded.objects()[0].built, "bit 0 clear restores nothing");
            require(restoreSuperBuildits(loaded, {{chase::assets::nameHash("pad.other"), 1}}, state, runner) == 0, "no SaveState: the callback is not called");
            require(restoreSuperBuildits(loaded, {{chase::assets::nameHash("pad.sbplate"), 1}}, state, runner) == 1, "bit 0 set restores the object");
            require(loaded.objects()[0].built && state.isDisabled("pad.sbplate") && state.isDisabled("pad.hidden"), "built bit set, plate and HideObject disabled (0x192574..0x192588)");
            require(!state.isDisabled("pad.sbhelipadprop") && !state.isDisabled("pad.sbfasttravel"), "PropToReveal and UseObject enabled (0x192594, 0x1925b0)");
            const float away[3] = {0, 0, 0};
            runner.checkTriggers(away);
            require(runner.threadCount() == 1, "BuildCompleteTriggerObject is activated (0x1925a4)");
        }
        {
            chase::assets::Level level;
            chase::save::LevelStateInput input;
            input.keptRecords = {{0x1234, {1}}, {0x5678, {7}}};
            input.liveRecords = {{0x1234, {0}}, {0x9999, {1}}};
            const std::optional<std::vector<std::uint8_t>> data = chase::save::levelStateData(level, input);
            require(data.has_value() && data->size() == 9 + 9 + 9 + 4, "two kept records, one appended, the end marker");
            require((*data)[0] == 0x34 && (*data)[4] == 1 && (*data)[8] == 0, "a live record replaces the payload of the kept one");
            require((*data)[9] == 0x78 && (*data)[17] == 7, "another kept record stays as it was");
            require((*data)[18] == 0x99 && (*data)[22] == 1 && (*data)[26] == 1, "a live record without a kept one is appended (stand-in LEVOBJ-S100)");
        }
    } catch (const std::exception& error) {
        std::cerr << "superbuildit-messages: " << error.what() << '\n';
        return 1;
    }
    std::cout << "superbuildit-messages: ok\n";
    return 0;
}

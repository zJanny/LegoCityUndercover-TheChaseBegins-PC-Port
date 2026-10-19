// Super build-it rules on a made-up plate (src/runtime/super_buildit.cpp). Provenance: the update 0x1af3d0, the use query (message 3) of the
// handler 0x1bbec4, the state change 0x26d550 and the prompt owner 0x1cc994 / 0x1cc4d4, read with REA (Ghidra) and llvm-objdump of
// artifacts/game/game.arm.elf (docs/re/super-buildit.md). The counts 296 and 302 and the count 2 after the build are the RPC reads of 0x593430 in the
// user's oracle states m13-3-helipad-needs-bricks and m13-4-helipad-300-bricks (value 300, class data +0x80 read at 0x08af82d0).
#include "runtime/super_buildit.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t count(const std::vector<SuperBuilditAction>& actions, SuperBuilditActionKind kind) {
    return std::size_t(std::count_if(actions.begin(), actions.end(), [&](const SuperBuilditAction& action) { return action.kind == kind; }));
}

SuperBuilditObject plate() {
    SuperBuilditObject object;
    object.instance = "pad.sbplate";
    object.parent = "pad";
    object.value = 300;
    object.type = 1;
    object.propToReveal = "pad.sbhelipadprop";
    object.useObject = "pad.sbfasttravel";
    object.buildCompleteTrigger = "pad.sbbuildcompletescript";
    object.parentTrigger = "pad_built";
    object.world = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 100, 10, 200, 1};
    object.zoneCentre[1] = 0.1055f;
    object.zoneHalf[0] = object.zoneHalf[2] = 6.596f;
    object.zoneHalf[1] = 0.3055f;
    object.hasZone = true;
    return object;
}

SuperBuilditInput at(float x, float y, float z, std::uint32_t bricks, bool action) {
    SuperBuilditInput input;
    input.player[0] = x;
    input.player[1] = y;
    input.player[2] = z;
    input.playerFree = true;
    input.actionHeld = action;
    input.superBricks = bricks;
    return input;
}

}

int main() {
    try {
        chase::assets::PlacedObjectState state;
        {
            SuperBuilditSet set;
            set.add(plate());
            std::vector<SuperBuilditAction> actions = set.update(state, at(101, 10.2f, 199, 296, false));
            require(count(actions, SuperBuilditActionKind::PromptShown) == 0, "the first update only moves state 0 to 2 (create 0x1af05c requests 2)");
            require(set.objects()[0].state == std::uint16_t(SuperBuilditPhase::Waiting), "state 2 after the first update");
            actions = set.update(state, at(101, 10.2f, 199, 296, false));
            require(count(actions, SuperBuilditActionKind::PromptShown) == 1, "in the zone the prompt is taken");
            require(set.prompt() && set.prompt()->value == 300 && !set.prompt()->enough && set.prompt()->type == 1, "needs 300, 296 is not enough");
            actions = set.update(state, at(101, 10.2f, 199, 296, true));
            require(actions.empty() && !set.confirmOpen(), "A without enough bricks does nothing");
            actions = set.update(state, at(101, 10.6f, 199, 296, false));
            require(count(actions, SuperBuilditActionKind::PromptHidden) == 1, "0.41 above the plate is outside the zone (centre 0.1055, half 0.3055)");
            set.update(state, at(101, 10.2f, 199, 296, false));
            state.disabled.insert("pad");
            actions = set.update(state, at(101, 10.2f, 199, 296, false));
            require(actions.empty() && set.objects()[0].promptShown, "a disabled holder stops the update (the prompt stays as it was)");
            state.disabled.clear();
        }
        {
            SuperBuilditSet set;
            set.add(plate());
            set.update(state, at(101, 10.2f, 199, 302, false));
            set.update(state, at(101, 10.2f, 199, 302, false));
            require(set.prompt() && set.prompt()->enough, "302 is enough");
            std::vector<SuperBuilditAction> actions = set.update(state, at(101, 10.2f, 199, 302, true));
            require(count(actions, SuperBuilditActionKind::ConfirmOpened) == 1 && count(actions, SuperBuilditActionKind::PromptHidden) == 1, "A opens the dialog");
            require(set.confirmOpen() && set.objects()[0].requested == std::uint16_t(SuperBuilditPhase::Confirm) && set.objects()[0].state == 2,
                    "requested 4, state stays 2 (oracle halfwords +2 = 2, +4 = 4 while the dialog shows)");
            require(set.objects()[0].countSeen == 302, "+0x84 keeps the count");
            set.answer(kSuperBuilditDialogCancel);
            actions = set.update(state, at(101, 10.2f, 199, 302, false));
            require(set.objects()[0].state == 2 && set.objects()[0].requested == 2 && actions.empty(), "Cancel goes back to state 2");
            actions = set.update(state, at(101, 10.2f, 199, 302, false));
            require(count(actions, SuperBuilditActionKind::PromptShown) == 1, "the prompt comes back after Cancel");
            set.update(state, at(101, 10.2f, 199, 302, true));
            set.answer(kSuperBuilditDialogConfirm);
            actions = set.update(state, at(101, 10.2f, 199, 302, false));
            require(count(actions, SuperBuilditActionKind::MovieStarted) == 1, "OK starts the movie");
            require(set.objects()[0].built && set.objects()[0].requested == 3 && set.objects()[0].state == 2,
                    "built bit, requested 3, state 2 (oracle after OK: +2 = 2, +4 = 3, +0x92 = 0x0a)");
            actions = set.update(state, at(101, 10.2f, 199, 302, false));
            const auto builtAction = std::find_if(actions.begin(), actions.end(), [](const SuperBuilditAction& a) { return a.kind == SuperBuilditActionKind::Built; });
            require(count(actions, SuperBuilditActionKind::Built) == 1 && builtAction->countsBricks, "the next update builds it");
            require(actions.size() == 1 && !set.objects()[0].promptShown, "no prompt after the OK (stand-in gate LEVOBJ-S94; oracle m13-4 after the movie)");
            actions = {*builtAction};
            require(set.objects()[0].state == 3, "state 3 (oracle after the movie: +2 = 3, +4 = 3)");

            chase::assets::Level level;
            chase::assets::LevelScript script;
            script.nameHash = chase::assets::nameHash("pad_built");
            script.calls.push_back({});
            level.scripts.push_back(script);
            chase::assets::LevelTrigger trigger;
            trigger.event = "ObjectTriggered";
            trigger.objectName = "Pad_Built";
            trigger.scriptName = "pad_built";
            level.triggers.push_back(trigger);
            chase::assets::PlacedObjectState built;
            applySuperBuilditLink(set, built);
            require(built.isDisabled("pad.sbhelipadprop") && built.isDisabled("pad.sbfasttravel"), "the links hide PropToReveal and UseObject");
            ScriptRunner runner(level, built);
            const SuperBuilditSpend spend = applySuperBuilditActions(set, actions, built, runner);
            require(302 - spend.bricks == 2, "the value is taken from the count (oracle 0x593430: 302 before, 2 after)");
            require(spend.saveRequests == 1, "one save request 0x21cf38(0) after the subtraction");
            require(built.isDisabled("pad.sbplate") && !built.isDisabled("pad.sbhelipadprop") && !built.isDisabled("pad.sbfasttravel"),
                    "plate off, revealed prop and use object on");
            const float away[3] = {0, 0, 0};
            runner.checkTriggers(away);
            require(runner.threadCount() == 1, "the holder's TriggerObject fires its ObjectTriggered script");
        }
        {
            SuperBuilditSet set;
            SuperBuilditObject started = plate();
            started.requested = std::uint16_t(SuperBuilditPhase::Built);
            set.add(started);
            const std::vector<SuperBuilditAction> actions = set.update(state, at(0, 0, 0, 0, false));
            require(count(actions, SuperBuilditActionKind::Built) == 1 && !actions[0].countsBricks, "StartBuilt builds from state 0 without the trigger or the bricks");
        }
        require(superBuilditPicture({"objectunknown.png", "objectHelipad.png"}, 1) == "objecthelipad.png", "table row 1 is the helipad picture");
        require(superBuilditPicture({"objectunknown.png"}, 5).empty(), "no picture outside the table");
    } catch (const std::exception& error) {
        std::cerr << "super-buildit: " << error.what() << '\n';
        return 1;
    }
    std::cout << "super-buildit: ok\n";
    return 0;
}

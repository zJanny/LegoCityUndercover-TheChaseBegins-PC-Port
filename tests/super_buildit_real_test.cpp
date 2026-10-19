// The helipad super build-it of lvl_01_cherrytreehills (src/runtime/super_buildit.cpp, src/frontend/super_build_prompt.cpp): 1-3_Helipad
// (props/prop_sbhelipad_cherrytree, kind 253) nests SBPlate (props/prop_superbuildit). Provenance of the recorded values: the RPC reads of the user's
// oracle states (docs/re/super-buildit.md): class data of the plate entity 0x08af7b30 (+0x7c -> 0x08af8250: value +0x80 = 300, type +0x8c = 1),
// its use zone record 0x08e226e0 (centre 0 0.1055 0, half 6.596 0.3055 6.596), the TextPopup key 0x3f9b3c28 at the call 0x213e28 (ctrace) and the
// player positions of m13-3-helipad-needs-bricks (901.454 30.903 1139.463), m13-4-helipad-300-bricks (901.203 30.899 1141.268) and
// m13-5-helipad-built-menu (906.561 31.445 1138.943, on the fast travel pad). The build fires 1-3_Helipad_Built (level trigger ObjectTriggered).
// Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "frontend/super_build_prompt.hpp"
#include "runtime/level_start.hpp"
#include "runtime/super_buildit.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: super-buildit-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "super-buildit-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        runtime::SuperBuilditSet set;
        set.load(archive, level, classOf);
        const runtime::SuperBuilditObject* plate = set.find("1-3_helipad.sbplate");
        require(plate != nullptr, "1-3_Helipad.SBPlate is a super build-it");
        require(plate->parent == "1-3_helipad", "holder " + plate->parent);
        require(plate->value == 300, "SuperBuildValue of the holder (300) replaces the plate's 100: " + std::to_string(plate->value));
        require(plate->type == 1, "SuperBuildType 1");
        require(plate->textPopup == 0x3f9b3c28u, "TextPopup key");
        require(plate->fmv == "superbuildit_helipad", "FMVToPlay " + plate->fmv);
        require(plate->propToReveal == "1-3_helipad.sbhelipadprop", "PropToReveal " + plate->propToReveal);
        require(plate->useObject == "1-3_helipad.sbfasttravel", "UseObject " + plate->useObject);
        require(plate->buildCompleteTrigger == "1-3_helipad.sbbuildcompletescript", "BuildCompleteTriggerObject " + plate->buildCompleteTrigger);
        require(plate->parentTrigger == "1-3_helipad_built", "TriggerObject of the holder " + plate->parentTrigger);
        require(plate->requested == std::uint16_t(runtime::SuperBuilditPhase::Waiting), "StartBuilt 0");
        require(plate->hasZone, "use zone");
        const float centre[3] = {0.0f, 0.1055f, 0.0f}, half[3] = {6.596f, 0.3055f, 6.596f};
        for (int a = 0; a < 3; ++a) {
            require(near(plate->zoneCentre[a], centre[a], 0.001f), "zone centre " + std::to_string(a) + ": " + std::to_string(plate->zoneCentre[a]));
            require(near(plate->zoneHalf[a], half[a], 0.001f), "zone half " + std::to_string(a) + ": " + std::to_string(plate->zoneHalf[a]));
        }
        require(near(plate->world[12], 902.318f, 0.01f) && near(plate->world[13], 30.6955f, 0.01f) && near(plate->world[14], 1143.185f, 0.01f), "plate at the holder");
        const float needsBricks[3] = {901.454f, 30.903f, 1139.463f}, enoughBricks[3] = {901.203f, 30.899f, 1141.268f}, fastTravel[3] = {906.561f, 31.445f, 1138.943f};
        require(runtime::SuperBuilditSet::inZone(*plate, needsBricks), "m13-3 stands in the zone");
        require(runtime::SuperBuilditSet::inZone(*plate, enoughBricks), "m13-4 stands in the zone");
        require(!runtime::SuperBuilditSet::inZone(*plate, fastTravel), "m13-5 (on the fast travel pad, 0.75 higher) is outside");

        assets::PlacedObjectState state;
        runtime::ScriptRunner runner(level, state);
        runtime::applySuperBuilditLink(set, state);
        require(state.isDisabled("1-3_helipad.sbhelipadprop") && state.isDisabled("1-3_helipad.sbfasttravel"), "revealed prop and fast travel pad hidden at the link");
        runtime::SuperBuilditInput input;
        std::copy_n(enoughBricks, 3, input.player);
        input.playerFree = true;
        input.superBricks = 302;
        set.update(state, input);
        set.update(state, input);
        require(set.prompt() && set.prompt()->enough && set.prompt()->value == 300, "the prompt asks for 300 and 302 is enough");
        input.actionHeld = true;
        set.update(state, input);
        require(set.confirmOpen(), "A opens the confirmation");
        set.answer(runtime::kSuperBuilditDialogConfirm);
        input.actionHeld = false;
        set.update(state, input);
        const std::vector<runtime::SuperBuilditAction> actions = set.update(state, input);
        require(actions.size() == 1 && actions[0].kind == runtime::SuperBuilditActionKind::Built && actions[0].countsBricks, "built after the movie");
        const runtime::SuperBuilditSpend spend = runtime::applySuperBuilditActions(set, actions, state, runner);
        require(spend.bricks == 300 && spend.saveRequests == 1, "300 bricks spent, one save request");
        require(!state.isDisabled("1-3_helipad.sbhelipadprop") && !state.isDisabled("1-3_helipad.sbfasttravel"), "helipad and fast travel pad shown");
        const float away[3] = {0, 0, 0};
        runner.checkTriggers(away);
        bool started = false;
        for (const runtime::ScriptThread& thread : runner.threads())
            if (thread.script && thread.script->nameHash == assets::nameHash("1-3_helipad_built")) started = true;
        require(started, "the build starts 1-3_Helipad_Built");

        frontend::Resources resources(archive);
        const assets::FuseEntry* table = archive.find("tables/tablessuperbuildit.bxls");
        require(table != nullptr, "tablessuperbuildit");
        const assets::Table pictures = assets::parseTable(archive.read(*table));
        require(assets::tableValue(pictures.sheets.at(0), 1, 1).text == "objecthelipad.png", "row 1 of tablessuperbuildit");
        require(resources.textForKey("localisation/hinttext_uk.loc", plate->textPopup) == "Helipad", "the name comes from hinttext");
        frontend::SuperBuildPromptView view;
        view.prompt = resources.textForKey("localisation/gametext_uk.loc", runtime::kSuperBuilditBuildKey);
        view.enough = true;
        view.on = true;
        require(view.prompt == "Build", "Build text");
        const frontend::LayoutScene scene = frontend::superBuildPromptScene(resources, view);
        require(scene.hidden.count("brickPlaceholder1") && scene.hidden.count("brickPlaceholder2") && !scene.hidden.count("buyButton"), "A icon shown, placeholders hidden");
        const float buttonX = scene.positions.at("buyButton")[0];
        std::cout << "buyButton x " << buttonX << '\n';
        // Oracle picture m13-4-a: the A icon's image starts at screen x 188 (menu x 54, so 134 = 150 - 30 / 2 for the 30 pixel wide "Build"); the
        // port's frontendfont measures "Build" 24 wide (HUD-S73), which puts the icon at 138.
        require(near(buttonX, 134.0f, 5.0f), "A icon left of the prompt: " + std::to_string(buttonX));
        view.enough = false;
        require(frontend::superBuildPromptScene(resources, view).hidden.count("buyButton"), "no A icon without enough bricks");
    } catch (const std::exception& error) {
        std::cerr << "super-buildit-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "super-buildit-real-test: ok\n";
    return 0;
}

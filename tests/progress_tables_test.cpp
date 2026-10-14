// Tables behind tools/progress_report.py: archive file decoders, object class behaviours, handled script functions.
#include "assets/file_decoders.hpp"
#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace chase;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelObject objectOfClass(const char* className) {
    assets::LevelObject object;
    object.className = className;
    return object;
}
}

int main() {
    try {
        require(assets::fileDecoder("models/Textures/pc01_lombardstreet.BTGA") == "decodeTexture", "texture decoder");
        require(assets::fileDecoder("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl") == "parseLevel", "level decoder");
        require(assets::fileDecoder("levels/triggers.txt") == "parseConfigText", "trigger list decoder");
        require(assets::fileDecoder("levels/scriptfns.txt").empty(), "script function list has no decoder");
        require(assets::fileDecoder("pregen/customchar.bin").empty(), "pregenerated data has no decoder");

        require(assets::isDoorClass("Props/Prop_Door"), "door class case");
        require(assets::isPropFamilyClass("leprops/leprop_crate"), "leprop family");
        require(!assets::isPropFamilyClass("props/prop_crate"), "plain prop is not in the family");
        const assets::Level emptyClassFile;
        require(assets::objectBehaviours(objectOfClass("props/prop_door"), emptyClassFile) == (assets::BehaviourCollision | assets::BehaviourDoor),
                "door behaviours");
        require(assets::objectBehaviours(objectOfClass("props/prop_crate"), emptyClassFile) == 0, "prop without bound has no behaviour");
        assets::Level bounded;
        assets::LevelBound collisionBound;
        collisionBound.name = "CollisionBound";
        collisionBound.half[0] = collisionBound.half[1] = collisionBound.half[2] = 1;
        bounded.bounds.push_back(collisionBound);
        require(assets::objectBehaviours(objectOfClass("props/prop_crate"), bounded) == assets::BehaviourCollision, "prop with bound collides");
        require(assets::objectBehaviours(objectOfClass("characters/char_minifig"), bounded) == 0, "characters have no behaviour");
        require(assets::behaviourNames(assets::BehaviourCollision | assets::BehaviourStud) == "collision,stud", "behaviour names");

        require(runtime::scriptFunctionSupport("EnableObject") == runtime::ScriptFunctionSupport::Implemented, "EnableObject handled");
        require(runtime::scriptFunctionSupport("IsLevelUnlocked") == runtime::ScriptFunctionSupport::StandIn, "IsLevelUnlocked is a stand-in");
        require(runtime::scriptFunctionSupport("Wait") == runtime::ScriptFunctionSupport::Implemented, "Wait handled (0x174604)");
        require(runtime::scriptFunctionSupport("TriggerObject") == runtime::ScriptFunctionSupport::Implemented, "TriggerObject handled (0x1cddf8)");
        require(runtime::scriptFunctionSupport("WarningDialogBox") == runtime::ScriptFunctionSupport::None, "WarningDialogBox is not handled");
        const std::vector<std::string> handled = runtime::handledScriptFunctions();
        require(std::find(handled.begin(), handled.end(), "enablecollision") != handled.end(), "handler list has EnableCollision");
        for (const std::string& name : handled)
            require(runtime::scriptFunctionSupport(name) != runtime::ScriptFunctionSupport::None, "every listed handler is supported");
    } catch (const std::exception& error) {
        std::cerr << "progress-tables: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "progress-tables: ok\n";
    return 0;
}

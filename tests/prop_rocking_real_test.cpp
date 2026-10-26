// SetPropRocking, WaitForPlatform and AISuperJump in the real `1-3_Generator_Script` of lvl_01_cherrytreehills (docs/re/props-rocking.md), and
// the targets of every SetPropRocking and WaitForPlatform call of the level files. Needs the extracted RomFS; exits with 77 (skipped) when
// it is missing. Values from the level data, printed with the C++ level loader by 20261025-garage-C on 2026-10-25.
#include "assets/object_classes.hpp"
#include "runtime/node_attach.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr std::size_t kKindPropRecords = 23;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: prop-rocking-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "prop-rocking-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        unsigned waits = 0, rockings = 0;
        for (const std::string& name : archive.names()) {
            if (name.rfind("levels/levels/", 0) != 0 || name.size() < 5 || name.compare(name.size() - 5, 5, ".blvl") != 0) continue;
            const assets::Level level = assets::parseLevel(archive.read(*archive.find(name)));
            for (const assets::LevelScript& script : level.scripts)
                for (const assets::ScriptCall& call : script.calls) {
                    const std::string function = lower(call.function);
                    if ((function != "waitforplatform" && function != "setproprocking") || call.arguments.empty()) continue;
                    const assets::LevelObject* target = nullptr;
                    for (const assets::LevelObject& object : level.objects)
                        if (lower(object.instance) == lower(call.arguments[0])) target = &object;
                    require(target != nullptr, name + ": " + call.function + "(" + call.arguments[0] + ") names a placed object");
                    const assets::Level* cls = classOf(target->className);
                    require(cls != nullptr, "class of " + target->instance);
                    if (function == "waitforplatform") {
                        ++waits;
                        require(assets::objectKind(*target, *cls) == runtime::kPlatformKind, "WaitForPlatform(" + target->instance + ") names a kind 64 object");
                    } else {
                        ++rockings;
                        runtime::PropRocking prop;
                        require(runtime::propRockingOf(*target, *cls, prop), "SetPropRocking(" + target->instance + ") names a kind 67 object with Rocking");
                    }
                }
        }
        require(waits == 13 && rockings == 1, "13 WaitForPlatform and 1 SetPropRocking calls (the census of docs/re/script-natives-census.md), got " +
                                                  std::to_string(waits) + " and " + std::to_string(rockings));

        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
        assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner runner(level, state);
        runner.loadObjectKinds(classOf);
        runner.setObjectPositionSource([&level](const std::string& name, float out[3]) {
            const auto start = assets::objectStart(level, name);
            if (!start) return false;
            std::copy_n(start->position, 3, out);
            return true;
        });
        // 1-3_Crane_Girder (leprops/leprop_prop, room7): Rocking 1, no WaveBob parameter (defaults 2, 3, 0), initialorientation (179.3434, 0.1555, 179.9896).
        const runtime::PropRocking* girder = runner.propRocking().find("1-3_crane_girder");
        require(girder && girder->rocking && girder->period1 == 60 && girder->period2 == 90 && girder->phase == 0, "the girder rocks with the default waves");
        const std::vector<runtime::PropRocking>& records = runner.propRocking().props();
        const auto rockingProps = std::count_if(records.begin(), records.end(), [](const runtime::PropRocking& prop) { return prop.rocking; });
        // The kind 67 records of lvl_01 by the rule 0x16a334..0x16a40c, counted with the C++ level loader by 20261026-crane-C on 2026-10-26.
        require(rockingProps == 1 && records.size() == kKindPropRecords,
                "the girder is the only rocking prop of lvl_01 among " + std::to_string(records.size()) + " kind 67 records");

        // The level's startup script attaches the girder guy to the girder (AttachObject 0x1bf68c, pc34 of "startup") and the girder to the
        // crane platform (pc169).
        assets::PlacedObjectState startState = runtime::levelInitialState(level, classOf);
        runtime::ScriptRunner startup(level, startState);
        startup.loadObjectKinds(classOf);
        runtime::runLevelStartup(startup, runtime::kStartupStepDt, runtime::kStartupMaxUpdates);
        const runtime::Attachment* onGirder = startup.animation().attachment("1-3_ai_girder_guy");
        require(onGirder && onGirder->kind == runtime::AttachKind::Object && onGirder->parent == "1-3_crane_girder", "AttachObject(1-3_Crane_Girder, 1-3_AI_Girder_Guy)");
        const runtime::Attachment* onPlatform = startup.animation().attachment("1-3_crane_girder");
        require(onPlatform && onPlatform->parent == "1-3_crane_platform_dummy", "AttachObject(1-3_Crane_Platform_Dummy, 1-3_Crane_Girder)");
        const std::optional<assets::Placement> guyStart = assets::objectStart(level, "1-3_AI_Girder_Guy");
        require(guyStart.has_value(), "1-3_AI_Girder_Guy is placed");
        runtime::NodeAttachments links;
        links.attach("1-3_ai_girder_guy", "1-3_crane_girder", assets::placementMatrix(*guyStart), girder->placement);
        const assets::Matrix rocked = runtime::childWorld(links.find("1-3_ai_girder_guy")->local, runtime::rockingMatrix(*girder, 0));
        const float offset[3] = {rocked[12] - girder->placement[12], rocked[13] - girder->placement[13], rocked[14] - girder->placement[14]};
        const float placedOffset[3] = {guyStart->position[0] - girder->placement[12], guyStart->position[1] - girder->placement[13], guyStart->position[2] - girder->placement[14]};
        require(near(std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]),
                     std::sqrt(placedOffset[0] * placedOffset[0] + placedOffset[1] * placedOffset[1] + placedOffset[2] * placedOffset[2]), 1e-4f) &&
                    !near(offset[2], placedOffset[2], 0.02f),
                "the rocked girder swings the guy on its arc: the roll of 2.5 degrees at clock 0 moves him about 0.034 sideways");
        // RPC 20261025-garage-C-1 (recorded from a save state on the generator trigger spot of mission 1-3, the game's dt 0.033): the girder's
        // node goes from y 27.481 (u0) to 16.852 (from u192) with the platform, x and z unchanged, and the guy stands at (947.881, 17.617, 1142.456)
        // at u193. The girder is taken at its placed rotation here (the phase of the rocking when it stopped is not known).
        assets::Matrix lowered = girder->placement;
        lowered[13] = 16.852f;
        const assets::Matrix carried = runtime::childWorld(links.find("1-3_ai_girder_guy")->local, lowered);
        require(near(carried[12], 947.881f, 0.02f) && near(carried[13], 17.617f, 0.02f) && near(carried[14], 1142.456f, 0.02f),
                "the guy goes down with the girder to the oracle's position at u193, got y " + std::to_string(carried[13]));
        const std::array<float, 9> still = runtime::rockingRotation(*girder, 0);
        // Hand evaluation (Python, 20261025-garage-C): rows of 0x237c48(0.0436332, 0, 0) times the rows of the orientation in radians.
        const std::array<float, 9> expected = {-0.999996f, -0.000213f, -0.002712f, -0.000063f, 0.998483f, -0.055066f, 0.002719f, -0.055065f, -0.998479f};
        require(near(girder->placement[12], 947.8834f, 1e-3f), "the girder's placement");
        for (int i = 0; i < 9; ++i)
            require(near(still[i], expected[i], 2e-5f), "clock 0: roll 2.5 degrees (cos 0) on the 0x247660 angles of the placement, element " + std::to_string(i));
        require(runner.platforms().find("1-3_crane_platform_dummy") != nullptr, "1-3_Crane_Platform_Dummy (props/prop_platform) has a platform record");

        // STAND-IN: the script is started by name; the pull switch 1-3_Generator_Pull_Switch that triggers it is not used here.
        require(runner.start("1-3_Generator_Script"), "1-3_Generator_Script");
        // RPC 20261025-garage-C-1 (recorded from a save state on the generator trigger spot, the game's dt 0.033): the platform is triggered at
        // u90, SetPropRocking and AISuperJump run at u193, 103 updates later.
        constexpr float kTraceDt = 0.033f;
        const runtime::PlatformRecord* crane = runner.platforms().find("1-3_crane_platform_dummy");
        bool rockingStopped = false;
        int triggeredAt = -1, stoppedAt = -1;
        for (int i = 0; i < 400 && !rockingStopped; ++i) {
            runner.update(kTraceDt);
            if (triggeredAt < 0 && crane->state == runtime::kPlatformMovingState) triggeredAt = i;
            rockingStopped = !runner.propRocking().find("1-3_crane_girder")->rocking;
            if (rockingStopped) stoppedAt = i;
        }
        require(rockingStopped && triggeredAt >= 0 && stoppedAt - triggeredAt == 103,
                "SetPropRocking(1-3_Crane_Girder, 0) runs 103 updates after TriggerObject(1-3_Crane_Platform_Dummy, 1) (speed 0.01; stand-in SCRIPT-S296), got " +
                    std::to_string(stoppedAt - triggeredAt));
        require(crane->value == 1.0f && crane->state == runtime::kPlatformArrivedState, "the platform reached 1.0 and state 5");
        runner.update(kTraceDt);
        const runtime::AiCharacter* guy = runner.ai().character("1-3_ai_girder_guy");
        require(guy && guy->superJump, "AISuperJump(1-3_AI_Girder_Guy, 1-3_girder_Jump_2, 2) follows in the same thread");
        // 1-3_girder_Jump_2 (dummy/dummy, room7) at (941.8098, 17.3054, 1139.4764); the guy is placed at (947.8676, 28.259, 1142.4548).
        require(near(guy->superJump->target[0], 941.8098f, 1e-3f) && near(guy->superJump->target[1], 17.3054f, 1e-3f) && guy->superJump->height == 2.0f,
                "the jump goes to 1-3_girder_Jump_2 with height 2");
        require(guy->superJump->plan.frames == runtime::kSuperJumpNativeFrames, "a target below the guy keeps the 28 frames of the native");
        for (const char* name : {"setproprocking", "waitforplatform", "aisuperjump"}) require(runner.unhandledCalls(name) == 0, std::string(name) + " is handled");
    } catch (const std::exception& error) {
        std::cerr << "prop-rocking-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-rocking-real-test: ok\n";
    return 0;
}

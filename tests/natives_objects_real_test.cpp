// Scripts of lvl_01_cherrytreehills that use the object natives of batch SCRIPT-26, run by ScriptRunner. The expected values
// are the static call texts of the level file (dumped with the C++ level loader on 2026-10-14: every script of the level
// with its calls). Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: natives-objects-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-objects-real-test: data not available, skipped\n";
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

        {
            // Script 1-3_parkour_working: ToggleUseable(1-3_grab_01..04, 1), EnableObject(BrickGrab_Mount_Hint_06, 1).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-3_parkour_working") && runner.threadCount() == 0, "1-3_parkour_working");
            for (const char* grab : {"1-3_grab_01", "1-3_grab_02", "1-3_grab_03", "1-3_grab_04"}) {
                const runtime::ObjectNativeFlags* flags = runner.objectNatives().flags(grab);
                require(flags && flags->usable == true, std::string("usable ") + grab);
            }
            require(!runner.unhandledCalls("toggleuseable"), "ToggleUseable is handled");
        }
        {
            // Script 1-3_Platform_Down: ToggleUseable(1-3_Ledge_Platform_Switch_Ground, 0), PrintDebugString(...).
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-3_Platform_Down"), "1-3_Platform_Down");
            const runtime::ObjectNativeFlags* flags = runner.objectNatives().flags("1-3_ledge_platform_switch_ground");
            require(flags && flags->usable == false && !runner.unhandledCalls("printdebugstring"), "Platform_Down");
        }
        {
            // Script 1-3_Ledge_Platform_Switch_Bottom_Script ends with ResetObject of 1-3_Ledge_Platform_Switch,
            // 1-3_Ledge_Platform and 1-3_Ledge_Platform_Switch_Ground; the natives before it have no handler and continue.
            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            require(runner.start("1-3_Ledge_Platform_Switch_Bottom_Script"), "Switch_Bottom_Script");
            for (int i = 0; i < 10 && runner.threadCount(); ++i) runner.update(1.0f / 30.0f);
            const auto messages = runner.takeObjectClassMessages();
            require(messages.size() == 3 && messages[0].instance == "1-3_ledge_platform_switch" && messages[1].instance == "1-3_ledge_platform" &&
                        messages[2].instance == "1-3_ledge_platform_switch_ground",
                    "ResetObject messages");
            for (const runtime::ObjectClassMessage& message : messages) require(message.message == runtime::kMessageReset, "message 8");
        }
        {
            // 1-2_Scanned_Pringles: Wait 1.5, EnableScanner, KillScript(1-2_Scanner_Enter_Script), Wait 2, ... ShowTipTextTimed after
            // 10.5 s. 1-2_Scanner_Enter_Script: PrintDebugString, Wait 1.5, ShowTipTextTimed(938638044, 5), Wait 5, ShowTipTextTimed ...
            assets::PlacedObjectState alone = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner control(level, alone);
            control.start("1-2_Scanner_Enter_Script");
            for (int i = 0; i < 60; ++i) control.update(0.1f);
            require(control.unhandledCalls("showtiptexttimed") >= 1, "without the kill the scanner script shows its tip");

            assets::PlacedObjectState state = runtime::levelInitialState(level, classOf);
            runtime::ScriptRunner runner(level, state);
            runner.start("1-2_Scanner_Enter_Script");
            runner.start("1-2_Scanned_Pringles");
            for (int i = 0; i < 60; ++i) runner.update(0.1f);
            require(runner.unhandledCalls("showtiptexttimed") == 0, "KillScript ends the scanner script before its first tip");
            require(!runner.running("1-2_Scanner_Enter_Script", false), "the scanner script thread is gone");
            require(runner.running("1-2_Scanned_Pringles", false), "the killer continues");
            const runtime::ObjectNativeFlags* zone = runner.objectNatives().flags("1-2_scannerzoneaudio");
            require(zone && zone->usable == false, "ToggleUseable(1-2_ScannerZoneAudio, 0)");
        }
    } catch (const std::exception& error) {
        std::cerr << "natives-objects-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-objects-real-test: ok\n";
    return 0;
}

// The sound scanner of mission 1-2 in lvl_01_cherrytreehills (src/runtime/scanner.cpp): the zone 1-2_ScannerZoneAudio (object type 201) and
// the scannable 1-2_Dog_Prop from the level data, then the scanner sequence from the roof trigger: the player stands where the oracle state
// m12-8-roof-sound-scanner has him (869.659 65.997 645.594, docs/re/oracle-states.md), presses A, aims at the dog and holds A; the scripts
// 1-2_Scanner_Enter_Script, 1-2_Scanner_Zoomed_Script, 1-2_Scanner_Locked_Script and 1-2_Scanned_Pringles start in the original's order, and
// the EnableScanner 0 of 1-2_Scanned_Pringles closes the scanner. Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/level_start.hpp"
#include "runtime/scanner.hpp"
#include "runtime/script_runner.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

std::string lowerName(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: scanner-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "scanner-real-test: data not available, skipped\n";
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

        runtime::Scanner scanner;
        scanner.load(archive, level, classOf);
        require(scanner.zones().size() == 1, "lvl_01 has one ObjectType 201 zone with ScannerMode 1: " + std::to_string(scanner.zones().size()));
        const runtime::ScannerZone& zone = scanner.zones().front();
        require(zone.instance == "1-2_scannerzoneaudio", "zone " + zone.instance);
        require(near(zone.position[0], 869.48f, 0.01f) && near(zone.position[1], 65.953f, 0.01f) && near(zone.position[2], 645.766f, 0.01f), "zone position");
        require(near(zone.yawDegrees, 88.804f, 0.01f), "zone yaw " + std::to_string(zone.yawDegrees));
        require(zone.mode == 1, "ScannerMode 1 (class default, read as 1 at zone component +0x4c in the oracle)");
        require(zone.hasUseBound && near(zone.useCentre[1], 1.0f, 1e-4f) && near(zone.useHalf[0], 1.0f, 1e-4f) && near(zone.useHalf[1], 1.5f, 1e-4f),
                "UseBound of prop_scannerzoneaudio.blvl: centre (0, 1, 0.0253), half (1, 1.5, 1)");
        require(zone.enterTrigger == "1-2_scanner_enter_script", "EnterScanTriggerObject " + zone.enterTrigger);
        require(zone.zoomedInTrigger == "1-2_scanner_zoomed_script", "ZoomedInTriggerObject " + zone.zoomedInTrigger);
        require(zone.lockedOnTrigger == "1-2_scanner_locked_script", "LockedOnTriggerObject " + zone.lockedOnTrigger);
        require(zone.preEnterTrigger.empty() && zone.exitTrigger.empty() && zone.completeExitTrigger.empty(), "no pre-enter, exit or complete-exit object");
        const runtime::ScannableObject* dog = nullptr;
        std::size_t dogIndex = 0;
        for (std::size_t i = 0; i < scanner.objects().size(); ++i)
            if (scanner.objects()[i].instance == "1-2_dog_prop") dog = &scanner.objects()[i], dogIndex = i;
        require(dog != nullptr, "1-2_Dog_Prop (props/prop_dogwaggles, ShowOnAudioScan 1) is scannable");
        require(dog->triggerScanned == "1-2_scanned_pringles", "TriggerScanned " + dog->triggerScanned);

        assets::PlacedObjectState state = runtime::levelStartState(level, classOf);
        runtime::ScriptRunner scripts(level, state);
        for (const char* name : {"1-2_ScannerZoneAudio", "1-2_Dog_Prop", "1-2_Scanner_Spot_Waypoint_01", "1-2_Scanner_Enter_Script",
                                 "1-2_Scanner_Zoomed_Script", "1-2_Scanner_Locked_Script", "1-2_Scanned_Pringles"})
            for (auto it = state.disabled.begin(); it != state.disabled.end();) it = (*it == lowerName(name)) ? state.disabled.erase(it) : std::next(it);

        std::vector<std::string> started;
        std::vector<unsigned> startedAt;
        const std::vector<std::string> names = {"1-2_Scanner_Enter_Script", "1-2_Scanner_Zoomed_Script", "1-2_Scanner_Locked_Script", "1-2_Scanned_Pringles"};
        unsigned update = 0;
        runtime::ScannerInput input;
        input.player[0] = 869.659f;
        input.player[1] = 65.997f;
        input.player[2] = 645.594f;
        input.facing = std::int16_t(std::lround(std::atan2(-0.122f, -0.993f) * 10430.378f));
        unsigned calls = 0;
        const auto step = [&]() {
            const unsigned seen = scripts.objectives().recordedCalls(runtime::ObjectiveNativeKind::EnableScanner);
            if (seen != calls) {
                calls = seen;
                scanner.requestEnabled(scripts.objectives().scannerFlag());
            }
            input.enabled = [&](const std::string& instance) { return !state.isDisabled(instance); };
            for (const runtime::ScannerEvent& event : scanner.update(input))
                if (event.kind == runtime::ScannerEventKind::Trigger) scripts.triggerObjectEnabling(event.instance);
            input.player[0] = scanner.playerPosition()[0];
            input.player[1] = scanner.playerPosition()[1];
            input.player[2] = scanner.playerPosition()[2];
            input.facing = scanner.playerFacing();
            scripts.checkTriggers(input.player);
            scripts.update(input.dt);
            for (const runtime::ScriptThreadEvent& event : scripts.takeThreadEvents())
                if (event.started)
                    for (const std::string& name : names)
                        if (event.nameHash == assets::nameHash(name) && std::find(started.begin(), started.end(), name) == started.end()) {
                            started.push_back(name);
                            startedAt.push_back(update);
                        }
            ++update;
        };
        input.usePressed = true;
        step();
        input.usePressed = false;
        require(scanner.phase() == runtime::ScannerPhase::TakingOut, "the A press on the roof trigger takes the scanner out");
        while (scanner.phase() != runtime::ScannerPhase::Active && update < 100) step();
        require(scanner.phase() == runtime::ScannerPhase::Active, "the scanner view starts");
        require(near(scanner.playerPosition()[0], 869.48f, 0.01f) && near(scanner.playerPosition()[2], 645.766f, 0.01f), "the player stands on the zone");
        while (scanner.records().empty() && update < 200) step();
        require(scanner.records()[dogIndex].distance > 30.0f && scanner.records()[dogIndex].distance < 100.0f,
                "the dog is beyond 30 but within the linked range 100: " + std::to_string(scanner.records()[dogIndex].distance));
        input.shoulderR = 1.0f;
        for (int i = 0; i < 30; ++i) step();
        input.shoulderR = 0.0f;
        for (int i = 0; i < 400 && scanner.target() != int(dogIndex); ++i) {
            const runtime::ScannerRecord& record = scanner.records()[dogIndex];
            input.stickX = record.visible ? std::clamp((record.screen[0] - 200.0f) / 200.0f, -1.0f, 1.0f) : 1.0f;
            input.stickY = record.visible ? std::clamp((120.0f - record.screen[1]) / 120.0f, -1.0f, 1.0f) : 0.0f;
            step();
        }
        input.stickX = input.stickY = 0;
        require(scanner.target() == int(dogIndex), "the dog comes into the reticle");
        for (int i = 0; i < 40 && scanner.targetState() != 2; ++i) step();
        require(scanner.targetState() == 2, "the dog is locked");
        input.scanHeld = 1.0f;
        for (int i = 0; i < 80 && scanner.targetState() != 6; ++i) step();
        input.scanHeld = 0;
        require(scanner.targetState() == 6, "the dog is scanned");
        for (int i = 0; i < 120 && scanner.phase() != runtime::ScannerPhase::Idle; ++i) step();
        require(scanner.phase() == runtime::ScannerPhase::Idle, "EnableScanner 0 of 1-2_Scanned_Pringles (after Wait 1.5) closes the scanner");
        std::string order;
        for (const std::string& name : started) order += name + " ";
        require(started.size() == 4 && started[0] == names[0] && started[1] == names[1] && started[2] == names[2] && started[3] == names[3],
                "scripts in the order of the inputs (enter first, zoomed when R zoomed in, locked when the dog was held, scanned last): " + order);
        require(!scripts.running("1-2_Scanner_Enter_Script", false), "1-2_Scanned_Pringles kills 1-2_Scanner_Enter_Script");
        std::cout << "scanner-real-test: started";
        for (std::size_t i = 0; i < started.size(); ++i) std::cout << ' ' << started[i] << '@' << startedAt[i];
        std::cout << '\n';
    } catch (const std::exception& error) {
        std::cerr << "scanner-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scanner-real-test: ok\n";
    return 0;
}

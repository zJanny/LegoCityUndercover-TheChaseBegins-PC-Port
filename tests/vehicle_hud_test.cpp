// Test of the vehicle HUD rules (src/runtime/vehicle_hud.*, docs/re/vehicles-boarding.md): 0x262188 (vehicle portrait path, hearts at the begin of
// the Drive state), 0x193604 (when the portrait is shown) and 0x307bcc (hearts at the end). The expectations follow from the disassembly of those
// functions (format strings at 0x2622c8 / 0x2622e4 / 0x262300); the trooper's five hearts are the oracle picture of the seated HUD (five blue
// hearts, the vehicle table health 5 of docs/re/vehicles-system.md) and the portrait file names exist in the archive (real-data mode).
#include "assets/assets.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_hud.hpp"

#include <cstdio>
#include <filesystem>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

const auto kAlwaysThere = [](const std::string&) { return true; };
const auto kNeverThere = [](const std::string&) { return false; };

void portraitPath() {
    expect(vehiclePortraitPath("police_trooper", kAlwaysThere) == "sprites/portraits/police_trooper.bpng", "the table name picks the portrait");
    expect(vehiclePortraitPath("Police_Trooper", kAlwaysThere) == "sprites/portraits/police_trooper.bpng", "archive names are lower case");
    expect(vehiclePortraitPath("police_trooper", kNeverThere) == kVehiclePortraitFallback, "a missing file loads Question.tga");
}

void shownRules() {
    expect(!vehiclePortraitShown(false, BoardingPhase::Seated), "not loaded: the character's portrait");
    expect(!vehiclePortraitShown(true, BoardingPhase::OnFoot), "on foot: the character's portrait");
    expect(vehiclePortraitShown(true, BoardingPhase::Seated), "state 0x11c");
    expect(vehiclePortraitShown(true, BoardingPhase::JumpOut), "state 0x11b keeps the vehicle portrait");
    expect(vehiclePortraitShown(true, BoardingPhase::JumpIn), "state 0x11a with a loaded portrait");
}

void hearts() {
    VehicleHud hud;
    expect(!hud.end().has_value(), "no end without a begin");
    expect(!hud.portrait(BoardingPhase::JumpIn).has_value(), "the jump in shows the character: the portrait is loaded at the begin of Drive");
    const VehicleHud::Change begin = hud.begin(5, "police_trooper", 4, kAlwaysThere);
    expect(begin.hearts == 5 && begin.vehicleImage, "Drive begin: the vehicle's health in blue hearts");
    expect(hud.portrait(BoardingPhase::Seated) == std::optional<std::string>("sprites/portraits/police_trooper.bpng"), "seated portrait");
    expect(hud.portrait(BoardingPhase::JumpOut).has_value(), "jump out portrait");
    const std::optional<VehicleHud::Change> end = hud.end();
    expect(end && end->hearts == 4 && !end->vehicleImage, "Drive end: the player's hearts again, normal image");
    expect(!hud.end().has_value(), "the end runs once");
    expect(!hud.portrait(BoardingPhase::OnFoot).has_value(), "on foot frees the portrait");
    expect(!hud.portrait(BoardingPhase::JumpIn).has_value(), "and it stays gone");
}

void real(const char* fib, const char* names) {
    chase::assets::FuseArchive archive(fib);
    archive.loadNames(names);
    for (const char* name : {"police_trooper", "van_donut", "van_dulmen"})
        expect(vehiclePortraitPath(name, [&](const std::string& path) { return archive.find(path) != nullptr; }) == std::string("sprites/portraits/") + name + ".bpng",
               "the portrait of a vehicle table row is in the archive");
    expect(archive.find(kVehiclePortraitFallback) != nullptr, "the fallback portrait is in the archive");
    expect(archive.find("sprites/ui_hud/blend/images/ui_heartvehicle_3ds.bpng") != nullptr, "the vehicle heart image is in the archive");
    expect(archive.find("sprites/ui_hud/blend/images/heartsbg_generic.bpng") != nullptr, "heartsBG_generic is in the archive");
}

}

int main(int argc, char** argv) {
    portraitPath();
    shownRules();
    hearts();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("real data not found, skipping\n");
            return 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) return 1;
    std::printf("vehicle hud ok\n");
    return 0;
}

// The hand item track of the player's detect_* clips on the extracted RomFS (src/runtime/hand_item.cpp). Exits with 77 without the RomFS.
// Provenance: the track hash 0xba1e5b53 and its rise / fall handling from REA pseudo code of 0x185668 (branch at 0x1d5c24); the clip data from
// `parseAnimation` over models/char_minifig/class_police/detect_*_minifig.bfnanm (sub-animation 0 holds the track, its data block words point at
// the strings Ability_MagnifyingGlass and weaponRight); the object from levels/players.blvl through `placements` (meshname Ability_MagnifyingGlass),
// all read 2026-10-18 (docs/re/abilities-detective.md); the scanner_out / scanner_in clips of class_generic and the Ability_Scanner object read
// 2026-10-19 the same way (docs/re/abilities-scanner.md).
#include "assets/assets.hpp"
#include "runtime/hand_item.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: hand-item-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "hand-item-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const auto read = [&](const std::string& path) {
            const assets::FuseEntry* entry = archive.find(path);
            require(entry != nullptr, "not in archive: " + path);
            return archive.read(*entry);
        };
        const auto clip = [&](const std::string& name) { return assets::parseAnimation(read("models/char_minifig/class_police/" + name + "_minifig.bfnanm")); };

        for (const char* name : {"detect_out", "detect_in", "detect_idle", "detect_walk", "detect_run", "detect_jump", "detect_jump_run", "detect_land"}) {
            const auto item = runtime::handItem(clip(name));
            require(item && item->object == runtime::kMagnifyingGlassObject && item->bone == "weaponRight", std::string(name) + " hangs Ability_MagnifyingGlass on weaponRight");
        }
        for (const char* name : {"detect_find", "detect_fall", "detect_fallland", "idle", "walk"})
            require(!runtime::handItem(clip(name)), std::string(name) + " has no hand item track");

        const assets::Animation out = clip("detect_out"), in = clip("detect_in"), idle = clip("detect_idle"), run = clip("detect_run");
        require(runtime::handItemTrackValue(out, 0.0f) == 0.0f && runtime::handItemTrackValue(out, 0.3f) == 0.0f, "detect_out starts without the glass");
        require(runtime::handItemTrackValue(out, 0.37f) == 1.0f && runtime::handItemTrackValue(out, 0.99f) == 1.0f, "detect_out shows the glass from 0.36 of the clip");
        require(runtime::handItemTrackValue(in, 0.2f) == 1.0f && runtime::handItemTrackValue(in, 0.38f) == 0.0f, "detect_in hides the glass at 0.37 of the clip");
        require(runtime::handItemTrackValue(idle, 0.0f) == 1.0f && runtime::handItemTrackValue(idle, 0.99f) == 1.0f, "detect_idle keeps it");
        require(runtime::handItemTrackValue(run, 0.5f) == 1.0f && runtime::handItemTrackValue(run, 1.0f) == 0.0f, "detect_run falls at its last key");

        const auto genericClip = [&](const std::string& name) { return assets::parseAnimation(read("models/char_minifig/class_generic/" + name + "_minifig.bfnanm")); };
        const assets::Animation scannerOut = genericClip("scanner_out"), scannerIn = genericClip("scanner_in");
        for (const assets::Animation* clipOfScanner : {&scannerOut, &scannerIn}) {
            const auto item = runtime::handItem(*clipOfScanner);
            require(item && item->object == runtime::kScannerObject && item->bone == "weaponRight", "scanner clips hang Ability_Scanner on weaponRight");
        }
        require(runtime::handItemTrackValue(scannerOut, 0.3f) == 0.0f && runtime::handItemTrackValue(scannerOut, 0.4f) == 1.0f, "scanner_out shows the scanner from 0.4 of the clip");
        require(runtime::handItemTrackValue(scannerIn, 0.6f) == 1.0f && runtime::handItemTrackValue(scannerIn, 0.7f) == 0.0f, "scanner_in hides the scanner at 0.7 of the clip");

        const assets::Level players = assets::parseLevel(read("levels/players.blvl"));
        bool found = false;
        for (const assets::Placement& placement : assets::placements(players))
            if (placement.instance == runtime::kMagnifyingGlassObject) {
                found = true;
                require(placement.mesh == "Ability_MagnifyingGlass", "the glass object's meshname");
                require(archive.find(assets::meshModelPath(placement.mesh)) != nullptr, "its model is in the archive");
            }
        require(found, "players.blvl has Ability_MagnifyingGlass");
        bool foundScanner = false;
        for (const assets::Placement& placement : assets::placements(players))
            if (placement.instance == runtime::kScannerObject) {
                foundScanner = true;
                require(placement.mesh == "Weap_ScannerDevice", "the scanner object's meshname");
                require(archive.find(assets::meshModelPath(placement.mesh)) != nullptr, "the scanner model is in the archive");
            }
        require(foundScanner, "players.blvl has Ability_Scanner");
    } catch (const std::exception& error) {
        std::cerr << "hand-item-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "hand-item-real-test: ok\n";
    return 0;
}

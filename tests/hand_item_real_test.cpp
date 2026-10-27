// The hand item track of the player's detect_* clips on the extracted RomFS (src/runtime/hand_item.cpp). Exits with 77 without the RomFS.
// Provenance: the track hash 0xba1e5b53 and its rise / fall handling from REA pseudo code of 0x185668 (branch at 0x1d5c24); the clip data from
// `parseAnimation` over models/char_minifig/class_police/detect_*_minifig.bfnanm (sub-animation 0 holds the track, its data block words point at
// the strings Ability_MagnifyingGlass and weaponRight); the object from levels/players.blvl through `placements` (meshname Ability_MagnifyingGlass),
// all read 2026-10-18 (docs/re/abilities-detective.md); the scanner_out / scanner_in clips of class_generic and the Ability_Scanner object read
// 2026-10-19 the same way (docs/re/abilities-scanner.md). The zipline, hammer, whistle and tea cup clips and the objects of players.blvl:
// the same dump over every .bfnanm of models/char_minifig/class_*, 20261027-graphics-A, 2026-10-27 (docs/re/hand-items.md).
#include "assets/assets.hpp"
#include "runtime/hand_item.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

        const auto classClip = [&](const std::string& directory, const std::string& name) {
            return assets::parseAnimation(read("models/char_minifig/" + directory + "/" + name + "_minifig.bfnanm"));
        };
        const auto step = [&](runtime::HandItemTrack& track, const assets::Animation& animation, bool restarted, unsigned frame) {
            return track.update(animation, float(frame / double(animation.frames)), restarted);
        };

        const struct { const char* directory; const char* object; } ziplines[] = {
            {"class_police", "Ability_Handcuffs"}, {"class_construction", "Ability_Hammer"}, {"class_farmer", "Weapon_ChickenGun"},
            {"class_fire", "Ability_FireAxe"}, {"class_generic", "Ability_Banana"}, {"class_miner", "Weapon_PickAxe"},
            {"class_robber", "Ability_CrowBar"}, {"class_space", "Ability_AstroFlag"}};
        for (const auto& zipline : ziplines)
            for (const char* name : {"zipline_grab", "zipline_slide", "zipline_dismount"}) {
                const auto item = runtime::handItem(classClip(zipline.directory, name));
                require(item && item->object == zipline.object && item->bone == "weaponRight",
                        std::string(zipline.directory) + " " + name + " hangs " + zipline.object + " on weaponRight");
            }
        for (const char* name : {"zipline_left", "zipline_left_360", "zipline_left_end", "zipline_right", "zipline_right_360", "zipline_right_end"})
            require(!runtime::handItem(classClip("class_police", name)), std::string(name) + " has no hand item track");

        const assets::Animation grab = classClip("class_police", "zipline_grab"), slide = classClip("class_police", "zipline_slide"),
                                dismount = classClip("class_police", "zipline_dismount");
        require(runtime::handItemTrackValue(grab, 0.0f) == 0.0f && runtime::handItemTrackValue(grab, 0.02f) == 0.0f && runtime::handItemTrackValue(grab, 0.03f) == 1.0f,
                "the handcuffs show from 0.0256 of the grab clip");
        require(runtime::handItemTrackValue(slide, 0.0f) == 1.0f && runtime::handItemTrackValue(slide, 0.99f) == 1.0f, "the slide keeps them");
        require(runtime::handItemTrackValue(dismount, 0.1f) == 1.0f && runtime::handItemTrackValue(dismount, 0.17f) == 0.0f, "the dismount hides them at 0.1668 of the clip");

        runtime::HandItemTrack ride;
        std::vector<bool> events;
        bool restarted = true;
        for (const assets::Animation* clipOfRide : {&grab, &slide, &dismount}) {
            for (unsigned frame = 0; frame < clipOfRide->frames; ++frame) {
                if (const auto event = step(ride, *clipOfRide, restarted, frame)) {
                    require(event->item.object == "Ability_Handcuffs", "the ride's events name the handcuffs");
                    events.push_back(event->show);
                }
                restarted = false;
            }
            restarted = true;
        }
        require(!events.empty() && events.front() && !events.back(), "a zipline ride shows the handcuffs first and hides them last");

        const auto hammerIntro = classClip("class_construction", "atmfix_intro"), hammerOutro = classClip("class_construction", "atmfix_outro");
        for (const assets::Animation* hammer : {&hammerIntro, &hammerOutro}) {
            const auto item = runtime::handItem(*hammer);
            require(item && item->object == "Ability_Hammer" && item->bone == "weaponRight", "atmfix_intro / outro hang Ability_Hammer on weaponRight");
        }
        require(runtime::handItemTrackValue(hammerIntro, 0.19f) == 0.0f && runtime::handItemTrackValue(hammerIntro, 0.2f) == 1.0f, "atmfix_intro shows the hammer from 0.1952");
        require(runtime::handItemTrackValue(hammerOutro, 0.69f) == 1.0f && runtime::handItemTrackValue(hammerOutro, 0.7f) == 0.0f, "atmfix_outro hides it at 0.6956");
        const assets::Animation whistle = classClip("class_police", "whistle");
        const auto whistleItem = runtime::handItem(whistle);
        require(whistleItem && whistleItem->object == "Ability_Whistle" && whistleItem->bone == "weaponRight", "whistle hangs Ability_Whistle on weaponRight");
        require(runtime::handItemTrackValue(whistle, 0.04f) == 0.0f && runtime::handItemTrackValue(whistle, 0.05f) == 1.0f && runtime::handItemTrackValue(whistle, 0.96f) == 0.0f,
                "whistle shows the whistle from 0.0456 and hides it at 0.9548");
        const auto cup = runtime::handItem(classClip("class_generic", "drinking_intro"));
        require(cup && cup->object == "Ability_TeaCup", "drinking_intro hangs Ability_TeaCup");
        const auto fish = runtime::handItem(classClip("class_fire", "cat_tease"));
        require(fish && fish->object == "Ability_Fish" && fish->bone == "weaponLeft", "cat_tease hangs Ability_Fish on weaponLeft");

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
        for (const char* object : {"Ability_Handcuffs", "Ability_Hammer", "Weapon_ChickenGun", "Ability_FireAxe", "Ability_Banana", "Weapon_PickAxe", "Ability_CrowBar",
                                   "Ability_AstroFlag", "Ability_Whistle", "Ability_TeaCup", "Ability_Fish"}) {
            bool listed = false;
            for (const assets::Placement& placement : assets::placements(players))
                if (placement.instance == object) {
                    listed = true;
                    require(archive.find(assets::meshModelPath(placement.mesh)) != nullptr, std::string("the model of ") + object + " is in the archive");
                }
            require(listed, std::string("players.blvl has ") + object);
        }
    } catch (const std::exception& error) {
        std::cerr << "hand-item-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "hand-item-real-test: ok\n";
    return 0;
}

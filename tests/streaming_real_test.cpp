#include "assets/assets.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::assets;

void require(bool ok, const std::string& what) {
    if (!ok) throw std::runtime_error(what);
}

Level load(FuseArchive& archive, const char* path) {
    const FuseEntry* entry = archive.find(path);
    require(entry != nullptr, std::string("not in archive: ") + path);
    return parseLevel(archive.read(*entry));
}

std::vector<std::uint32_t> linkedRooms(const Level& level, std::uint32_t worldLevel, std::uint32_t room) {
    for (const auto& links : level.roomLinks)
        if (links.worldLevelHash == worldLevel && links.roomHash == room) {
            std::vector<std::uint32_t> out;
            for (const auto& entry : links.linked) out.push_back(entry.roomHash);
            return out;
        }
    throw std::runtime_error("no room link record");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: streaming-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "streaming-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);

        // Provenance of every number: section +0x34 and +0x1c block counts printed by a scratch dump of the pooled blocks
        // (docs/re/streaming.md 2026-10-13), loader 0x1cfc50 (ev_1eed109a…f773).
        const Level city = load(archive, "levels/levels/lvl_the_city.blvl");
        require(city.roomLinks.size() == 309, "lvl_the_city record count");
        const char* worlds[] = {"WorldLevel_00_PoliceStation", "WorldLevel_01_CherryTreeHills", "WorldLevel_02_Albatross Island",
                                "WorldLevel_03_Auburn", "WorldLevel_04_FortMeadows", "WorldLevel_05_BlueBellWood",
                                "WorldLevel_06_ApolloIsland", "WorldLevel_07_Downtown"};
        const std::size_t expected[] = {3, 43, 12, 51, 40, 56, 44, 60};
        std::size_t crossLevel = 0, badFlag = 0;
        for (std::size_t w = 0; w < 8; ++w) {
            std::size_t count = 0;
            for (const auto& links : city.roomLinks) {
                if (links.worldLevelHash != nameHash(worlds[w])) continue;
                ++count;
                for (const auto& entry : links.linked) {
                    if (entry.worldLevelHash != links.worldLevelHash) ++crossLevel;
                    if (entry.flag != 1) ++badFlag;
                }
            }
            require(count == expected[w], std::string("record count of ") + worlds[w]);
        }
        require(crossLevel == 0 && badFlag == 0, "link entries leave their world level or have a flag other than 1");

        // Oracle (state gameplay-start, Room8 current): rooms 1,2,3,7,8,9,14,15,16 are loaded together with 17,22,23.
        const std::uint32_t cherry = nameHash("WorldLevel_01_CherryTreeHills");
        std::vector<std::uint32_t> room8 = linkedRooms(city, cherry, nameHash("Room8"));
        std::vector<std::uint32_t> wanted;
        for (int n : {1, 2, 3, 7, 9, 14, 15, 16}) wanted.push_back(nameHash("Room" + std::to_string(n)));
        std::sort(room8.begin(), room8.end());
        std::sort(wanted.begin(), wanted.end());
        require(room8 == wanted, "Room8 links of WorldLevel_01_CherryTreeHills");

        const Level police = load(archive, "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl");
        require(police.roomLinks.size() == 1 && police.roomLinks[0].linked.empty(), "lvl_00 room links");
        require(police.triggers.size() == 53, "lvl_00 trigger count");
        const Level bluebell = load(archive, "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl");
        require(bluebell.roomLinks.size() == 54 && bluebell.triggers.size() == 221, "lvl_05 counts");

        const Level cherryTree = load(archive, "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        require(cherryTree.roomLinks.empty() && cherryTree.triggers.size() == 241, "lvl_01 counts");
        bool found = false;
        for (const auto& trigger : cherryTree.triggers)
            if (trigger.scriptName == "Police_ExtToInt" && trigger.event == "PlayerOnFootBoundEntered") {
                found = trigger.firstArgument == "Police_Station_Transition_Lower" && trigger.objectName.empty();
            }
        require(found, "Police_ExtToInt trigger");
    } catch (const std::exception& error) {
        std::cerr << "streaming-real-test: FAIL: " << error.what() << "\n";
        return 1;
    }
    std::cout << "streaming-real-test: OK\n";
    return 0;
}

// The use marker of the detective spot and the ability table behind its texture, against the real game data (src/runtime/ability_table.cpp,
// use_marker.cpp, docs/re/use-spot.md). Needs the extracted RomFS; exits with 77 without it.
// Provenance: the table rows are tables/tablescharacters.bxls (sheet 0 row 9 "police" bytes +15..+22, row 167 all ones, sheet 2 byte +6 of 64 rows;
// read with the loader of src/assets/table.cpp); the spot and its position are level data (lvl_01_cherrytreehills_main.blvl); the oracle is the user's
// state footprint-scanner-trigger (servant answer C-1, docs/re/oracle-states.md): kind 182 entity at the node 720.39 37.04 654.53, state 0, marker
// shown as the police disc (artifacts/oracle/r47-servant-out/c1_disc_sheet.png).
#include "assets/assets.hpp"
#include "runtime/ability_table.hpp"
#include "runtime/detective.hpp"
#include "runtime/use_marker.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr unsigned kChase = 9;
constexpr unsigned kAllAbilities = 167;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void abilityTable(const assets::Table& characters) {
    runtime::AbilityTable table;
    table.load(characters);
    require(table.characterHas(kChase, 25) && table.characterHas(kChase, 2), "Chase (row 9, police) has the police abilities 2 and 25");
    require(!table.characterHas(kChase, 28) && !table.characterHas(kChase, 30), "Chase has neither the astronaut nor the fireman ability");
    for (int ability = 0; ability < 64; ++ability) require(table.characterHas(kAllAbilities, ability), "row 167, the character of the marker query, has every ability");
    const int locked[] = {2, 38, 46, 47, 48, 49, 54, 55, 56, 57, 58, 59, 60};
    for (int ability = 0; ability < 64; ++ability) {
        bool expected = false;
        for (int row : locked) expected = expected || row == ability;
        require(table.startsLocked(ability) == expected, "unlock row of ability " + std::to_string(ability));
    }
    require(!table.startsLocked(25), "ability 25 does not start locked: the detective spot never shows the locked disc to Chase");
    require(runtime::useMarkerTexture(25, table.startsLocked(25), table.characterHas(kChase, 25)) == 1, "the detective spot of Chase shows the police disc");
    require(runtime::useMarkerTexture(2, table.startsLocked(2), table.characterHas(kChase, 2)) == runtime::kUseMarkerLockedTexture,
            "ability 2 starts locked: kinds 144..146 show the locked disc until it is unlocked");
    require(runtime::useMarkerTexture(30, false, table.characterHas(kChase, 30)) == runtime::kUseMarkerLockedTexture, "Chase lacks the fireman ability: locked disc");
}

void detectiveSpot(const assets::Level& level, const assets::ClassLookup& classOf) {
    runtime::DetectiveSet set;
    set.load(level, classOf);
    const runtime::DetectiveSpot* second = nullptr;
    for (const runtime::DetectiveSpot& spot : set.spots())
        if (spot.instance == "1-2_dtv_dogspot_02") second = &spot;
    require(second != nullptr, "the second dog spot of mission 1-2 is in the level");
    require(std::fabs(second->matrix[12] - 720.391f) < 1e-3f && std::fabs(second->matrix[13] - 37.0395f) < 1e-3f && std::fabs(second->matrix[14] - 654.534f) < 1e-3f,
            "the node matrix stands at the spot (oracle node 720.39 37.04 654.53)");
    require(second->state == 0 && runtime::detectiveSpotAnswersUseQuery(*second), "state 0: message 3 is answered, the marker shows");
    runtime::DetectiveSpot inMode = *second;
    inMode.state = 1;
    require(!runtime::detectiveSpotAnswersUseQuery(inMode), "in detective mode (state 1) the spot answers 0 and its marker fades out");
    inMode.state = 2;
    require(!runtime::detectiveSpotAnswersUseQuery(inMode), "a spot that is done (state 2) shows no marker");
    require(std::fabs(second->useZone.centre[1] - 1.0f) < 1e-6f && second->useZone.half[0] == 1.5f, "UseBound centre (0, 1, 0), half 1.5");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: use-marker-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "use-marker-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* characters = archive.find("tables/tablescharacters.bxls");
        require(characters != nullptr, "tablescharacters.bxls not in archive");
        abilityTable(assets::parseTable(archive.read(*characters)));
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        detectiveSpot(assets::parseLevel(archive.read(*entry)), assets::archiveClassLookup(archive));
    } catch (const std::exception& error) {
        std::cerr << "use-marker-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "use-marker-real-test: ok\n";
    return 0;
}

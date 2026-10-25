// Reference records of the main levels (src/runtime/reference_trigger_level.*) and the build-it caller on top of them. Values: the registration
// output of the kind table (loader call sites 0x233de8 and 0x23e84c, docs/re/script-triggers.md entry of 20261025-mission13-B) over the level
// files, run by 20261025-mission13-B with `reference-trigger-real-test`; the AND-joins are the groups of references that share one TriggerObject
// (all fires of one group, the three engine parts of 7-3). `DonutTable_14` has no entry: the oracle ctrace of the destroy chain (state
// the save state in front of the table of mission 1-2, five punches) shows 0x23211c(prop, 0, 1) raising no event (docs/re/props-wobbly-bouncer.md). Needs the extracted RomFS;
// exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/reference_trigger_level.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::Level load(assets::FuseArchive& archive, const std::string& file) {
    const assets::FuseEntry* entry = archive.find("levels/levels/" + file + "/" + file + "_main.blvl");
    require(entry != nullptr, "not in archive: " + file);
    return assets::parseLevel(archive.read(*entry));
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: reference-trigger-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "reference-trigger-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        const assets::Level cherryTree = load(archive, "lvl_01_cherrytreehills");
        runtime::ReferenceTriggers cherry;
        runtime::registerLevelReferences(cherry, cherryTree, classOf);
        require(cherry.size() == 121, "lvl_01 registers 121 records, got " + std::to_string(cherry.size()));
        const auto van = cherry.findOwner("1-1_bi_donutvanparts");
        require(van && cherry.target(*van) == "1-1_so_vanfixed", "the van parts build-it: TriggerObject 1-1_SO_VanFixed");
        const auto crane = cherry.findOwner("1-3_crane_switch");
        require(crane && cherry.target(*crane) == "1-3_start_rescue_lawnmower_man" && cherry.navNode(*crane) == runtime::kReferenceNoNavNode,
                "the mission 1-3 crane switch (kind 58) registers its TriggerObject");
        for (std::size_t i = 0; i < cherry.size(); ++i) require(cherry.navNode(i) == runtime::kReferenceNoNavNode, "no record of lvl_01 names a NavNode (class default 255.0)");
        require(!cherry.findOwner("donuttable_14"), "a wobbly destructible (kind 119) has no entry (oracle: no event after 0x23211c(prop, 0, 1))");
        require(!cherry.findOwner("player1"), "an object of another kind has no entry");

        const assets::Level downtown = load(archive, "lvl_07_downtown");
        runtime::ReferenceTriggers town;
        runtime::registerLevelReferences(town, downtown, classOf);
        require(town.size() == 320, "lvl_07 registers 320 records, got " + std::to_string(town.size()));
        for (std::size_t i = 0; i < town.size(); ++i) require(town.navNode(i) == runtime::kReferenceNoNavNode, "no record of lvl_07 names a NavNode");
        const auto fire1 = town.findOwner("7-1_fire_01"), fire2 = town.findOwner("7-1_fire_02"), fire3 = town.findOwner("7-1_fire_03");
        require(fire1 && fire2 && fire3 && town.target(*fire1) == "7-1_firesout" && town.target(*fire3) == "7-1_firesout", "three fires share 7-1_FiresOut");
        require(town.activate(*fire1).joinWaiting && town.activate(*fire2).joinWaiting, "the first two fires wait");
        const runtime::ReferenceActivation allOut = town.activate(*fire3);
        require(allOut.fired && allOut.target == "7-1_firesout" && allOut.owners.size() == 3, "the third fire fires the target and all three records");
        const auto part1 = town.findOwner("7-3_enginepart_bi_01"), part2 = town.findOwner("7-3_enginepart_bi_02"), part3 = town.findOwner("7-3_enginepart_bi_03");
        require(part1 && part2 && part3, "three engine part build-its");
        require(!town.activate(*part1).fired && !town.activate(*part3).fired && town.activate(*part2).fired, "the engine is fixed with the last of the three parts");

        runtime::BuilditSet set;
        set.load(archive, cherryTree, classOf);
        const runtime::BuilditObject* parts = set.find("1-1_bi_donutvanparts");
        require(parts != nullptr && parts->reference && set.references().target(*parts->reference) == "1-1_so_vanfixed", "the build-it holds its record of the level list");
        std::size_t partsIndex = 0;
        while (&set.objects()[partsIndex] != parts) ++partsIndex;
        assets::PlacedObjectState state;
        runtime::ScriptRunner scripts(cherryTree, state);
        state.disabled.insert("1-1_so_vanfixed");
        runtime::applyBuilditActions(set, {{runtime::BuilditActionKind::Built, partsIndex}}, state, scripts);
        require(!state.isDisabled("1-1_so_vanfixed"), "the finished build enables TriggerObject");
        const std::vector<runtime::ScriptObjectMessage> sent = scripts.takeObjectMessages();
        require(sent.size() == 3 && sent[0].instance == "1-1_so_vanfixed" && sent[0].message == 10 && sent[1].instance == "1-1_bi_donutvanparts" &&
                    sent[1].message == 0xff && sent[2].instance == "1-1_so_vanfixed" && sent[2].message == 0xff,
                "message 10 to the target, 0xff to the build-it, 0xff to the target");
        runtime::applyBuilditActions(set, {{runtime::BuilditActionKind::Built, partsIndex}}, state, scripts);
        require(set.references().kind(*parts->reference) == runtime::ReferenceKind::Fired, "the record stays Fired after a second finish (deactivate, then activate)");
    } catch (const std::exception& error) {
        std::cerr << "reference-trigger-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "reference-trigger-real-test: ok\n";
    return 0;
}

// The award viewer with the shipped tables, texts and pictures (src/frontend/award_viewer.cpp). Provenance: the item list and its texts and pictures
// come from page enter 0x304e20 (gametext 0xc6103ad9 for item 1, TablesLevels rows 2..8 field +8 and name +0x30), the title from 0x1d2db4 (gametext
// 0x90a614ef), read by 20261028-uiport-C on 2026-10-28 (REA, Ghidra 12.1.2); the totals are the sums of the shipped TablesSuperCounters and
// TablesLevels columns that 0x1f61f0 adds. Needs the extracted RomFS; exits with 77 without it.
#include "frontend/award_viewer.hpp"
#include "save/completion_tables.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: award-viewer-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "award-viewer-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        Resources resources(archive);
        const AwardViewerSetup setup = awardViewerSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc");
        require(setup.title == "Awards", "title: " + setup.title);
        require(setup.items[0].text == "Overview" && setup.items[0].image == "UI_AwardViewer/UI_AwardViewer_General.png", "item 1: " + setup.items[0].text);
        const char* names[] = {"lvl_01_cherrytreehills", "lvl_02_albatrossisland", "lvl_03_auburn", "lvl_04_fortmeadows",
                               "lvl_06_apolloisland",    "lvl_05_bluebellwood",    "lvl_07_downtown"};
        for (unsigned i = 1; i < chase::runtime::kAwardViewerItems; ++i) {
            require(setup.items[i].image == std::string("UI_LevelSelect/Portraits/") + names[i - 1] + ".png", "picture of item " + std::to_string(i + 1) + ": " + setup.items[i].image);
            require(!setup.items[i].text.empty(), "text of item " + std::to_string(i + 1));
            require(resources.has("sprites/ui_levelselect/portraits/" + std::string(names[i - 1]) + ".bpng"), "portrait in the archive");
        }
        require(setup.items[1].text == "Cherry Tree Hills", "item 2 text: " + setup.items[1].text);
        require(resources.has("sprites/ui_awardviewer/ui_awardviewer_general.bpng") && resources.has(kAwardTopTexture), "pictures in the archive");

        const chase::save::CompletionTotals totals = chase::save::completionTotals(archive);
        std::array<chase::runtime::LevelAwardData, chase::runtime::kAwardLevels> levels{};
        for (unsigned l = 0; l < chase::runtime::kAwardLevels; ++l) {
            for (unsigned c = 0; c < chase::runtime::kAwardCategories; ++c) levels[l].totals[c] = totals.categoryTotals[c][l];
            levels[l].missions = totals.levelTotals[l];
            levels[l].done = 0;
        }
        AwardViewer viewer(setup);
        viewer.open(levels);
        const std::array<std::string, 3> texts = viewer.countTexts();
        require(texts[0] == "0/260" && texts[1] == "0/46" && texts[2] == "0/7", "new game totals: " + texts[0] + " " + texts[1] + " " + texts[2]);
        for (int i = 0; i < 40; ++i) viewer.update(1.0f / 30, {});
        require(viewer.round().selected() == 0 && viewer.round().items().size() == 8, "open on item 1");
        Canvas top(kTopWidth, kScreenHeight), bottom(kBottomWidth, kScreenHeight);
        viewer.drawTop(top, resources);
        viewer.drawBottom(bottom, resources);
        const std::uint8_t* edge = top.pixel(kTopWidth - 1, kScreenHeight / 2);
        require(edge[2] > 60 && edge[2] > 4 * edge[0], "the picture's 400 x 240 texels cover the top screen 1:1 (blue at the right edge, not squeezed)");
    } catch (const std::exception& error) {
        std::cerr << "award-viewer-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "award-viewer-real-test: ok\n";
    return 0;
}

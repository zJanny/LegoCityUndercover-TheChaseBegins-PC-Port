// The save writer and the slot row on the user's save (built from the named values of save_fixture.hpp).
// Usage: save-writer-real-test <lego_city3ds.fib> <lego_city3ds.csv>; exits 77 (skip) when a file is missing.
#include "save_fixture.hpp"
#include "save/completion_tables.hpp"
#include "save/save_writer.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
using namespace chase;

namespace {
constexpr std::uint32_t kLevelOneLocationHash = 2577700376u;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3 || !std::ifstream(argv[1]) || !std::ifstream(argv[2])) {
        std::cout << "save-writer-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const save::SaveImage loaded = testing::userSaveImage(archive);
        require(loaded.validate() == save::Status::Ok, "the save is not a valid image");
        const save::Bytes bytes = loaded.bytes();
        save::SaveImage image = loaded;

        const save::SlotView slot0 = image.slotView(0);
        const std::optional<save::SlotRow> row = save::slotRow(slot0);
        require(row && row->used, "slot 0 row");
        // Provenance: slot 0 of the user's save as read on 2026-10-20 (play ticks u32 +0, per mille u16 +4, level byte +6): 10462, 2, 1.
        require(row->playTicks == 10462 && row->permille == 2 && row->levelByte == 1, "slot 0 row values");
        require(save::formatPlayTime(row->playTicks) == "05:48" && save::formatPercent(row->permille, 1) == "0.2%", "row texts");
        require(!save::slotRow(image.slotView(1))->used, "slot 1 is empty");

        // Provenance: chase-tabledump tables/tableslevels.bxls, row 1, column 11 = -1717266920.
        const chase::assets::FuseEntry* levels = archive.find("tables/tableslevels.bxls");
        require(levels != nullptr, "TablesLevels");
        const chase::assets::Table table = chase::assets::parseTable(archive.read(*levels));
        require(save::locationTextHash(table, save::locationLevel(row->levelByte)) == kLevelOneLocationHash, "location hash");
        require(!save::locationTextHash(table, 28) && !save::locationTextHash(table, -1), "rows outside the table");

        const save::CompletionTotals totals = save::completionTotals(archive);
        require(save::completionPermille(slot0, totals) == row->permille, "recomputed permille equals the stored one");

        const save::Bytes live(slot0.data, slot0.data + slot0.size);
        require(save::commitSlot(image, 0, live), "commit");
        require(image.bytes() == bytes, "committing the slot's own bytes leaves the file unchanged");

        save::Bytes edited = live;
        save::setSlotPermille(edited.data(), 0x1234);
        require(save::commitSlot(image, 0, edited) && image.validate() == save::Status::Ok, "edited commit");
        require(image.bytes() != bytes && image.slot(0)[4] == 0x34 && image.slot(0)[5] == 0x12, "edit visible");
        std::cout << "save-writer-real-test passed\n";
    } catch (const std::exception& error) {
        std::cerr << "save-writer-real-test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

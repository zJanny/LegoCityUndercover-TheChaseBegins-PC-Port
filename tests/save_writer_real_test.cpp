// The save writer and the slot row on the user's real save (a copy, never committed, never modified).
// Usage: save-writer-real-test <lego_city3ds.fib> <lego_city3ds.csv> <savegame.dat>; exits 77 (skip) when a file is
// missing or the save is not the pinned one.
#include "save/completion_tables.hpp"
#include "save/save_writer.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
using namespace chase;

namespace {
constexpr std::uint32_t kPinnedCrc = 0x937dacf8;
constexpr std::uint32_t kLevelOneLocationHash = 2577700376u;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 4 || !std::ifstream(argv[1]) || !std::ifstream(argv[2]) || !std::ifstream(argv[3])) {
        std::cout << "save-writer-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        std::ifstream file(argv[3], std::ios::binary);
        const save::Bytes bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        const std::optional<save::SaveImage> loaded = save::SaveImage::fromBytes(bytes);
        require(loaded && loaded->validate() == save::Status::Ok, "the save is not a valid image");
        if (loaded->storedChecksum() != kPinnedCrc) {
            std::cout << "save-writer-real-test: this is not the pinned save, skipped\n";
            return 77;
        }
        save::SaveImage image = *loaded;

        const save::SlotView slot0 = image.slotView(0);
        const std::optional<save::SlotRow> row = save::slotRow(slot0);
        require(row && row->used, "slot 0 row");
        // Provenance: values read from the pinned copy with python struct at 0x234 (u32, u16, u8, u8): 10462, 2, 1, 4.
        require(row->playTicks == 10462 && row->permille == 2 && row->levelByte == 1, "slot 0 row values");
        require(save::formatPlayTime(row->playTicks) == "05:48" && save::formatPercent(row->permille, 1) == "0.2%", "row texts");
        require(!save::slotRow(image.slotView(1))->used, "slot 1 is empty");

        // Provenance: chase-tabledump tables/tableslevels.bxls, row 1, column 11 = -1717266920.
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const chase::assets::FuseEntry* levels = archive.find("tables/tableslevels.bxls");
        require(levels != nullptr, "TablesLevels");
        const chase::assets::Table table = chase::assets::parseTable(archive.read(*levels));
        require(save::locationTextHash(table, save::locationLevel(row->levelByte)) == kLevelOneLocationHash, "location hash");
        require(!save::locationTextHash(table, 28) && !save::locationTextHash(table, -1), "rows outside the table");

        const save::CompletionTotals totals = save::completionTotals(archive);
        require(save::completionPermille(slot0, totals) == row->permille, "recomputed permille equals the stored one");

        const save::Bytes live(slot0.data, slot0.data + slot0.size);
        require(save::commitSlot(image, 0, live), "commit");
        require(image.bytes() == bytes, "committing the slot's own bytes reproduces the file byte for byte");

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

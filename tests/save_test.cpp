// Synthetic save images: header, CRC, validation statuses and atomic storage. No real save exists.
#include "save/save.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
using namespace chase::save;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void writeFile(const std::filesystem::path& path, const Bytes& data) {
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
}
}

int main() {
    try {
        // Reference: zlib crc32("123456789") = 0xcbf43926, and this CRC is its complement (0x248308).
        const std::string check = "123456789";
        require(checksum(reinterpret_cast<const std::uint8_t*>(check.data()), check.size()) == 0x340bc6d9u,
                "CRC variant");
        // Descriptor 0x272948: 0x18 + 0x21c + 2 * 0x5230 = 0xa694.
        require(kFileSize == 0xa694, "file size");

        SaveImage image;
        require(image.validate() == Status::Ok, "fresh image is valid");
        require(image.storedSize() == 0xa694 && image.storedVersion() == 30, "header fields");
        const Bytes& raw = image.bytes();
        require(std::string(raw.begin() + 8, raw.begin() + 24) == "LEGO_ENGINE_SAVE", "magic");
        require(image.slot(0) - raw.data() == 0x234 && image.slot(1) - raw.data() == 0x5464, "slot offsets");
        bool threw = false;
        try { image.slot(2); } catch (const std::out_of_range&) { threw = true; }
        require(threw, "slot index checked");

        // Totals: docs/re/save-format.md 09:25 (level byte +0x40 of rows 2..8) and 09:19 (TablesSuperCounters rows,
        // `xxd -s 0x50 -l 104 artifacts/fib/files/tables/tablessupercounters.bxls`, cell sum 271).
        CompletionTotals totals;
        totals.levelTotals = {7, 8, 5, 8, 9, 8, 8};
        totals.categoryTotals = {{{5, 1, 5, 4, 4, 4, 5}, {8, 0, 5, 1, 1, 2, 7}, {4, 4, 4, 4, 4, 4, 4},
                                  {5, 4, 5, 0, 4, 5, 5}, {4, 4, 4, 4, 4, 4, 4}, {4, 4, 4, 4, 4, 4, 4},
                                  {4, 4, 4, 4, 4, 4, 4}, {4, 4, 4, 4, 4, 4, 4}, {1, 1, 2, 2, 2, 2, 2},
                                  {1, 1, 1, 1, 1, 1, 1}, {2, 0, 2, 1, 1, 2, 2}, {2, 0, 2, 2, 1, 2, 2},
                                  {2, 0, 2, 2, 1, 2, 2}}};
        totals.excludedCharacters = {9};
        std::uint32_t cellSum = 0;
        for (const auto& row : totals.categoryTotals)
            for (std::uint8_t cell : row) cellSum += cell;
        require(cellSum == 271, "supercounter cell sum (09:19 table dump)");

        // Hand-computed from docs/re/save-format.md 09:25: 53 + 42 + 2*132 + 2*41 + 271 - 1 = 711.
        SaveImage progress;
        SlotView empty = progress.slotView(0);
        require(completionCounts(empty, totals)->total == 711, "documented completion total");
        // Police (id 9) both bits set, id 9 subtracts one from each side: collected 2 - 1 = 1, 1000 / 711 = 1.
        progress.slot(0)[0x51be] = 0x0c;
        require(completionPermille(progress.slotView(0), totals) == 1, "police-only slot");

        // Hand-computed from docs/re/save-format.md 09:13 offsets, one value per source of the collected side.
        SaveImage filled;
        std::uint8_t* slot = filled.slot(0);
        slot[0x827] = 7;                  // level 2 byte +0x81f (block +0x8)
        slot[0x38e7] = 8;                 // level 8 byte +0x81f (block +0x30c8)
        slot[0x5147] = 99;                // level 1 block is not part of the sum
        slot[0x51b5] = 0xff;              // set 0 indices 0..7
        slot[0x51b7] = 0x3f;              // set 0 indices 16..20; bit 21 is outside the set
        slot[0x51b8] = 0x01;              // set 1 index 0
        slot[0x51be] = 0x30;              // character 10, both bits (bits 20, 21)
        slot[0x51bd] = 0x0c;              // character 5 is below id 9 and not counted
        slot[0x51e3] = 0x01;              // id 1 first bit
        slot[0x51ed] = 0x02;              // id 41 second bit (bit 81)
        slot[0x51fa] = 0xf1;              // flags 0 (level 2) and 4..7 (level 3) for category 7
        slot[0x8] = 5;                    // level 2 category 0 counter
        slot[0x1060] = 3;                 // level 4 category 12 counter (block +0x1048, +24)
        // 15 + 14 + 2 + 2 + (5 + 3 + 1 + 4) - 1 = 45; 45 * 1000 / 711 = 63.
        std::optional<CompletionCounts> counts = completionCounts(filled.slotView(0), totals);
require(counts && counts->collected == 45 && counts->total == 711, "filled slot counts");
        require(completionPermille(filled.slotView(0), totals) == 63, "filled slot per-mille");

        require(*levelCollected(filled.slotView(0), 1) == 99 && *levelCollected(filled.slotView(0), 8) == 8, "level bytes");
        require(*categoryCounter(filled.slotView(0), 7, 3) == 4 && *categoryCounter(filled.slotView(0), 7, 2) == 1,
                "category 7 sums flags");
        require(*characterBit(filled.slotView(0), 10, 1) && !*characterBit(filled.slotView(0), 11, 0), "character bits");
        require(*extraSetBit(filled.slotView(0), 0, 20) && !*extraSetBit(filled.slotView(0), 1, 1), "extra set bits");
        require(*idSetBit(filled.slotView(0), 41, 1) && !*idSetBit(filled.slotView(0), 41, 0), "id set bits");

        // 0x1cd5e8: min(1000, collected * 1000 / total), unsigned truncating.
        require(completionPermille(0u, 711u) == 0 && completionPermille(1u, 3u) == 333, "per-mille truncation");
        require(completionPermille(711u, 711u) == 1000 && completionPermille(800u, 711u) == 1000, "per-mille cap");
        require(completionPermille(5u, 0u) == 0, "zero total");

        // u16 counters are little-endian: bytes 02 01 at level 3 category 1 (block +0x828, +2) = 0x0102.
        SaveImage wide;
        wide.slot(0)[0x82a] = 0x02;
        wide.slot(0)[0x82b] = 0x01;
        require(*categoryCounter(wide.slotView(0), 1, 3) == 0x0102, "counter byte order");

        require(levelBlockOffset(1) == 0x4928 && levelBlockOffset(2) == 0x8 && levelBlockOffset(8) == 0x30c8, "block offsets");
        require(!levelBlockOffset(0) && !levelBlockOffset(9), "level range");
        require(!categoryCounter(empty, 13, 2) && !categoryCounter(empty, 0, 1), "counter range");
        require(!characterBit(empty, 141, 0) && !idSetBit(empty, 0, 0) && !extraSetBit(empty, 0, 21) && !flagBit(empty, 28),
                "bit ranges");
        require(!levelCollected(SlotView{empty.data, 0x827}, 2) && !completionCounts(SlotView{empty.data, 0x100}, totals),
                "short slot");
        CompletionTotals badExclusion = totals;
        badExclusion.excludedCharacters = {8};
        require(!completionCounts(empty, badExclusion), "excluded id range");

        image.slot(1)[0x5190] = 0x42;
        require(image.validate() == Status::Corrupt, "edit without seal breaks CRC");
        image.seal();
        require(image.validate() == Status::Ok && image.slot(1)[0x5190] == 0x42, "seal keeps data");

        // Reference: 0x2081cc + 0x1a25a0 (REA ev_4e9c96fe…), globals 0x379d64..66 = 8, 8, 10.
        SaveImage defaults;
        defaults.options()[0x100] = 0x55;
        defaults.resetOptions(languageForSystem(3));
        const std::uint8_t expected[] = {0x8a, 4, 0xff, 0x21, 10, 10, 8, 8, 0};
        require(std::equal(std::begin(expected), std::end(expected), defaults.options()) &&
                    defaults.options()[0x100] == 0,
                "option defaults");
        require(languageForSystem(0) == 1 && languageForSystem(7) == 12 && languageForSystem(11) == 1,
                "language mapping 0x21b650");
        // Reference: options menu 0x31af98 and draw code 0x1e4804, 0x1e9358 (manual disassembly).
        require(defaults.language() == std::uint8_t(Language::German) && !defaults.lastSlot() &&
                    defaults.subtitles() && !defaults.invertCamera() && defaults.soundVolume(1) == 10 &&
                    defaults.musicVolume(0) == 8,
                "option fields after reset");
        defaults.setInvertCamera(true);
        defaults.setSubtitles(false);
        defaults.setMusicVolume(1, 200);
        defaults.setSoundVolume(0, 3);
        defaults.options()[2] = 1;
        const std::uint8_t edited[] = {0x8a, 4, 1, 0x22, 3, 10, 8, 10};
        require(std::equal(std::begin(edited), std::end(edited), defaults.options()) && defaults.lastSlot() == 1u,
                "option field setters");
        threw = false;
        try { defaults.soundVolume(2); } catch (const std::out_of_range&) { threw = true; }
        require(threw, "volume slot range");

        Bytes data = image.bytes();
        data[6] = 29;
        require(SaveImage::fromBytes(data)->validate() == Status::Corrupt, "version mismatch");
        data = image.bytes();
        data[4] ^= 1;
        require(SaveImage::fromBytes(data)->validate() == Status::Corrupt, "size mismatch");
        data = image.bytes();
        data[8] = 'l';
        require(SaveImage::fromBytes(data)->validate() == Status::Unreadable, "bad magic");
        data = image.bytes();
        data.pop_back();
        require(!SaveImage::fromBytes(data), "short image");
        data = image.bytes();
        data.push_back(0xee);
        require(SaveImage::fromBytes(data)->validate() == Status::Ok, "trailing bytes ignored");

        auto dir = std::filesystem::temp_directory_path() / ("chase-save-test-" + std::to_string(::getpid()));
        std::filesystem::remove_all(dir);
        std::string path = (dir / "nested" / "savegame.dat").string();
        LoadResult missing = load(path);
        require(!missing.found && !missing.image, "missing file");

        store(path, image);
        require(std::filesystem::file_size(path) == kFileSize, "stored size");
        LoadResult loaded = load(path);
        require(loaded.found && loaded.status == Status::Ok && loaded.image->bytes() == image.bytes(), "round trip");
        int entries = 0;
        for (auto& entry : std::filesystem::directory_iterator(dir / "nested")) entries += entry.is_regular_file();
        require(entries == 1, "temporary file left behind");

        SaveImage unsealed = image;
        unsealed.options()[2] = 1;
        store(path, unsealed);
        require(load(path).status == Status::Ok, "store seals");

        writeFile(path, Bytes(100, 0));
        LoadResult truncated = load(path);
        require(truncated.found && truncated.status == Status::Unreadable && !truncated.image, "short file");

        std::filesystem::remove_all(dir);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "save tests passed\n";
    return 0;
}

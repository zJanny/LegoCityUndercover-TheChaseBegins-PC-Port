// Save request pieces and the slot row fields (docs/re/save-format.md, save writer entry). Synthetic data only.
#include "save/save_writer.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
using namespace chase::save;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void testPlayTime() {
    // Provenance: arithmetic of 0x2cc39c..0x2cc3e0 (reciprocals 0x136b06e7, 0x91a2b3c5, 0x88888889) read with llvm-objdump;
    // 30 ticks per second from 1 / 108000 = 0x136b06e7 / 2^45 and the pinned values below.
    require(formatPlayTime(0) == "00:00", "zero");
    require(formatPlayTime(29) == "00:00", "29 ticks");
    require(formatPlayTime(30) == "00:01", "30 ticks");
    require(formatPlayTime(10462) == "05:48", "user's save time 10462 ticks");
    require(formatPlayTime(107999) == "59:59", "one tick below an hour");
    require(formatPlayTime(108000) == "1:00:00", "an hour");
    require(formatPlayTime(108000 * 999 + 107999) == "999:59:59", "999 h 59 min 59 s");
    require(formatPlayTime(108000 * 1000) == "999:59:59", "1000 hours shows the cap text");
    require(formatPlayTime(0xffffffffu) == "999:59:59", "largest counter");
    for (std::uint64_t ticks = 0; ticks < std::uint64_t(108000) * 1000; ticks += 7919) {
        const PlayTime time = playTime(std::uint32_t(ticks));
        require(time.hours == ticks / 108000 && time.minutes == ticks % 108000 / 1800 && time.seconds == ticks % 1800 / 30,
                "split of " + std::to_string(ticks));
    }
}

void testPercent() {
    // Provenance: 0x2cc340..0x2cc384 and 0x2cc474 (formats 0x358a58, 0x358a60; separators 0x358a50, 0x358a54).
    require(formatPercent(0, 1) == "0.0%", "0");
    require(formatPercent(1, 1) == "0.0%", "1 is cleared to 0");
    require(formatPercent(2, 1) == "0.2%", "2");
    require(formatPercent(999, 1) == "99.9%", "999");
    require(formatPercent(1000, 1) == "100%", "1000");
    require(formatPercent(1000, 2) == "100%", "1000 in another language");
    require(formatPercent(2, 2) == "0,2%", "comma outside language 1");
    require(formatPercent(655, 9) == "65,5%", "655");
}

void testLocation() {
    require(locationLevel(0) == 1, "level byte 0 shows row 1");
    require(locationLevel(5) == 5, "level byte 5");
    require(locationLevel(-3) == -3, "negative byte stays negative");
    chase::assets::Table table;
    require(!locationTextHash(table, 1), "empty table");
}

void testSummary() {
    Bytes slot(kSlotSize, 0xaa);
    SlotSummaryInput input;
    input.studs = 0x0102030405060708ull;
    input.studTotal = 0x11223344;
    input.displayByte = 3;
    require(!writeSlotSummary(slot.data(), kSlotSize - 1, input, 7), "short slot accepted");
    require(writeSlotSummary(slot.data(), slot.size(), input, 0x0203), "summary");
    require(slot[kSlotStudsOffset] == 0x08 && slot[kSlotStudsOffset + 7] == 0x01, "studs bytes");
    require(slot[kSlotStudFlagsOffset] == ((0xaa & ~0x1c) | 0x0c), "flag bits 2..4");
    require(slot[kSlotStudTotalOffset] == 0x44 && slot[kSlotStudTotalOffset + 3] == 0x11, "stud total");
    require(slot[kSlotPermilleOffset] == 0x03 && slot[kSlotPermilleOffset + 1] == 0x02, "permille");
    require(slot[3] == 0xaa && slot[kSlotStudsOffset + 8] == 0xaa, "neighbours untouched");
    input.displayByte = 0xff;
    writeSlotSummary(slot.data(), slot.size(), input, 0);
    require(slot[kSlotStudFlagsOffset] == ((0xaa & ~0x1c) | 0x1c), "flag bits masked to 0x1c");
}

void testPlayTicks() {
    Bytes slot(kSlotSize, 0);
    slot[0] = 0xff;
    slot[1] = 0xff;
    addPlayTick(slot.data());
    require(slot[0] == 0 && slot[1] == 0 && slot[2] == 1 && playTicks({slot.data(), slot.size()}) == 0x10000, "carry");
}

void testCommit() {
    SaveImage image;
    image.seal();
    Bytes live(kSlotSize);
    for (std::size_t i = 0; i < live.size(); ++i) live[i] = std::uint8_t(i * 7 + 3);
    require(!commitSlot(image, -1, live), "slot -1");
    require(!commitSlot(image, int(kSlotCount), live), "slot 2");
    require(!commitSlot(image, 0, Bytes(kSlotSize - 1)), "short live slot");
    require(image.validate() == Status::Ok && image.slot(0)[0] == 0, "refused commits left the image alone");
    require(commitSlot(image, 1, live), "commit slot 1");
    require(image.validate() == Status::Ok, "sealed after commit");
    require(std::equal(live.begin(), live.end(), image.slot(1)) && image.slot(0)[0] == 0, "slot 1 only");
    require(image.lastSlot() == std::size_t(1), "options +2 holds the slot");
}

void testRequestWriter() {
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / ("chase-save-writer-" + std::to_string(getpid()));
    std::filesystem::create_directories(dir);
    const std::string path = (dir / "out" / "savegame.dat").string();
    Bytes live(kSlotSize, 0x5a);
    SaveRequestWriter off;
    require(!off.enabled() && !off.update(1, live), "disabled writer");
    SaveRequestWriter writer(path, SaveImage(), 0, std::nullopt);
    require(writer.enabled() && !writer.update(0, live) && !std::filesystem::exists(path), "no request, no file");
    require(writer.update(1, live) && writer.written() == 1, "first request");
    LoadResult loaded = load(path);
    require(loaded.found && loaded.status == Status::Ok && loaded.image, "stored file valid");
    require(std::equal(live.begin(), live.end(), loaded.image->slot(0)) && loaded.image->lastSlot() == std::size_t(0), "stored slot");
    require(!writer.update(1, live) && writer.written() == 1, "same count writes nothing");
    live[10] = 1;
    require(writer.update(2, live) && writer.written() == 2, "second request");
    require(load(path).image->slot(0)[10] == 1, "second write stored");
    std::filesystem::remove_all(dir);
}
}

int main() {
    try {
        testPlayTime();
        testPercent();
        testLocation();
        testSummary();
        testPlayTicks();
        testCommit();
        testRequestWriter();
    } catch (const std::exception& error) {
        std::cerr << "save-writer-test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "save-writer-test passed\n";
    return 0;
}

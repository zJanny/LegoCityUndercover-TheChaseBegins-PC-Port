// The draw list of the scene pass (src/render/draw_order.cpp, docs/re/rendering-draw-order.md). Provenance of the pinned values
// (20261027-graphics-D, 2026-10-27): REA (Ghidra 12.1.2) pseudo code and llvm-objdump of artifacts/game/game.arm.elf.
// - 0x25a248 builds the CRC tables at [0x3765dc + 4] (256 halfwords, reflected polynomial 0xa001) and [+8] (64 halfwords, polynomial
//   0xf01, masked to 12 bits); 0x24d20c steps the first, 0x115648 the second (six bits a round). The CRC-16 values are the CRC-16/USB
//   catalogue values (table[1] 0xc0c1, table[255] 0x4040, check of "123456789" 0xb4c8; 0x1157a0 / 0x11563c are the init and final
//   complement). The CRC-12 values were worked by hand from the table rule (table[1] = 0xa29) and the round order of 0x115648.
// - 0x115128: key high word = ((h >> 1) & 0x7ff) | (byte 5 & 0xf) << 11 | (~crc16(bytes 1..3) & 0xffff) << 15, low word = h << 31,
//   h the CRC-12 over 4 bytes of each texture slot.
// - 0x25688c: q = int((z - near) x 2^31 / (far - near) + 0.5) with z - near clamped to 0 .. (far - near) x 0.99999988, key = q ^ 0x7fffffff.
// - 0x10bbe8: opaque key = (material high, material low bit 31 | ~depth & 0x7fffffff), blended key = ((material high & 0x80000000) |
//   depth, material >> 31). 0x256734 appends front items and pushes back items down from the end; 0x2d0f84 compares the high word of the
//   64-bit difference; 0x259ebc runs the quicksort 0x259b94 for more than 10 items and then an insertion sort.
// - 0x10bbe8 / 0x188778: pass 2 for a record alpha (material alpha x node alpha / 255) below 255, a material alpha below 255 or material
//   byte 9 not 0; the record alpha below 255 makes the blend indices 4 / 5, a destination of 1 stays.
#include "render/draw_order.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase::render;

void require(bool ok, const std::string& what) {
    if (!ok) throw std::runtime_error(what);
}

std::uint16_t crc16Usb(const std::string& text) {
    std::uint16_t crc = 0xffff;
    for (const char c : text) crc = crc16Step(crc, std::uint8_t(c));
    return std::uint16_t(~crc);
}

void crcs() {
    require(crc16Step(0, 1) == 0xc0c1, "CRC-16 table entry 1");
    require(crc16Step(0, 255) == 0x4040, "CRC-16 table entry 255");
    require(crc16Usb("123456789") == 0xb4c8, "CRC-16/USB check value");
    const std::uint8_t one[4] = {1, 0, 0, 0}, zero[4] = {0, 0, 0, 0};
    require(crc12(0, one, 4) == 0xa0e, "CRC-12 of one texture word 1");
    require(crc12(0, zero, 4) == 0xa22, "CRC-12 of one texture word 0");
    require(crc12(5, one, 0) == 5, "CRC-12 of no bytes keeps the state");
}

void materialKeys() {
    std::uint8_t material[0x60] = {};
    require(materialSortKey(material, {}) == 0x1fc7000000000000ull, "material key of a zero material");
    material[2] = 2;
    material[3] = 1;
    material[5] = 3;
    require(materialSortKey(material, {}) == 0x4fa7180000000000ull, "material key of mode bytes 0 / 2 / 1 and alpha 3");
    std::uint8_t plain[0x60] = {};
    require(materialSortKey(plain, {1}) == (0x1fc7000000000000ull | (std::uint64_t(0x507) << 32)), "texture bits of the key");
}

void depthKeys() {
    require(depthSortKey(1.0f, 1.0f, 500.0f) == 0x7fffffffu, "depth key at the near plane");
    require(depthSortKey(0.5f, 1.0f, 500.0f) == 0x7fffffffu, "depth key before the near plane");
    require(depthSortKey(1.0f + 499.0f * 0.5f, 1.0f, 500.0f) == 0x3fffffffu, "depth key half way");
    require(depthSortKey(600.0f, 1.0f, 500.0f) == 0xffu, "depth key beyond the far plane");
    require(depthSortKey(10.0f, 1.0f, 500.0f) > depthSortKey(20.0f, 1.0f, 500.0f), "nearer is larger");
}

void itemKeys() {
    const std::uint64_t material = 0x4fa71800ffffffffull;
    require(opaqueItemKey(material, 0x7fffff00u) == 0x4fa71800800000ffull, "opaque key keeps material high and inverts the depth");
    require(blendedItemKey(material, 0x12345678u) == 0x123456789f4e3001ull, "blended key puts the depth high");
    require(compareItemKeys(1, 2) < 0 && compareItemKeys(2, 1) > 0 && compareItemKeys(7, 7) == 0, "low word order");
    require(compareItemKeys(0x100000000ull, 0xffffffffull) > 0, "high word order");
    require(compareItemKeys(0x0000000100000000ull, 0x0000000200000000ull) < 0, "high word order below");
}

void lists() {
    DrawList list;
    list.addOpaque(5, 0);
    list.addOpaque(5, 1);
    list.addOpaque(3, 2);
    list.addBlended(9, 3);
    list.addBlended(9, 4);
    list.addBlended(1, 5);
    const std::vector<DrawItem> opaque = list.sortedOpaque(), blended = list.sortedBlended();
    require(opaque.size() == 3 && opaque[0].ref == 2 && opaque[1].ref == 0 && opaque[2].ref == 1, "opaque order keeps the insertion order of ties");
    require(blended.size() == 3 && blended[0].ref == 5 && blended[1].ref == 4 && blended[2].ref == 3, "blended ties come in the reverse order");

    std::mt19937 random(7);
    for (int round = 0; round < 50; ++round) {
        std::vector<DrawItem> items;
        const int count = 1 + int(random() % 200);
        for (int i = 0; i < count; ++i) items.push_back({std::uint64_t(random() % 37) << (random() % 2 ? 32 : 0), std::size_t(i)});
        std::vector<DrawItem> sorted = items;
        sortDrawItems(sorted);
        require(std::is_sorted(sorted.begin(), sorted.end(), [](const DrawItem& a, const DrawItem& b) { return compareItemKeys(a.key, b.key) < 0; }),
                "sorted ascending");
        std::vector<std::size_t> refs;
        for (const DrawItem& item : sorted) refs.push_back(item.ref);
        std::sort(refs.begin(), refs.end());
        for (int i = 0; i < count; ++i) require(refs[std::size_t(i)] == std::size_t(i), "a permutation");
    }
}

void passes() {
    require(!blendedPass(255, 0, 255), "opaque material at full alpha");
    require(blendedPass(255, 0, 254), "node alpha below 255");
    require(blendedPass(128, 0, 255), "material alpha below 255");
    require(blendedPass(255, 1, 255), "blend destination one");
    require(drawAlpha(128, 128) == 64, "record alpha");
    require((drawBlendIndices(1, 0, 255) == std::array<std::uint8_t, 2>{1, 0}), "own blend at alpha 255");
    require((drawBlendIndices(1, 0, 200) == std::array<std::uint8_t, 2>{4, 5}), "source alpha below 255");
    require((drawBlendIndices(4, 1, 200) == std::array<std::uint8_t, 2>{4, 1}), "destination one stays");
}

}

int main() {
    try {
        crcs();
        materialKeys();
        depthKeys();
        itemKeys();
        lists();
        passes();
    } catch (const std::exception& error) {
        std::cerr << "draw-order-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "draw-order-test: ok\n";
    return 0;
}

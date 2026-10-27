#pragma once
// The draw list of the scene pass (docs/re/rendering-draw-order.md): the material sort key 0x115128, the depth key 0x25688c, the item keys
// the node draw 0x10bbe8 queues into list 0 (opaque parts at the front, blended parts at the back), the library sort 0x259ebc / 0x259b94
// with the comparator 0x2d0f84, and the pass and blend choice of 0x10bbe8 / 0x188778. No OpenGL; plain types only.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace chase::render {

// 0x24d20c over the reflected CRC-16 table of 0x25a248 (polynomial 0xa001).
std::uint16_t crc16Step(std::uint16_t crc, std::uint8_t byte);
// 0x115648: the 12-bit CRC (table polynomial 0xf01, 64 entries) of `length` bytes, chained from `state`.
std::uint16_t crc12(std::uint16_t state, const std::uint8_t* bytes, std::size_t length);

// 0x115128 over the 0x60-byte material record and one word per texture slot (slot +0 after the load 0x114860).
std::uint64_t materialSortKey(const std::uint8_t* material, const std::vector<std::uint32_t>& textureWords);

// 0x25688c: the view depth `z` between the camera's near and far plane (camera +0xc4 / +0xc8) as a 31-bit key, larger when nearer.
std::uint32_t depthSortKey(float z, float nearZ, float farZ);

// 0x10bbe8: the keys of list 0 (low word at item +0, high word at +4).
std::uint64_t opaqueItemKey(std::uint64_t materialKey, std::uint32_t depthKey);
std::uint64_t blendedItemKey(std::uint64_t materialKey, std::uint32_t depthKey);

// 0x2d0f84: the high word of the 64-bit difference, made non-zero when only the low word differs.
int compareItemKeys(std::uint64_t a, std::uint64_t b);

struct DrawItem {
    std::uint64_t key = 0;
    std::size_t ref = 0;
};
// 0x259ebc with 0x2d0f84: the quicksort pass 0x259b94 for more than 10 items, then an insertion sort.
void sortDrawItems(std::vector<DrawItem>& items);

// List 0 of 0x530634 for one frame: 0x256734 appends opaque items at the front and pushes blended items down from the end, 0x21d360 and
// 0x228d20 sort and hand out each half.
class DrawList {
public:
    void addOpaque(std::uint64_t key, std::size_t ref) { front_.push_back({key, ref}); }
    void addBlended(std::uint64_t key, std::size_t ref) { back_.push_back({key, ref}); }
    std::vector<DrawItem> sortedOpaque() const;
    std::vector<DrawItem> sortedBlended() const;

private:
    std::vector<DrawItem> front_, back_;
};

// 0x10bbe8: the alpha of the draw record (material colour alpha x node alpha / 255) and pass 2 for an alpha below 255, a material colour
// alpha below 255 or a blend destination index other than 0 (material byte 9).
std::uint8_t drawAlpha(std::uint8_t materialAlpha, std::uint8_t nodeAlpha);
bool blendedPass(std::uint8_t materialAlpha, std::uint8_t destinationIndex, std::uint8_t nodeAlpha);
// 0x188778: a record alpha below 255 replaces the blend indices (material bytes 8, 9) by 4 (source alpha) and 5 (one minus source alpha),
// a destination of 1 (one) stays.
std::array<std::uint8_t, 2> drawBlendIndices(std::uint8_t sourceIndex, std::uint8_t destinationIndex, std::uint8_t alpha);

}

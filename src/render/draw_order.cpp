#include "render/draw_order.hpp"

#include <algorithm>
#include <utility>

namespace chase::render {

namespace {

std::array<std::uint16_t, 256> buildCrc16Table() {
    std::array<std::uint16_t, 256> table{};
    for (unsigned c = 0; c < 256; ++c) {
        unsigned crc = 0, bits = c;
        for (int i = 0; i < 8; ++i) {
            crc = ((crc ^ bits) & 1) ? (crc >> 1) ^ 0xa001u : crc >> 1;
            bits >>= 1;
        }
        table[c] = std::uint16_t(crc);
    }
    return table;
}

std::array<std::uint16_t, 64> buildCrc12Table() {
    std::array<std::uint16_t, 64> table{};
    for (unsigned c = 0; c < 64; ++c) {
        unsigned crc = 0, bits = c;
        for (int i = 0; i < 6; ++i) {
            crc = ((crc ^ bits) & 1) ? (crc >> 1) ^ 0xf01u : crc >> 1;
            bits >>= 1;
        }
        table[c] = std::uint16_t(crc & 0x0fffu);
    }
    return table;
}

const std::array<std::uint16_t, 256>& crc16Table() {
    static const std::array<std::uint16_t, 256> table = buildCrc16Table();
    return table;
}

const std::array<std::uint16_t, 64>& crc12Table() {
    static const std::array<std::uint16_t, 64> table = buildCrc12Table();
    return table;
}

std::uint32_t crc12Round(std::uint32_t crc, std::uint32_t bits) {
    return crc12Table()[(crc ^ bits) & 63u] ^ ((crc << 20) >> 26);
}

bool greater(const DrawItem& a, const DrawItem& b) { return compareItemKeys(a.key, b.key) > 0; }
bool less(const DrawItem& a, const DrawItem& b) { return compareItemKeys(a.key, b.key) < 0; }

void quicksortPass(std::vector<DrawItem>& a) {
    struct Range {
        std::size_t start;
        int count;
    };
    std::vector<Range> stack{{0, int(a.size())}};
    while (!stack.empty()) {
        std::size_t lo = stack.back().start;
        int n = stack.back().count;
        stack.pop_back();
        for (;;) {
            const std::size_t mid = lo + std::size_t(n >> 1);
            const std::size_t hi = lo + std::size_t(n - 1);
            if (greater(a[lo], a[mid])) std::swap(a[lo], a[mid]);
            if (greater(a[mid], a[hi])) {
                if (greater(a[lo], a[hi])) std::swap(a[lo], a[hi]);
                std::swap(a[mid], a[hi]);
            }
            const std::size_t pivot = hi - 1;
            std::swap(a[mid], a[pivot]);
            std::size_t i = lo, j = pivot;
            for (;;) {
                do ++i;
                while (less(a[i], a[pivot]));
                do --j;
                while (greater(a[j], a[pivot]));
                if (j <= i) break;
                std::swap(a[i], a[j]);
            }
            std::swap(a[i], a[pivot]);
            const int left = int(i - lo) - 1;
            const int right = n - left - 1;
            if (left <= right) {
                if (left > 10) {
                    stack.push_back({i, right});
                    n = left;
                } else if (right > 10) {
                    lo = i;
                    n = right;
                } else {
                    break;
                }
            } else {
                if (right > 10) {
                    stack.push_back({lo, left});
                    lo = i;
                    n = right;
                } else if (left > 10) {
                    n = left;
                } else {
                    break;
                }
            }
        }
    }
}

}

std::uint16_t crc16Step(std::uint16_t crc, std::uint8_t byte) {
    return std::uint16_t(crc16Table()[(crc ^ byte) & 0xffu] ^ ((crc >> 8) & 0xffu));
}

std::uint16_t crc12(std::uint16_t state, const std::uint8_t* bytes, std::size_t length) {
    std::uint32_t crc = ~std::uint32_t(state) & 0xfffu;
    if (length == 0) return std::uint16_t(~crc & 0xfffu);
    std::uint32_t pending = 0;
    unsigned bits = 0;
    for (std::size_t k = 0; k < length; ++k) {
        pending |= std::uint32_t(bytes[k]) << bits;
        crc = crc12Round(crc, pending);
        bits += 2;
        pending >>= 6;
        if (bits >= 6) {
            crc = crc12Round(crc, pending);
            pending >>= 6;
            bits -= 2;
        }
    }
    if (bits != 0) crc = crc12Round(crc, pending);
    return std::uint16_t(~crc & 0xfffu);
}

std::uint64_t materialSortKey(const std::uint8_t* material, const std::vector<std::uint32_t>& textureWords) {
    std::uint16_t crc = 0xffff;
    for (int i = 1; i <= 3; ++i) crc = crc16Step(crc, material[i]);
    const std::uint32_t modes = ~std::uint32_t(crc) & 0xffffu;
    std::uint32_t textures = 0;
    for (const std::uint32_t word : textureWords) {
        const std::uint8_t bytes[4] = {std::uint8_t(word), std::uint8_t(word >> 8), std::uint8_t(word >> 16), std::uint8_t(word >> 24)};
        textures = crc12(std::uint16_t(textures), bytes, 4);
    }
    const std::uint32_t low = textures << 31;
    const std::uint32_t high = ((textures >> 1) & 0x7ffu) | (std::uint32_t(material[5] & 0xf) << 11) | (modes << 15);
    return (std::uint64_t(high) << 32) | low;
}

std::uint32_t depthSortKey(float z, float nearZ, float farZ) {
    const float range = farZ - nearZ;
    const float limit = range * 0.99999988f;
    float d = z - nearZ;
    if (!(d < limit))
        d = limit;
    else if (!(d > 0.0f))
        d = 0.0f;
    const std::int32_t scaled = std::int32_t(d * 2147483648.0f / range + 0.5f);
    return std::uint32_t(scaled) ^ 0x7fffffffu;
}

std::uint64_t opaqueItemKey(std::uint64_t materialKey, std::uint32_t depthKey) {
    const std::uint32_t low = std::uint32_t(materialKey) & 0x80000000u;
    return (materialKey & 0xffffffff00000000ull) | low | (~depthKey & 0x7fffffffu);
}

std::uint64_t blendedItemKey(std::uint64_t materialKey, std::uint32_t depthKey) {
    const std::uint32_t high = std::uint32_t(materialKey >> 32);
    const std::uint32_t low = std::uint32_t(materialKey >> 31);
    return (std::uint64_t((high & 0x80000000u) | (depthKey & 0x7fffffffu)) << 32) | low;
}

int compareItemKeys(std::uint64_t a, std::uint64_t b) {
    const std::uint64_t difference = a - b;
    const std::uint32_t low = std::uint32_t(difference), high = std::uint32_t(difference >> 32);
    const std::uint32_t lowBits = (low | (low >> 16) | (high << 16)) & 0xffffu;
    return int(std::int32_t(high | lowBits));
}

void sortDrawItems(std::vector<DrawItem>& items) {
    if (items.empty()) return;
    if (items.size() > 10) quicksortPass(items);
    for (std::size_t current = 0; current + 1 < items.size(); ++current) {
        const std::size_t next = current + 1;
        std::ptrdiff_t slot = std::ptrdiff_t(current);
        while (slot >= 0 && greater(items[std::size_t(slot)], items[next])) --slot;
        const DrawItem moved = items[next];
        for (std::size_t k = next; k > std::size_t(slot + 1); --k) items[k] = items[k - 1];
        items[std::size_t(slot + 1)] = moved;
    }
}

std::vector<DrawItem> DrawList::sortedOpaque() const {
    std::vector<DrawItem> items = front_;
    sortDrawItems(items);
    return items;
}

std::vector<DrawItem> DrawList::sortedBlended() const {
    std::vector<DrawItem> items(back_.rbegin(), back_.rend());
    sortDrawItems(items);
    return items;
}

std::uint8_t drawAlpha(std::uint8_t materialAlpha, std::uint8_t nodeAlpha) {
    return std::uint8_t(unsigned(materialAlpha) * unsigned(nodeAlpha) / 255u);
}

bool blendedPass(std::uint8_t materialAlpha, std::uint8_t destinationIndex, std::uint8_t nodeAlpha) {
    return drawAlpha(materialAlpha, nodeAlpha) < 255 || materialAlpha != 255 || destinationIndex != 0;
}

std::array<std::uint8_t, 2> drawBlendIndices(std::uint8_t sourceIndex, std::uint8_t destinationIndex, std::uint8_t alpha) {
    if (alpha == 255) return {sourceIndex, destinationIndex};
    return {4, std::uint8_t(destinationIndex == 1 ? 1 : 5)};
}

}

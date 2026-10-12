#pragma once
#include "assets.hpp"
#include <algorithm>
#include <cstring>
#include <string>

namespace chase::assets::detail {
struct BlockReader {
    const Bytes& data;
    PoolCursor cursor;
    explicit BlockReader(const Bytes& bytes) : data(bytes), cursor(bytes) {}
    Bytes next() {
        const Block* block = cursor.next();
        if (!block) return {};
        return Bytes(data.begin() + std::ptrdiff_t(block->offset),
                     data.begin() + std::ptrdiff_t(block->offset + block->size));
    }
    void skip() { cursor.next(); }
};

inline std::string cString(const Bytes& block) {
    return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
}

inline float readF32(const Bytes& data, std::size_t offset) {
    std::uint32_t bits = readU32(data, offset);
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
} // namespace chase::assets::detail

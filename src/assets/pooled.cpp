#include "assets.hpp"
#include <stdexcept>

namespace chase::assets {
std::vector<Block> parsePooled(const Bytes& data) {
    if (data.size() < 8) throw std::runtime_error("truncated resource header");
    std::uint32_t first = readU32(data, 0), headerBytes = readU32(data, 4);
    std::size_t words = first >> 8;
    if ((first & 0xff) || !words || 4 * words > headerBytes || headerBytes > data.size() - 8)
        throw std::runtime_error("not a pooled resource header");
    std::vector<std::pair<Pool, std::size_t>> table;
    bool havePool = false;
    Pool pool = Pool::M14;
    for (std::size_t i = 0; i < words; ++i) {
        auto word = static_cast<std::int32_t>(readU32(data, 8 + 4 * i));
        if (word == -14 || word == -15 || word == -16) {
            pool = static_cast<Pool>(word);
            havePool = true;
        } else if (!havePool || word < 0) {
            throw std::runtime_error("block size before pool marker or negative size");
        } else {
            table.emplace_back(pool, static_cast<std::size_t>(word));
        }
    }
    std::vector<Block> blocks;
    std::size_t cursor = 8 + 4 * words;
    // -16 blocks live in the header; -15 and -14 data follow in table order.
    for (int pass = 0; pass < 2; ++pass) {
        for (const auto& [blockPool, size] : table) {
            if ((blockPool == Pool::M16) != (pass == 0)) continue;
            if (size > data.size() - cursor) throw std::runtime_error("block outside file");
            blocks.push_back({blockPool, cursor, size});
            cursor += size;
        }
        if (pass == 0 && cursor != 8 + std::size_t(headerBytes))
            throw std::runtime_error("static (-16) data does not fill header");
    }
    if (cursor != data.size()) throw std::runtime_error("blocks do not end at end of file");
    return blocks;
}

PoolCursor::PoolCursor(const Bytes& data) {
    for (const auto& block : parsePooled(data)) queues_[block.pool].push_back(block);
}

void PoolCursor::pop() {
    if (stack_.empty()) throw std::runtime_error("pool stack underflow");
    stack_.pop_back();
}

const Block* PoolCursor::next() {
    if (stack_.empty()) throw std::runtime_error("no current pool");
    Pool pool = stack_.back();
    auto& queue = queues_[pool];
    auto& index = cursor_[pool];
    if (index >= queue.size()) throw std::runtime_error("pool exhausted");
    const Block& block = queue[index++];
    return block.size ? &block : nullptr;
}

bool PoolCursor::exhausted() const {
    for (const auto& [pool, queue] : queues_) {
        auto at = cursor_.find(pool);
        if ((at == cursor_.end() ? 0 : at->second) != queue.size()) return false;
    }
    return true;
}
} // namespace chase::assets

#include "assets/pregen_traffic.hpp"
#include <cstring>
#include <stdexcept>

namespace chase::assets {
namespace {
constexpr std::size_t kPointBytes = 24;
constexpr std::size_t kCellBytes = 20;
constexpr int kMaxDepth = 16;

class Reader {
public:
    Reader(const std::vector<std::uint8_t>& data, const char* what) : data_(data), what_(what) {}

    void need(std::size_t size) const {
        if (size > data_.size() - offset_)
            throw std::runtime_error(std::string(what_) + ": truncated at byte " + std::to_string(offset_));
    }
    std::uint16_t u16() {
        need(2);
        const std::uint16_t value = std::uint16_t(data_[offset_] | (data_[offset_ + 1] << 8));
        offset_ += 2;
        return value;
    }
    std::uint32_t u32() {
        need(4);
        const std::uint32_t value = std::uint32_t(data_[offset_]) | std::uint32_t(data_[offset_ + 1]) << 8 |
                                    std::uint32_t(data_[offset_ + 2]) << 16 | std::uint32_t(data_[offset_ + 3]) << 24;
        offset_ += 4;
        return value;
    }
    float f32() {
        const std::uint32_t bits = u32();
        float value;
        std::memcpy(&value, &bits, sizeof value);
        return value;
    }
    void skip(std::size_t size) {
        need(size);
        offset_ += size;
    }
    std::size_t remaining() const { return data_.size() - offset_; }
    const char* what() const { return what_; }

private:
    const std::vector<std::uint8_t>& data_;
    const char* what_;
    std::size_t offset_ = 0;
};

const char* kindName(PregenSpawnKind kind) {
    return kind == PregenSpawnKind::Traffic ? "pregen traffic file" : "pregen pedestrians file";
}

std::int32_t readCell(Reader& r, PregenSpawns& spawns, std::size_t capacity, int depth) {
    if (depth > kMaxDepth) throw std::runtime_error(std::string(r.what()) + ": cell tree deeper than 16 levels");
    const std::int32_t index = std::int32_t(spawns.cells.size());
    spawns.cells.emplace_back();
    PregenSpawnCell cell;
    cell.halfSize = r.u16();
    const std::uint16_t count = r.u16();
    cell.centreX = r.f32();
    cell.centreZ = r.f32();
    cell.storedListPointer = r.u32();
    cell.storedChildPointer = r.u32();
    if (count) {
        if (count > capacity)
            throw std::runtime_error(std::string(r.what()) + ": cell list of " + std::to_string(count) +
                                     " points exceeds the reader's " + std::to_string(capacity));
        cell.points.resize(count);
        for (auto& point : cell.points) {
            point = r.u16();
            if (point >= spawns.points.size())
                throw std::runtime_error(std::string(r.what()) + ": cell list names point " + std::to_string(point) +
                                         " of " + std::to_string(spawns.points.size()));
        }
    } else if (cell.storedChildPointer) {
        r.skip(4 * kCellBytes);
        for (std::size_t k = 0; k < 4; ++k) cell.children[k] = readCell(r, spawns, capacity, depth + 1);
    }
    spawns.cells[std::size_t(index)] = std::move(cell);
    return index;
}
}

std::size_t pregenSpawnCellCapacity(PregenSpawnKind kind) { return kind == PregenSpawnKind::Traffic ? 200 : 256; }

std::string pregenSpawnPath(PregenSpawnKind kind, const std::string& levelName) {
    return kind == PregenSpawnKind::Traffic ? "Pregen/Traffic/" + levelName + "_Traffic.bin"
                                            : "Pregen/Pedestrians/" + levelName + "_Pedestrians.bin";
}

PregenSpawns parsePregenSpawns(const std::vector<std::uint8_t>& data, PregenSpawnKind kind) {
    Reader r(data, kindName(kind));
    PregenSpawns spawns;
    const std::uint32_t count = r.u32();
    if (count > r.remaining() / kPointBytes)
        throw std::runtime_error(std::string(r.what()) + ": " + std::to_string(count) + " points do not fit");
    spawns.points.resize(count);
    for (auto& point : spawns.points) {
        for (auto& axis : point.position) axis = r.f32();
        point.storedPathPointer = r.u32();
        point.pathHash = r.u32();
        point.pathPosition = r.f32();
    }
    readCell(r, spawns, pregenSpawnCellCapacity(kind), 0);
    if (r.remaining())
        throw std::runtime_error(std::string(r.what()) + ": " + std::to_string(r.remaining()) +
                                 " bytes after the cell tree");
    return spawns;
}

std::int32_t findPregenSpawnCell(const PregenSpawns& spawns, float x, float z) {
    if (spawns.cells.empty()) return -1;
    std::int32_t index = 0;
    for (;;) {
        const PregenSpawnCell& cell = spawns.cells[std::size_t(index)];
        if (!cell.points.empty()) return index;
        if (cell.children[0] < 0) return -1;
        const PregenSpawnCell& low = spawns.cells[std::size_t(cell.children[0])];
        if (!(low.centreX + float(low.halfSize) < x)) {
            index = !(low.centreZ + float(low.halfSize) < z) ? cell.children[0] : cell.children[2];
        } else {
            const PregenSpawnCell& high = spawns.cells[std::size_t(cell.children[1])];
            index = !(high.centreZ + float(high.halfSize) < z) ? cell.children[1] : cell.children[3];
        }
    }
}

}

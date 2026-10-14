#include "block_reader.hpp"
#include <stdexcept>

namespace chase::assets {
namespace {
using Reader = detail::BlockReader;
using detail::readF32;

constexpr std::size_t gridRecord = 0x3c;
constexpr std::size_t pathRecord = 0x20;

bool padded(std::size_t size, std::size_t wanted) {
    return size == wanted || (size % 4 == 0 && size >= wanted && size < wanted + 4);
}
}

NavMesh parseNav(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M14);
    NavMesh nav;
    Bytes grids = r.next();
    if (grids.size() % gridRecord) throw std::runtime_error("nav grid block is not a multiple of 0x3c");
    for (std::size_t i = 0; i < grids.size() / gridRecord; ++i) {
        std::size_t at = i * gridRecord;
        NavGrid grid;
        grid.x = readU16(grids, at);
        grid.height = readU16(grids, at + 2);
        grid.z = readU16(grids, at + 4);
        grid.width = readU16(grids, at + 6);
        grid.heightExtent = readU16(grids, at + 8);
        grid.depth = readU16(grids, at + 10);
        grid.flags = readU16(grids, at + 0x34);
        for (std::size_t k = 0; k < 16; ++k) {
            std::uint16_t link = readU16(grids, at + 0x14 + 2 * k);
            grid.linkSlots[k] = link;
            if (link != 0xffff) grid.links.push_back(link);
        }
        Bytes cells = r.next();
        std::size_t count = std::size_t(grid.width) * grid.depth;
        if (!padded(cells.size(), count)) throw std::runtime_error("nav cell block size mismatch");
        grid.cells.assign(cells.begin(), cells.begin() + std::ptrdiff_t(count));
        if (std::size_t extra = readU16(grids, at + 0x34) & 0x3fffu) {
            Bytes block = r.next();
            if (block.size() != extra * 4) throw std::runtime_error("nav extra block size mismatch");
            for (std::size_t k = 0; k < extra; ++k)
                grid.extras.push_back({block[4 * k], block[4 * k + 1], block[4 * k + 2], block[4 * k + 3]});
        }
        nav.grids.push_back(std::move(grid));
    }
    Bytes paths = r.next();
    if (paths.size() % pathRecord) throw std::runtime_error("nav path block is not a multiple of 0x20");
    for (std::size_t i = 0; i < paths.size() / pathRecord; ++i) {
        std::size_t at = i * pathRecord;
        NavPath path;
        for (std::size_t k = 0; k < 3; ++k) {
            path.position[k] = readF32(paths, at + 4 * k);
            path.extent[k] = readF32(paths, at + 12 + 4 * k);
        }
        std::size_t count = readU32(paths, at + 0x18);
        Bytes indices = r.next();
        if (!padded(indices.size(), count * 2)) throw std::runtime_error("nav path index block size mismatch");
        for (std::size_t k = 0; k < count; ++k) path.indices.push_back(readU16(indices, 2 * k));
        nav.paths.push_back(std::move(path));
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed nav blocks");
    return nav;
}
} // namespace chase::assets

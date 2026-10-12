#include "assets.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace chase::assets {
namespace {
constexpr std::uint32_t maxEntry = 64u * 1024 * 1024;
constexpr std::uint32_t maxEntries = 1u << 20;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}
std::string trim(const std::string& text) {
    auto first = text.find_first_not_of(" \t\r\n");
    auto last = text.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
}
}

std::uint32_t readU32(const Bytes& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4) throw std::runtime_error("u32 read out of bounds");
    return std::uint32_t(data[offset]) | std::uint32_t(data[offset + 1]) << 8 |
           std::uint32_t(data[offset + 2]) << 16 | std::uint32_t(data[offset + 3]) << 24;
}
std::uint16_t readU16(const Bytes& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 2) throw std::runtime_error("u16 read out of bounds");
    return std::uint16_t(data[offset] | data[offset + 1] << 8);
}

Bytes decompressRefPack(const std::uint8_t* data, std::size_t size, std::size_t limit) {
    Bytes out;
    out.reserve(limit);
    std::size_t pos = 0;
    auto take = [&]() -> std::uint8_t {
        if (pos >= size) throw std::runtime_error("truncated compressed chunk");
        return data[pos++];
    };
    auto literals = [&](std::size_t count) {
        if (count > size - pos) throw std::runtime_error("truncated compressed chunk");
        out.insert(out.end(), data + pos, data + pos + count);
        pos += count;
    };
    auto match = [&](std::size_t distance, std::size_t length) {
        if (distance > out.size()) throw std::runtime_error("compressed reference before chunk start");
        std::size_t start = out.size() - distance;
        for (std::size_t i = 0; i < length; ++i) out.push_back(out[start + i]);
    };
    for (;;) {
        if (out.size() > limit) throw std::runtime_error("compressed chunk exceeds block size");
        std::uint8_t b = take();
        if (!(b & 0x80)) {
            std::uint8_t b1 = take();
            literals((b >> 2) & 3);
            match(((b & 3u) << 8 | b1) + 1, ((b >> 4) & 7u) + 3);
        } else if (!(b & 0x40)) {
            std::uint8_t b1 = take(), b2 = take();
            literals(b1 >> 6);
            match(((b1 & 0x3fu) << 8 | b2) + 1, (b & 0x3fu) + 4);
        } else if (!(b & 0x20)) {
            std::uint8_t b1 = take(), b2 = take(), b3 = take();
            literals((b >> 3) & 3);
            match(((b & 1u) << 16 | std::size_t(b1) << 8 | b2) + 1, ((b & 6u) << 7) + b3 + 5);
        } else {
            std::size_t run = (b & 0x1fu) * 4 + 4;
            if (run > 112) {
                literals(b & 3);
                break;
            }
            literals(run);
        }
    }
    if (out.size() > limit) throw std::runtime_error("compressed chunk exceeds block size");
    if (pos != size) throw std::runtime_error("trailing bytes after compressed chunk");
    return out;
}

FuseArchive::FuseArchive(const std::string& path) : file_(path, std::ios::binary) {
    if (!file_) throw std::runtime_error("cannot open archive: " + path);
    file_.seekg(0, std::ios::end);
    fileSize_ = static_cast<std::uint64_t>(file_.tellg());
    Bytes header = readAt(0, 20);
    if (std::memcmp(header.data(), "FUSE", 4) != 0) throw std::runtime_error("not a FUSE archive");
    std::uint32_t count = readU32(header, 8), extra = readU32(header, 12), table = readU32(header, 16);
    if (extra) throw std::runtime_error("second FUSE table not supported");
    if (count > maxEntries) throw std::runtime_error("too many FUSE entries");
    Bytes directory = readAt(table, std::size_t(count) * 12);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t packed = readU32(directory, i * 12 + 8);
        FuseEntry entry{readU32(directory, i * 12), readU32(directory, i * 12 + 4), packed >> 5, packed & 0x1f};
        if (!byCrc_.emplace(entry.crc, entries_.size()).second) throw std::runtime_error("duplicate FUSE name CRC");
        entries_.push_back(entry);
    }
}

Bytes FuseArchive::readAt(std::uint64_t offset, std::size_t size) {
    if (offset > fileSize_ || fileSize_ - offset < size) throw std::runtime_error("range outside archive");
    Bytes data(size);
    file_.clear();
    file_.seekg(static_cast<std::streamoff>(offset));
    file_.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    if (!file_) throw std::runtime_error("truncated archive");
    return data;
}

Bytes FuseArchive::read(const FuseEntry& entry) {
    if (entry.size > maxEntry) throw std::runtime_error("entry exceeds size cap");
    unsigned codec = entry.flags & 3;
    std::size_t block = std::size_t(0x8000) << ((entry.flags >> 2) & 7);
    if (codec == 0) return readAt(entry.offset, entry.size);
    if (codec == 3) throw std::runtime_error("codec 3 (decoder 0x12d50c) not recovered");
    Bytes out;
    out.reserve(entry.size);
    std::uint64_t cursor = entry.offset;
    while (out.size() < entry.size) {
        std::uint32_t length = readU32(readAt(cursor, 4), 0);
        Bytes chunk = readAt(cursor + 4, length);
        cursor += 4 + std::uint64_t(length);
        std::size_t expected = std::min<std::size_t>(block, entry.size - out.size());
        Bytes data = codec == 1 ? decompressRefPack(chunk.data(), chunk.size(), expected) : std::move(chunk);
        if (data.size() != expected) throw std::runtime_error("chunk decoded to the wrong size");
        out.insert(out.end(), data.begin(), data.end());
    }
    return out;
}

void FuseArchive::loadNames(const std::string& csvPath) {
    std::ifstream csv(csvPath);
    if (!csv) throw std::runtime_error("cannot open name index: " + csvPath);
    std::string line;
    std::getline(csv, line);
    while (std::getline(csv, line)) {
        std::stringstream row(line);
        std::string crc, name;
        if (!std::getline(row, crc, ',') || !std::getline(row, name, ',')) continue;
        names_[lower(trim(name))] = static_cast<std::uint32_t>(std::stoul(trim(crc), nullptr, 16));
    }
}

const FuseEntry* FuseArchive::find(const std::string& name) const {
    auto named = names_.find(lower(name));
    if (named == names_.end()) return nullptr;
    auto index = byCrc_.find(named->second);
    return index == byCrc_.end() ? nullptr : &entries_[index->second];
}
} // namespace chase::assets

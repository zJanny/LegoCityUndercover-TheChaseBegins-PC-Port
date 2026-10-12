#include "assets.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace chase::assets {
namespace {
constexpr std::uint32_t initialPosition = 0x86166f20;     // nameHash("initialposition")
constexpr std::uint32_t initialOrientation = 0xa3b9aa7f;  // nameHash("initialorientation")
constexpr std::uint32_t meshName = 0xacd1ebb8;            // nameHash("meshname")

struct Reader {
    const Bytes& data;
    PoolCursor cursor;
    explicit Reader(const Bytes& bytes) : data(bytes), cursor(bytes) {}
    Bytes next() {
        const Block* block = cursor.next();
        if (!block) return {};
        return Bytes(data.begin() + std::ptrdiff_t(block->offset),
                     data.begin() + std::ptrdiff_t(block->offset + block->size));
    }
    void skip() { cursor.next(); }
};

std::string cString(const Bytes& block) {
    return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
}
void need(const Bytes& block, std::size_t size) {
    if (block.size() < size) throw std::runtime_error("level block too small");
}

// 0x208ae0: parameter records of 12 bytes; out-of-line data depends on the type.
std::vector<LevelParam> readParams(Reader& r, const Bytes& block, std::size_t count) {
    need(block, count * 12);
    std::vector<LevelParam> params(count);
    for (std::size_t i = 0; i < count; ++i) {
        LevelParam& p = params[i];
        p.hash = readU32(block, i * 12);
        p.type = readU16(block, i * 12 + 6);
        p.value = readU32(block, i * 12 + 8);
        if (p.type == 1 || p.type == 2 || p.type == 3 || p.type == 6 || (p.type == 4 && p.value))
            p.blocks.push_back(r.next());
        else if (p.type == 8) {
            p.blocks.push_back(r.next());
            p.blocks.push_back(r.next());
        }
    }
    return params;
}
}

std::uint32_t nameHash(const std::string& name) {
    static const auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    std::uint32_t crc = 0xffffffffu;
    for (unsigned char ch : name) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<unsigned char>(ch + 32);
        crc = table[(ch ^ crc) & 0xff] ^ (crc >> 8);
    }
    return ~crc;
}

Level parseLevel(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M15);
    Bytes head = r.next();
    need(head, 0x10);
    std::size_t n30 = head[1], n10 = readU16(head, 2), n14 = readU16(head, 4), n18 = readU16(head, 6),
                n1c = readU16(head, 8), n20 = readU16(head, 10), n24 = readU16(head, 12), n2c = readU16(head, 14);
    readParams(r, r.next(), n10);
    readParams(r, r.next(), n14);
    Bytes s18 = r.next();
    need(s18, n18 * 0x1c);
    for (std::size_t i = 0; i < n18; ++i) {
        r.skip();
        r.skip();
        for (std::uint32_t k = 0; k < readU32(s18, i * 0x1c + 0x14); ++k) r.skip();
        r.cursor.push(Pool::M16);
        for (std::uint16_t k = 0; k < readU16(s18, i * 0x1c + 6); ++k) r.skip();
        r.cursor.pop();
    }
    r.skip();  // s1c
    for (std::size_t i = 0; i < n1c; ++i) {
        r.skip();
        r.skip();
        r.cursor.push(Pool::M16);
        for (int k = 0; k < 3; ++k) r.skip();
        r.cursor.pop();
    }
    Bytes s20 = r.next();
    need(s20, n20 * 0x28);
    for (std::size_t i = 0; i < n20; ++i) {
        if (readU32(s20, i * 0x28 + 4)) r.skip();
        r.skip();
        if (readU32(s20, i * 0x28 + 0x10)) {
            r.skip();
            r.skip();
        }
    }
    Bytes s24 = r.next();
    need(s24, n24 * 0x28);
    for (std::size_t i = 0; i < n24; ++i) {
        r.skip();
        if (readU32(s24, i * 0x28 + 4)) r.skip();
    }
    r.skip();  // +0x28
    Bytes s2c = r.next();
    need(s2c, n2c * 0x18);
    Level level;
    level.objects.resize(n2c);
    for (std::size_t i = 0; i < n2c; ++i) {
        LevelObject& obj = level.objects[i];
        obj.instance = cString(r.next());
        obj.room = cString(r.next());
        obj.params = readParams(r, r.next(), s2c[i * 0x18 + 0xd]);
        auto second = readParams(r, r.next(), s2c[i * 0x18 + 0xe]);
        obj.params.insert(obj.params.end(), second.begin(), second.end());
    }
    Bytes s34 = r.next();
    if (!s34.empty()) {
        r.skip();
        for (std::uint32_t k = 0; k < readU32(s34, 0); ++k) r.skip();
    }
    r.cursor.push(Pool::M16);
    for (auto& obj : level.objects) obj.className = cString(r.next());
    r.cursor.pop();
    if (n30) {
        Bytes s30 = r.next();
        need(s30, n30 * 0x28);
        for (std::size_t i = 0; i < n30; ++i) {
            r.skip();
            for (std::uint16_t k = 0; k < readU16(s30, i * 0x28); ++k) r.skip();
            r.skip();
        }
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed level blocks");
    return level;
}

std::vector<Placement> placements(const Level& level) {
    std::vector<Placement> out;
    for (const auto& obj : level.objects) {
        Placement p;
        p.instance = obj.instance;
        p.room = obj.room;
        for (const auto& param : obj.params) {
            if (param.blocks.empty() || param.blocks[0].empty()) continue;
            const Bytes& b = param.blocks[0];
            if (param.hash == meshName && param.type == 1) p.mesh = cString(b);
            float* target = param.hash == initialPosition ? p.position
                          : param.hash == initialOrientation ? p.orientation : nullptr;
            if (target && param.type == 2 && b.size() >= 12) std::memcpy(target, b.data(), 12);
        }
        if (!p.mesh.empty()) out.push_back(p);
    }
    return out;
}

std::string meshModelPath(const std::string& mesh) {
    std::string m = mesh;
    std::replace(m.begin(), m.end(), '\\', '/');
    std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    if (m.find('/') == std::string::npos) m += "/" + m;
    return "models/" + m + ".bfnmdl";
}
} // namespace chase::assets

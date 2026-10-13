#include "assets.hpp"
#include <cstring>
#include <stdexcept>

namespace chase::assets {
namespace {
const Block& required(PoolCursor& cursor, const char* what) {
    const Block* block = cursor.next();
    if (!block) throw std::runtime_error(std::string("particle defs: missing ") + what);
    return *block;
}

std::uint32_t word(const Bytes& data, PoolCursor& cursor, const char* what) {
    const Block& block = required(cursor, what);
    if (block.size < 4) throw std::runtime_error(std::string("particle defs: short ") + what);
    return readU32(data, block.offset);
}
}

std::uint32_t pjwHash(const std::string& text) {
    std::uint32_t hash = 0;
    for (unsigned char c : text) {
        if (c >= 'A' && c <= 'Z') c = static_cast<unsigned char>(c + 32);
        hash = (hash << 4) + static_cast<std::int8_t>(c);
        std::uint32_t high = hash & 0xf0000000u;
        if (high) hash = (hash ^ (high >> 24)) & ~0xf0000000u;
    }
    return hash;
}

ParticleDefs parseParticleDefs(const Bytes& data) {
    PoolCursor cursor(data);
    ParticleDefs defs;
    cursor.push(Pool::M15);
    defs.unknown0 = word(data, cursor, "header word 0");
    defs.unknown1 = word(data, cursor, "header word 1");
    std::uint32_t textureCount = word(data, cursor, "texture count");
    std::uint32_t defCount = word(data, cursor, "definition count");
    defs.unknown4 = word(data, cursor, "header word 4");
    std::uint32_t scaleBits = word(data, cursor, "header float");
    std::memcpy(&defs.scale, &scaleBits, sizeof defs.scale);
    if (defs.scale == 0.0f) defs.scale = 1.0f;
    cursor.pop();

    cursor.push(Pool::M16);
    for (std::uint32_t i = 0; i < textureCount; ++i) {
        const Block& block = required(cursor, "texture name");
        std::string name;
        for (std::size_t at = block.offset; at < block.offset + block.size && data[at]; ++at) name += char(data[at]);
        defs.textures.push_back(name);
    }
    cursor.pop();

    cursor.push(Pool::M14);
    const Block& records = required(cursor, "definition data");
    cursor.pop();
    if (records.size != std::size_t(defCount) * kParticleDefSize)
        throw std::runtime_error("particle defs: data size is not count x record size");
    for (std::uint32_t i = 0; i < defCount; ++i) {
        ParticleDef def;
        std::size_t at = records.offset + std::size_t(i) * kParticleDefSize;
        std::memcpy(def.raw.data(), data.data() + at, kParticleDefSize);
        def.textureHash = readU32(data, at + 0x2c);
        for (std::size_t t = 0; t < defs.textures.size(); ++t)
            if (pjwHash(defs.textures[t]) == def.textureHash) def.texture = int(t);
        defs.defs.push_back(def);
    }
    if (!cursor.exhausted()) throw std::runtime_error("particle defs: unread blocks");
    return defs;
}
} // namespace chase::assets

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

float readFloat(const Bytes& data, std::size_t offset) {
    std::uint32_t bits = readU32(data, offset);
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
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
    defs.maxEmitters = word(data, cursor, "emitter limit");
    defs.maxParticles = word(data, cursor, "particle limit");
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
        def.flags = readU32(data, at + 0x168);
        def.seed = readU32(data, at + 0x84);
        def.particles = data[at + 0x175];
        def.startAlpha = data[at + 0x134];
        def.atlasColumns = data[at + 0x178];
        def.atlasRows = data[at + 0x179];
        def.frameMode = data[at + 0x176];
        def.flipModes = data[at + 0x17a];
        def.rate = readFloat(data, at + 0x28);
        def.duration = readFloat(data, at + 0x38);
        def.lifetimeMin = readFloat(data, at + 0x7c);
        def.lifetimeMax = readFloat(data, at + 0x80);
        std::uint8_t keys = data[at + 0x172];
        if (keys > kMaxColourKeys) throw std::runtime_error("particle defs: too many colour keys");
        for (std::uint8_t k = 0; k < keys; ++k) {
            ColourKey key;
            key.time = readFloat(data, at + 0xf8 + 8u * k);
            std::memcpy(key.rgba.data(), data.data() + at + 0xfc + 8u * k, key.rgba.size());
            def.colourKeys.push_back(key);
        }
        std::uint8_t fades = data[at + 0x170];
        if (fades > kMaxFadeKeys) throw std::runtime_error("particle defs: too many fade keys");
        for (std::uint8_t k = 0; k < fades; ++k)
            def.fadeKeys.push_back({readFloat(data, at + 0x88 + 8u * k), data[at + 0x8c + 8u * k]});
        for (std::size_t t = 0; t < defs.textures.size(); ++t)
            if (pjwHash(defs.textures[t]) == def.textureHash) def.texture = int(t);
        defs.defs.push_back(def);
    }
    if (!cursor.exhausted()) throw std::runtime_error("particle defs: unread blocks");
    return defs;
}
} // namespace chase::assets

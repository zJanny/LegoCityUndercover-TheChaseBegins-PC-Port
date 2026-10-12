#include "assets.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace chase::assets {
namespace {
constexpr std::size_t header = 0x38;
// Bits per pixel for PICA200 formats 0..13.
constexpr std::array<int, 14> bitsPerPixel{32, 24, 16, 16, 16, 16, 16, 8, 8, 8, 4, 4, 4, 8};
constexpr std::array<std::array<int, 2>, 8> etc1Table{{
    {2, 8}, {5, 17}, {9, 29}, {13, 42}, {18, 60}, {24, 80}, {33, 106}, {47, 183}}};

std::uint64_t readU64(const std::uint8_t* p) {
    std::uint64_t value = 0;
    for (int i = 7; i >= 0; --i) value = value << 8 | p[i];
    return value;
}
std::uint8_t expandBits(unsigned value, int bits) {
    return static_cast<std::uint8_t>(bits >= 4 ? (value << (8 - bits)) | (value >> (2 * bits - 8))
                                                : value * 255 / ((1u << bits) - 1));
}
std::uint8_t clampByte(int value) { return static_cast<std::uint8_t>(std::clamp(value, 0, 255)); }

using Pixel = std::array<std::uint8_t, 4>;

Pixel texel(int format, const std::uint8_t* d, std::size_t i) {
    switch (format) {
    case 0: return {d[i * 4 + 3], d[i * 4 + 2], d[i * 4 + 1], d[i * 4]};
    case 1: return {d[i * 3 + 2], d[i * 3 + 1], d[i * 3], 255};
    case 2: case 3: case 4: {
        unsigned v = d[i * 2] | d[i * 2 + 1] << 8;
        if (format == 2) return {expandBits(v >> 11, 5), expandBits(v >> 6 & 31, 5), expandBits(v >> 1 & 31, 5),
                                 std::uint8_t(255 * (v & 1))};
        if (format == 3) return {expandBits(v >> 11, 5), expandBits(v >> 5 & 63, 6), expandBits(v & 31, 5), 255};
        return {std::uint8_t((v >> 12) * 17), std::uint8_t((v >> 8 & 15) * 17), std::uint8_t((v >> 4 & 15) * 17),
                std::uint8_t((v & 15) * 17)};
    }
    case 5: return {d[i * 2 + 1], d[i * 2 + 1], d[i * 2 + 1], d[i * 2]};
    case 6: return {d[i * 2 + 1], d[i * 2], 0, 255};
    case 7: return {d[i], d[i], d[i], 255};
    case 8: return {0, 0, 0, d[i]};
    case 9: {
        std::uint8_t l = std::uint8_t((d[i] >> 4) * 17), a = std::uint8_t((d[i] & 15) * 17);
        return {l, l, l, a};
    }
    default: {
        auto n = std::uint8_t(((d[i / 2] >> (4 * (i & 1))) & 15) * 17);
        return format == 10 ? Pixel{n, n, n, 255} : Pixel{0, 0, 0, n};
    }
    }
}

// One 4x4 ETC1 block; pixel index is column-major (x * 4 + y) as on the 3DS.
void etc1Block(std::uint64_t word, const std::uint64_t* alpha, Pixel out[4][4]) {
    unsigned high = unsigned(word >> 32), low = unsigned(word);
    std::array<std::array<int, 3>, 2> colours{};
    if (high & 2) {
        for (int ch = 0; ch < 3; ++ch) {
            int shift = 27 - 8 * ch;
            int c = int(high >> shift & 31), delta = int(high >> (shift - 3) & 7);
            if (delta & 4) delta -= 8;
            colours[0][ch] = expandBits(unsigned(c), 5);
            colours[1][ch] = expandBits(unsigned((c + delta) & 31), 5);
        }
    } else {
        for (int ch = 0; ch < 3; ++ch) {
            colours[0][ch] = int(high >> (28 - 8 * ch) & 15) * 17;
            colours[1][ch] = int(high >> (24 - 8 * ch) & 15) * 17;
        }
    }
    const auto& table1 = etc1Table[high >> 5 & 7];
    const auto& table2 = etc1Table[high >> 2 & 7];
    bool flip = high & 1;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y) {
            int i = x * 4 + y;
            int sub = (flip ? y : x) >= 2;
            unsigned index = (low >> (i + 16) & 1) << 1 | (low >> i & 1);
            int modifier = (sub ? table2 : table1)[index & 1] * ((index & 2) ? -1 : 1);
            Pixel& p = out[y][x];
            for (int ch = 0; ch < 3; ++ch) p[ch] = clampByte(colours[sub][ch] + modifier);
            p[3] = alpha ? std::uint8_t((*alpha >> (4 * i) & 15) * 17) : 255;
        }
}

// Decode one level into `rgba` rows starting at `row0`, flipping bottom-up storage.
void decodeLevel(const std::uint8_t* d, int width, int height, int format, std::vector<std::uint8_t>& rgba, int row0) {
    int tilesX = width / 8;
    auto store = [&](int x, int y, const Pixel& p) {
        std::size_t at = (std::size_t(row0 + height - 1 - y) * width + x) * 4;
        std::copy(p.begin(), p.end(), rgba.begin() + std::ptrdiff_t(at));
    };
    if (format == 12 || format == 13) {
        std::size_t step = format == 13 ? 16 : 8, offset = 0;
        static const int corners[4][2] = {{0, 0}, {4, 0}, {0, 4}, {4, 4}};
        for (int tile = 0; tile < tilesX * (height / 8); ++tile) {
            int tx = tile % tilesX * 8, ty = tile / tilesX * 8;
            for (const auto& corner : corners) {
                std::uint64_t alpha = format == 13 ? readU64(d + offset) : 0;
                Pixel block[4][4];
                etc1Block(readU64(d + offset + step - 8), format == 13 ? &alpha : nullptr, block);
                offset += step;
                for (int y = 0; y < 4; ++y)
                    for (int x = 0; x < 4; ++x) store(tx + corner[0] + x, ty + corner[1] + y, block[y][x]);
            }
        }
        return;
    }
    for (int tile = 0; tile < tilesX * (height / 8); ++tile) {
        int tx = tile % tilesX * 8, ty = tile / tilesX * 8;
        for (int i = 0; i < 64; ++i) {
            int x = (i & 1) | (i >> 1 & 2) | (i >> 2 & 4);
            int y = (i >> 1 & 1) | (i >> 2 & 2) | (i >> 3 & 4);
            store(tx + x, ty + y, texel(format, d, std::size_t(tile) * 64 + std::size_t(i)));
        }
    }
}
}

Image decodeTexture(const Bytes& data) {
    if (data.size() < header) throw std::runtime_error("truncated texture header");
    static const std::uint32_t magic[5] = {0x400, 0x10, 0xfffffff2u, 0x20, 0xfffffff2u};
    for (int i = 0; i < 5; ++i)
        if (readU32(data, std::size_t(i) * 4) != magic[i]) throw std::runtime_error("unknown texture header");
    std::uint32_t size = readU32(data, 0x14), size2 = readU32(data, 0x1c);
    Image image;
    image.width = readU16(data, 0x18);
    int faceHeight = readU16(data, 0x1a);
    image.format = data[0x20];
    image.levels = int(readU32(data, 0x24));
    if (image.format > 13 || data[0x21] || data[0x22] || data[0x23] > 1)
        throw std::runtime_error("unknown PICA texture format");
    image.faces = data[0x23] ? 6 : 1;
    if (size != size2 || std::size_t(size) != data.size() - header)
        throw std::runtime_error("texture size fields disagree with file length");
    if (!image.width || !faceHeight || image.width % 8 || faceHeight % 8 || image.levels < 1 || image.levels > 16)
        throw std::runtime_error("invalid texture dimensions or mip count");
    std::size_t chain = 0;
    for (int level = 0, w = image.width, h = faceHeight; level < image.levels; ++level, w /= 2, h /= 2) {
        if (w < 8 || h < 8) throw std::runtime_error("mip level smaller than one tile");
        chain += std::size_t(w) * h * std::size_t(bitsPerPixel[image.format]) / 8;
    }
    if (chain * std::size_t(image.faces) != size) throw std::runtime_error("mip chain size disagrees with data size");
    image.height = faceHeight * image.faces;
    image.rgba.assign(std::size_t(image.width) * image.height * 4, 0);
    for (int face = 0; face < image.faces; ++face)
        decodeLevel(data.data() + header + chain * std::size_t(face), image.width, faceHeight, image.format,
                    image.rgba, face * faceHeight);
    return image;
}
} // namespace chase::assets

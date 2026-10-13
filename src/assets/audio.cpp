#include "assets.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace chase::assets {
namespace {
constexpr std::size_t soundRecord = 0x2c;
constexpr std::uint32_t maxSamples = 1u << 26;
constexpr std::array<int, 89> imaSteps{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767};
constexpr std::array<int, 8> imaIndex{-1, -1, -1, -1, 2, 4, 6, 8};

std::int16_t clamp16(int value) { return static_cast<std::int16_t>(std::clamp(value, -32768, 32767)); }
std::int16_t readS16(const Bytes& data, std::size_t offset) { return static_cast<std::int16_t>(readU16(data, offset)); }

std::size_t reference(const Bytes& cwav, std::size_t offset, std::uint16_t kind) {
    if (readU16(cwav, offset) != kind) throw std::runtime_error("unexpected CWAV reference type");
    return readU32(cwav, offset + 4);
}
void need(const Bytes& cwav, std::size_t start, std::size_t length) {
    if (start > cwav.size() || cwav.size() - start < length) throw std::runtime_error("CWAV sample data out of bounds");
}

std::vector<std::int16_t> decodeDsp(const Bytes& cwav, std::size_t start, std::uint32_t samples, std::size_t adpcm) {
    need(cwav, adpcm, 0x26);
    std::array<int, 16> coefs{};
    for (int i = 0; i < 16; ++i) coefs[std::size_t(i)] = readS16(cwav, adpcm + 2 * std::size_t(i));
    int hist1 = readS16(cwav, adpcm + 0x22), hist2 = readS16(cwav, adpcm + 0x24);
    need(cwav, start, (std::size_t(samples) + 13) / 14 * 8);
    std::vector<std::int16_t> out;
    out.reserve(samples);
    for (std::size_t frame = start; out.size() < samples; frame += 8) {
        int header = cwav[frame], scale = 1 << (header & 15), predictor = header >> 4;
        if (predictor > 7) throw std::runtime_error("invalid DSP-ADPCM predictor");
        int c1 = coefs[std::size_t(predictor) * 2], c2 = coefs[std::size_t(predictor) * 2 + 1];
        for (std::size_t i = 1; i < 8 && out.size() < samples; ++i)
            for (int shift : {4, 0}) {
                if (out.size() == samples) break;
                int nibble = (cwav[frame + i] >> shift) & 15;
                if (nibble > 7) nibble -= 16;
                // 64-bit sum (can exceed 2^31); arithmetic shift floors like the reference.
                std::int64_t sum = std::int64_t(nibble) * scale * 2048 + 1024 + std::int64_t(c1) * hist1 +
                                   std::int64_t(c2) * hist2;
                int value = clamp16(int(std::clamp<std::int64_t>(sum >> 11, -32768, 32767)));
                hist2 = hist1;
                hist1 = value;
                out.push_back(std::int16_t(value));
            }
    }
    return out;
}

std::vector<std::int16_t> decodeIma(const Bytes& cwav, std::size_t start, std::uint32_t samples, std::size_t adpcm) {
    need(cwav, adpcm, 3);
    int predictor = readS16(cwav, adpcm), index = std::clamp<int>(cwav[adpcm + 2], 0, 88);
    need(cwav, start, (std::size_t(samples) + 1) / 2);
    std::vector<std::int16_t> out;
    out.reserve(samples);
    for (std::size_t at = start; out.size() < samples; ++at)
        for (int shift : {0, 4}) {
            if (out.size() == samples) break;
            int nibble = (cwav[at] >> shift) & 15, step = imaSteps[std::size_t(index)], diff = step >> 3;
            if (nibble & 4) diff += step;
            if (nibble & 2) diff += step >> 1;
            if (nibble & 1) diff += step >> 2;
            predictor = clamp16(nibble & 8 ? predictor - diff : predictor + diff);
            index = std::clamp(index + imaIndex[std::size_t(nibble & 7)], 0, 88);
            out.push_back(std::int16_t(predictor));
        }
    return out;
}

void decodeCwav(const Bytes& cwav, Sound& sound, bool first) {
    if (cwav.size() < 0x40 || std::memcmp(cwav.data(), "CWAV", 4) != 0 || readU16(cwav, 4) != 0xfeff ||
        readU32(cwav, 0xc) != cwav.size() || readU16(cwav, 0x10) != 2)
        throw std::runtime_error("invalid CWAV header");
    std::size_t info = reference(cwav, 0x14, 0x7000), data = reference(cwav, 0x20, 0x7001);
    need(cwav, info, 0x20);
    need(cwav, data, 8);
    if (std::memcmp(&cwav[info], "INFO", 4) != 0 || std::memcmp(&cwav[data], "DATA", 4) != 0)
        throw std::runtime_error("missing CWAV INFO/DATA block");
    int encoding = cwav[info + 8];
    int rate = int(readU32(cwav, info + 0xc));
    std::uint32_t samples = readU32(cwav, info + 0x14);
    if (encoding > 3 || rate <= 0 || rate > 192000 || samples > maxSamples)
        throw std::runtime_error("unsupported CWAV encoding or parameters");
    if (first) {
        sound.rate = rate;
        sound.loop = cwav[info + 9] != 0;
        sound.loopStart = readU32(cwav, info + 0x10);
        sound.version = readU32(cwav, 8);
    } else if (rate != sound.rate) {
        throw std::runtime_error("CWAV parts disagree on sample rate");
    }
    sound.encodings.push_back(std::uint8_t(encoding));
    std::size_t table = info + 0x1c;
    for (std::uint32_t c = 0, n = readU32(cwav, table); c < n; ++c) {
        std::size_t channel = table + reference(cwav, table + 4 + 8 * std::size_t(c), 0x7100);
        std::size_t start = data + 8 + reference(cwav, channel, 0x1f00);
        std::vector<std::int16_t> pcm;
        if (encoding == 0) {
            need(cwav, start, samples);
            for (std::uint32_t i = 0; i < samples; ++i) pcm.push_back(std::int16_t(std::int8_t(cwav[start + i])));
        } else if (encoding == 1) {
            need(cwav, start, std::size_t(samples) * 2);
            for (std::uint32_t i = 0; i < samples; ++i) pcm.push_back(readS16(cwav, start + 2 * std::size_t(i)));
        } else {
            std::size_t adpcm = channel + reference(cwav, channel + 8, encoding == 2 ? 0x0300 : 0x0301);
            pcm = encoding == 2 ? decodeDsp(cwav, start, samples, adpcm) : decodeIma(cwav, start, samples, adpcm);
        }
        sound.channels.push_back(std::move(pcm));
    }
}
}

Sound decodeSound(const Bytes& data) {
    auto blocks = parsePooled(data);
    if (blocks.size() < 2 || blocks[0].size != soundRecord) throw std::runtime_error("unexpected .bwav block layout");
    Sound sound;
    for (std::size_t i = 1; i < blocks.size(); ++i) {
        Bytes cwav(data.begin() + std::ptrdiff_t(blocks[i].offset),
                   data.begin() + std::ptrdiff_t(blocks[i].offset + blocks[i].size));
        decodeCwav(cwav, sound, i == 1);
    }
    if (sound.channels.empty()) throw std::runtime_error("sound has no channels");
    for (const auto& channel : sound.channels) sound.frames = std::max(sound.frames, std::uint32_t(channel.size()));
    for (auto& channel : sound.channels) channel.resize(sound.frames, 0);
    return sound;
}
} // namespace chase::assets

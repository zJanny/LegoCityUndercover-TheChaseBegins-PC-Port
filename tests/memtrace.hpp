#pragma once
// Reader of the oracle's `memtrace` recordings of the player (tools/azahar_oracle.py): per update a u32 frame, f32 dt, the
// entity (0x100 bytes), the controller (0x340), the controller extension *(ctrl+0xe0) (0x140) and the node (0xb0).
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace memtrace {

constexpr std::size_t kEntity = 0x100, kController = 0x340, kExtension = 0x140, kNode = 0xb0;
constexpr std::size_t kRecord = 8 + kEntity + kController + kExtension + kNode;

struct Row {
    float dt = 0;
    std::uint16_t state = 0, held = 0;
    std::int16_t facing = 0, intent = 0, lastIntent = 0;
    float x = 0, y = 0, z = 0;
    float fallSpeed = 0, fallDistance = 0;
    float scale = 1.0f;            // ctrl+0x94
    float runTimer = 0;            // ctrl+0x24c
    float normal[3] = {0, 1, 0};   // ctrl+0x1ec
    float slide[3] = {0, 0, 0};    // ext+0x5c
};

template <typename T> T read(const std::vector<char>& data, std::size_t offset) {
    T value;
    std::memcpy(&value, data.data() + offset, sizeof value);
    return value;
}

inline std::vector<Row> load(const std::filesystem::path& file) {
    std::ifstream stream(file, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (data.empty() || data.size() % kRecord != 0) throw std::runtime_error("unexpected trace size: " + file.string());
    std::vector<Row> rows;
    for (std::size_t i = 0; i < data.size() / kRecord; ++i) {
        const std::size_t base = i * kRecord;
        const std::size_t controller = base + 8 + kEntity, extension = controller + kController, node = extension + kExtension;
        Row row;
        row.dt = read<float>(data, base + 4);
        row.state = read<std::uint16_t>(data, controller + 0x6c);
        row.held = read<std::uint16_t>(data, controller + 0xc);
        row.facing = read<std::int16_t>(data, controller + 8);
        row.intent = read<std::int16_t>(data, controller + 6);
        row.lastIntent = read<std::int16_t>(data, controller + 0xa);
        row.x = read<float>(data, node + 0x8c);
        row.y = read<float>(data, node + 0x90);
        row.z = read<float>(data, node + 0x94);
        row.fallSpeed = read<float>(data, controller + 0x2b8);
        row.fallDistance = read<float>(data, controller + 0x2b4);
        row.scale = read<float>(data, controller + 0x94);
        row.runTimer = read<float>(data, controller + 0x24c);
        for (int a = 0; a < 3; ++a) {
            row.normal[a] = read<float>(data, controller + 0x1ec + std::size_t(a) * 4);
            row.slide[a] = read<float>(data, extension + 0x5c + std::size_t(a) * 4);
        }
        rows.push_back(row);
    }
    return rows;
}

}

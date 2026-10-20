#pragma once
// Rows of the oracle's `memtrace` recordings of the player (tools/azahar_oracle.py), kept in tests/data/*.inc: per update the fields
// of the controller (entity +0x100, controller +0x340, extension *(ctrl+0xe0) +0x140, node +0xb0 in the recording) that the tests compare.
#include <cstdint>
#include <string>
#include <vector>

namespace memtrace {

struct Row {
    float dt = 0;
    std::uint16_t state = 0, held = 0;
    std::int16_t facing = 0, intent = 0, lastIntent = 0;
    float x = 0, y = 0, z = 0;
    float fallSpeed = 0, fallDistance = 0;
    float normal[3] = {0, 1, 0};   // ctrl+0x1ec
    float slide[3] = {0, 0, 0};    // ext+0x5c
    float scale = 1.0f;            // ctrl+0x94
    float runTimer = 0;            // ctrl+0x24c
};

struct NamedTrace {
    std::string name;
    std::vector<Row> rows;
};

}

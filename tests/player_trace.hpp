#pragma once
// Rows of the oracle's player traces (tools/azahar_oracle.py `trace N NAME mem`), kept in tests/data/*.inc: per update the position,
// facing, state, held flags, fall values and ground normal. A jump or fall recorded in several `trace` calls is one trace.
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace player_trace {

struct Row {
    float dt = 0, x = 0, y = 0, z = 0, forwardX = 0, forwardZ = 0;
    std::uint16_t state = 0, held = 0;
    float fallDistance = 0, fallSpeed = 0;
    float normal[3] = {0, 0, 0};   // ctrl+0x1ec
};

struct NamedTrace {
    std::string name;
    std::vector<Row> rows;
};

inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

}

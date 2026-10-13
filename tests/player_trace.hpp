#pragma once
// Reader of the oracle's player traces (tools/azahar_oracle.py `trace N NAME mem`): NAME.csv holds the position,
// facing and injected input per update, NAME.mem the entity, controller and node bytes. A jump or fall recorded
// in several `trace` calls is stored as STEM-1, STEM-2, ... (consecutive frame numbers) and read as one trace.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace player_trace {

constexpr std::size_t kRecord = 4 + 0x100 + 0x340 + 0xb0;

struct Row {
    float dt = 0, x = 0, y = 0, z = 0, forwardX = 0, forwardZ = 0;
    std::uint16_t state = 0, held = 0;
    float fallDistance = 0, fallSpeed = 0;
};

template <typename T> T read(const std::vector<char>& data, std::size_t offset) {
    T value;
    std::memcpy(&value, data.data() + offset, sizeof value);
    return value;
}

inline std::vector<Row> loadTrace(const std::filesystem::path& csvFile) {
    std::ifstream csv(csvFile);
    if (!csv) throw std::runtime_error("cannot open " + csvFile.string());
    std::string line;
    std::getline(csv, line);
    std::vector<Row> rows;
    while (std::getline(csv, line)) {
        std::vector<std::string> fields;
        std::stringstream stream(line);
        std::string field;
        while (std::getline(stream, field, ',')) fields.push_back(field);
        Row row;
        row.dt = std::stof(fields[1]);
        row.x = std::stof(fields[5]);
        row.y = std::stof(fields[6]);
        row.z = std::stof(fields[7]);
        row.forwardX = std::stof(fields[8]);
        row.forwardZ = std::stof(fields[9]);
        rows.push_back(row);
    }
    std::filesystem::path memFile = csvFile;
    memFile.replace_extension(".mem");
    std::ifstream file(memFile, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.size() != rows.size() * kRecord) throw std::runtime_error("unexpected trace record size: " + memFile.string());
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const std::size_t controller = i * kRecord + 4 + 0x100;
        rows[i].state = read<std::uint16_t>(data, controller + 0x6c);
        rows[i].held = read<std::uint16_t>(data, controller + 0xc);
        rows[i].fallDistance = read<float>(data, controller + 0x2b4);
        rows[i].fallSpeed = read<float>(data, controller + 0x2b8);
    }
    return rows;
}

struct NamedTrace {
    std::string name;
    std::vector<Row> rows;
};

// Every trace of the directory: STEM-N parts are joined in order of N, other files stand alone.
inline std::vector<NamedTrace> loadDirectory(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    for (const auto& item : std::filesystem::directory_iterator(directory))
        if (item.path().extension() == ".csv") files.push_back(item.path());
    std::sort(files.begin(), files.end());
    std::vector<NamedTrace> traces;
    for (const auto& file : files) {
        const std::string stem = file.stem().string();
        const std::size_t dash = stem.rfind('-');
        const bool part = dash != std::string::npos && dash + 1 < stem.size() &&
                          std::all_of(stem.begin() + std::ptrdiff_t(dash) + 1, stem.end(), [](char c) { return c >= '0' && c <= '9'; });
        const std::string name = part ? stem.substr(0, dash) : stem;
        std::vector<Row> rows = loadTrace(file);
        if (part && !traces.empty() && traces.back().name == name)
            traces.back().rows.insert(traces.back().rows.end(), rows.begin(), rows.end());
        else
            traces.push_back({name, std::move(rows)});
    }
    return traces;
}

inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

}

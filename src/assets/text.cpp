#include "assets.hpp"
#include <cctype>
#include <stdexcept>

namespace chase::assets {
std::uint32_t textKey(const std::string& name) {
    std::uint32_t value = 0;
    for (unsigned char c : name) value = value * 65599u + std::uint32_t(std::toupper(c));
    return value;
}

std::map<std::uint32_t, std::string> parseLocalisation(const Bytes& data) {
    if (data.size() < 20 || data[0] != 'L' || data[1] != 'O' || data[2] != 'C' || data[3] != 'A')
        throw std::runtime_error("not a LOCA file");
    std::uint32_t version = readU32(data, 4), count = readU32(data, 8), table = readU32(data, 12),
                  strings = readU32(data, 16);
    if (version != 2 || table != std::uint64_t(count) * 8 || 20 + std::uint64_t(table) + strings != data.size())
        throw std::runtime_error("LOCA header sizes do not match the file");
    std::size_t base = 20 + std::size_t(table);
    std::map<std::uint32_t, std::string> entries;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t key = readU32(data, 20 + 8 * std::size_t(i)), offset = readU32(data, 24 + 8 * std::size_t(i));
        if (i && key <= std::prev(entries.end())->first) throw std::runtime_error("LOCA keys are not sorted");
        if (offset >= strings) throw std::runtime_error("LOCA string offset outside the string block");
        std::size_t begin = base + offset, end = begin;
        while (end < data.size() && data[end]) ++end;
        if (end == data.size()) throw std::runtime_error("unterminated LOCA string");
        entries.emplace(key, std::string(data.begin() + std::ptrdiff_t(begin), data.begin() + std::ptrdiff_t(end)));
    }
    return entries;
}
} // namespace chase::assets

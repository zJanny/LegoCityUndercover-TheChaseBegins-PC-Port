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
std::vector<std::string> extendedCharacters(const Bytes& utf8) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < utf8.size();) {
        std::size_t length = utf8[i] < 0x80 ? 1 : utf8[i] < 0xe0 ? 2 : utf8[i] < 0xf0 ? 3 : 4;
        if (i + length > utf8.size()) throw std::runtime_error("truncated UTF-8 in extended character list");
        std::string glyph(utf8.begin() + std::ptrdiff_t(i), utf8.begin() + std::ptrdiff_t(i + length));
        i += length;
        if (glyph != "\n" && glyph != "\r" && glyph != "\xef\xbb\xbf") out.push_back(glyph);
    }
    return out;
}

std::string decodeText(const std::string& raw, const std::vector<std::string>& extended) {
    std::string out;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        auto c = static_cast<unsigned char>(raw[i]);
        if (c == 0x01 && i + 2 < raw.size() && raw[i + 1] == '2') {
            std::size_t index = static_cast<unsigned char>(raw[i + 2]) - 0x30u;
            if (static_cast<unsigned char>(raw[i + 2]) >= 0x30 && index < extended.size()) {
                out += extended[index];
                i += 2;
                continue;
            }
        }
        if (c < 0x80) {
            out += char(c);
        } else {
            out += char(0xc0 | (c >> 6));
            out += char(0x80 | (c & 0x3f));
        }
    }
    return out;
}
} // namespace chase::assets

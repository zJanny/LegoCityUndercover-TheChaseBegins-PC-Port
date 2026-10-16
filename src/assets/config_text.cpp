#include "assets.hpp"
#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace chase::assets {
namespace {
class ConfigParser {
public:
    explicit ConfigParser(const std::string& text) : text_(text) {}

    std::vector<ConfigBlock> parseDocument() {
        std::vector<ConfigBlock> roots;
        while (skipSpace(), pos_ < text_.size()) {
            unsigned line = line_;
            std::string name = readName();
            skipSpace();
            if (peek() != '{') fail("expected '{' after block name '" + name + "'");
            roots.push_back(parseBody(name, line));
        }
        return roots;
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;
    unsigned line_ = 1;

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error("config text line " + std::to_string(line_) + ": " + message);
    }
    char peek() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }

    void skipSpace() {
        while (pos_ < text_.size()) {
            char c = text_[pos_];
            if (c == '\n') {
                ++line_;
                ++pos_;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                ++pos_;
            } else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/') {
                while (pos_ < text_.size() && text_[pos_] != '\n') ++pos_;
            } else {
                break;
            }
        }
    }

    std::string readName() {
        std::size_t begin = pos_;
        while (pos_ < text_.size()) {
            char c = text_[pos_];
            if (std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}' || c == '(' || c == ')') break;
            ++pos_;
        }
        if (pos_ == begin) fail(std::string("expected a name, found '") + (pos_ < text_.size() ? text_[pos_] : ' ') + "'");
        return text_.substr(begin, pos_ - begin);
    }

    std::string readValue() {
        std::size_t begin = ++pos_;
        bool inBraces = false;
        while (pos_ < text_.size() && text_[pos_] != '\n') {
            if (text_[pos_] == '{') inBraces = true;
            else if (text_[pos_] == '}') inBraces = false;
            else if (text_[pos_] == ')' && !inBraces) break;
            ++pos_;
        }
        std::string value = text_.substr(begin, pos_ - begin);
        while (pos_ < text_.size() && text_[pos_] != '\n') ++pos_;
        std::size_t first = value.find_first_not_of(" \t\r");
        if (first == std::string::npos) return {};
        return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
    }

    ConfigBlock parseBody(const std::string& name, unsigned line) {
        ConfigBlock block;
        block.name = name;
        block.line = line;
        ++pos_;
        for (;;) {
            skipSpace();
            if (pos_ >= text_.size()) fail("missing '}' for block '" + name + "' opened on line " + std::to_string(line));
            if (peek() == '}') {
                ++pos_;
                return block;
            }
            unsigned childLine = line_;
            std::string child = readName();
            skipSpace();
            if (peek() == '(') {
                block.entries.push_back({child, readValue(), childLine});
            } else if (peek() == '{') {
                block.blocks.push_back(parseBody(child, childLine));
            } else {
                fail("expected '(' or '{' after '" + child + "'");
            }
        }
    }
};

double parseNumber(const std::string& text, const std::string& key) {
    const char* begin = text.c_str();
    char* end = nullptr;
    double value = std::strtod(begin, &end);
    while (*end == ' ' || *end == '\t') ++end;
    if (end == begin || *end) throw std::runtime_error("config value of '" + key + "' is not a number: " + text);
    return value;
}

template <std::size_t N> std::array<double, N> fixedNumbers(const ConfigBlock& block, const std::string& key) {
    std::vector<double> values = block.numbers(key);
    if (values.size() != N)
        throw std::runtime_error("camera entry '" + key + "' needs " + std::to_string(N) + " numbers");
    std::array<double, N> out;
    for (std::size_t i = 0; i < N; ++i) out[i] = values[i];
    return out;
}
}

const ConfigEntry* ConfigBlock::find(const std::string& key) const {
    for (const auto& entry : entries)
        if (entry.key == key) return &entry;
    return nullptr;
}

const std::string& ConfigBlock::value(const std::string& key) const {
    const ConfigEntry* entry = find(key);
    if (!entry) throw std::runtime_error("config block '" + name + "' has no entry '" + key + "'");
    return entry->value;
}

double ConfigBlock::number(const std::string& key) const { return parseNumber(value(key), key); }

std::vector<double> ConfigBlock::numbers(const std::string& key) const {
    const std::string& text = value(key);
    std::vector<double> out;
    std::size_t begin = 0;
    for (;;) {
        std::size_t comma = text.find(',', begin);
        out.push_back(parseNumber(text.substr(begin, comma == std::string::npos ? comma : comma - begin), key));
        if (comma == std::string::npos) return out;
        begin = comma + 1;
    }
}

std::vector<ConfigBlock> parseConfigText(const std::string& text) { return ConfigParser(text).parseDocument(); }

CameraFollow parseCameraFollow(const std::string& text) {
    std::vector<ConfigBlock> roots = parseConfigText(text);
    if (roots.size() != 1 || roots[0].name != "CameraFollow" || !roots[0].entries.empty())
        throw std::runtime_error("expected a single CameraFollow block");
    CameraFollow out;
    for (const ConfigBlock& block : roots[0].blocks) {
        CameraSetup setup;
        setup.kind = block.name;
        setup.name = block.value("Name");
        setup.type = int(block.number("Type"));
        setup.rotationY = block.number("RotationY");
        setup.rotationX = block.number("RotationX");
        setup.distance = block.number("Distance");
        setup.location = fixedNumbers<3>(block, "Location");
        setup.lookat = fixedNumbers<3>(block, "Lookat");
        setup.offset = fixedNumbers<3>(block, "Offset");
        setup.fov = block.number("FOV");
        setup.filters = fixedNumbers<2>(block, "Filters");
        setup.blendTime = block.number("BlendTime");
        setup.shakeAmp = block.number("ShakeAmp");
        setup.shakeRate = block.number("ShakeRate");
        setup.shakeRoll = block.number("ShakeRoll");
        setup.offset3d = block.number("3DOffset");
        out.setups.push_back(std::move(setup));
    }
    return out;
}
} // namespace chase::assets

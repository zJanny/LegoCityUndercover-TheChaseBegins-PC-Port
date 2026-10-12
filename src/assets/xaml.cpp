#include "block_reader.hpp"
#include <stdexcept>

namespace chase::assets {
namespace {
using Reader = detail::BlockReader;
using detail::cString;
using detail::readF32;

constexpr std::size_t elementRecord = 0x4c;
constexpr std::size_t storyboardList = 8;
constexpr std::size_t storyboardRecord = 0x10;
constexpr std::size_t trackRecord = 0x20;
constexpr std::size_t keyframeRecord = 0x20;
constexpr int maxDepth = 64;

Bytes record(Reader& r, std::size_t size, const char* what) {
    Bytes block = r.next();
    if (block.size() != size) throw std::runtime_error(std::string("bxaml ") + what + " block has the wrong size");
    return block;
}

std::string name(Reader& r) {
    Bytes block = r.next();
    if (block.empty()) throw std::runtime_error("bxaml name block is missing");
    return cString(block);
}

std::size_t count(const Bytes& block, std::size_t offset) {
    auto value = static_cast<std::int16_t>(readU16(block, offset));
    if (value < 0) throw std::runtime_error("bxaml negative count");
    return std::size_t(value);
}

UiElement element(Reader& r, int depth) {
    if (depth > maxDepth) throw std::runtime_error("bxaml element tree too deep");
    Bytes b = record(r, elementRecord, "element");
    UiElement e;
    std::uint32_t type = readU32(b, 8);
    if (type < 1 || type > 3) throw std::runtime_error("bxaml unknown element type");
    e.type = static_cast<UiElementType>(type);
    e.name = name(r);
    if (e.type != UiElementType::Container) e.source = name(r);
    e.unknown14 = b[0x14];
    e.opacity = readF32(b, 0x18);
    e.rotation = readF32(b, 0x1c);
    for (std::size_t i = 0; i < 2; ++i) {
        e.scale[i] = readF32(b, 0x20 + 4 * i);
        e.position[i] = readF32(b, 0x28 + 4 * i);
        e.origin[i] = readF32(b, 0x30 + 4 * i);
        e.size[i] = readF32(b, 0x38 + 4 * i);
    }
    e.flags = readU16(b, 0x42);
    e.callback = static_cast<std::int16_t>(readU16(b, 0x44));
    e.fontSize = b[0x46];
    e.alignment = b[0x47] & 3;
    for (std::size_t i = 0; i < 4; ++i) e.colour[i] = b[0x48 + i];
    std::size_t children = count(b, 0x40);
    for (std::size_t i = 0; i < children; ++i) e.children.push_back(element(r, depth + 1));
    return e;
}

UiTrack track(Reader& r) {
    Bytes b = record(r, trackRecord, "track");
    UiTrack t;
    t.target = name(r);
    t.repeat = static_cast<std::int16_t>(readU16(b, 0xa));
    t.property = b[0x14];
    t.duration = readF32(b, 0x18);
    t.unknown1c = readU32(b, 0x1c);
    std::size_t keyframes = count(b, 8);
    for (std::size_t i = 0; i < keyframes; ++i) {
        Bytes k = record(r, keyframeRecord, "keyframe");
        UiKeyframe key;
        key.value = readF32(k, 4);
        for (std::size_t c = 0; c < 4; ++c) key.spline[c] = readF32(k, 8 + 4 * c);
        key.time = readF32(k, 0x18);
        key.flag = k[0x1c];
        t.keyframes.push_back(key);
    }
    return t;
}
}

UiLayout parseLayout(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M14);
    UiLayout layout;
    bool animated = readU32(record(r, 4, "header"), 0) != 0;
    layout.root = element(r, 0);
    if (animated) {
        Bytes list = record(r, storyboardList, "storyboard list");
        std::size_t storyboards = count(list, 4);
        for (std::size_t i = 0; i < storyboards; ++i) {
            Bytes b = record(r, storyboardRecord, "storyboard");
            UiStoryboard s;
            s.name = name(r);
            std::size_t tracks = count(b, 0xc);
            for (std::size_t k = 0; k < tracks; ++k) s.tracks.push_back(track(r));
            layout.storyboards.push_back(std::move(s));
        }
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed bxaml blocks");
    return layout;
}
} // namespace chase::assets

#include "assets.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::assets {
namespace {
constexpr std::uint32_t previousStage = 0xf;
constexpr std::uint32_t alphaPreviousStage = 0xf0000;
constexpr std::size_t maxStages = 6;

enum Combine : std::uint32_t { Replace = 0, Modulate = 1, Add = 2, Lerp = 4, Dot3Rgb = 6, MultAdd = 8, AddMult = 9 };

constexpr std::uint8_t lightingByM2[16] = {0, 4, 0x0f, 0, 4, 0x0f, 0x0f, 0x0f, 0, 0x0f, 0, 0, 0, 0, 0, 0};
constexpr unsigned blendEquations[5] = {0, 1, 2, 3, 4};
constexpr unsigned blendFactors[10] = {0, 1, 2, 3, 6, 7, 4, 5, 8, 9};
constexpr unsigned depthFunctions[8] = {5, 6, 7, 4, 2, 0, 1, 0};

TevStage stage(std::size_t index, std::uint32_t source, std::uint32_t operand, std::uint32_t combiner, std::uint32_t constant) {
    TevStage s;
    s.source = index == 0 ? source : source | alphaPreviousStage;
    s.operand = operand;
    s.combiner = combiner;
    s.constant = constant;
    return s;
}

std::uint32_t packedColour(const std::uint8_t* material) {
    auto channel = [&](std::size_t offset) {
        std::uint32_t bits = std::uint32_t(material[offset]) | std::uint32_t(material[offset + 1]) << 8 |
                             std::uint32_t(material[offset + 2]) << 16 | std::uint32_t(material[offset + 3]) << 24;
        float value;
        std::memcpy(&value, &bits, sizeof value);
        return value;
    };
    auto byte = [](float value) { return std::uint32_t(std::min(255.0f, std::max(0.0f, std::floor(value * 255.0f + 0.5f)))); };
    const float alpha = channel(0x3c) - channel(0x44);
    return byte(channel(0x30)) | byte(channel(0x34)) << 8 | byte(channel(0x38)) << 16 | byte(alpha) << 24;
}
}

MaterialState materialState(const ModelPart& part) {
    MaterialState out;
    if (part.material.size() < 0x60) return out;
    const std::uint8_t* m = part.material.data();
    out.mode = m[1] & 0x1f;
    out.lightingKey = m[2] & 0xf;
    out.textureKey = m[3] & 0xf;
    out.alphaKey = m[5] & 7;
    const unsigned textureCount = (m[10] & 0x3fu) >> 3;
    const bool vertexColour = m[10] & 0x40, bump = m[0xd] & 2, materialColour = m[0xc] & 0x40;
    out.reflection = out.textureKey == 5 || out.textureKey == 6 || out.textureKey == 7;
    out.lighting = lightingByM2[out.lightingKey] != 0;

    const unsigned used = std::min(textureCount, 3u);
    for (unsigned unit = 0; unit < used; ++unit)
        out.unitSlot[unit] = out.reflection ? int(textureCount - 1 - unit) : int(unit);

    std::uint32_t diffuse = 0xe, second = 0xe, third = 0xe;
    for (unsigned i = 0; i < used; ++i) {
        const std::uint32_t unit = out.reflection ? textureCount - i + 2 : i + 3;
        const std::size_t roleAt = std::size_t(i) * 8 + 7;
        const unsigned role = roleAt < part.textureSlots.size() ? part.textureSlots[roleAt] : 0;
        if (role == 0) diffuse = unit;
        else if (role == 1) second = unit;
        else if (role == 2) third = unit;
    }

    auto& stages = out.stages;
    switch (out.mode) {
    case 0: stages.push_back(stage(0, diffuse, 0, Replace, 0xffffff)); break;
    case 1:
        stages.push_back(stage(0, second, 0x22, Modulate, 0xffffff));
        stages.push_back(stage(1, diffuse | second << 4 | 0xf00, 0, Lerp, 0xffffff));
        break;
    case 2: stages.push_back(stage(0, diffuse | second << 4, 0, Add, 0xffffff)); break;
    case 3: stages.push_back(stage(0, 0xe, 0, Replace, 0xffffff)); break;
    default: return out;
    }
    std::uint32_t buffer = 0;
    if (bump) buffer = 1u << stages.size();

    switch (out.lightingKey) {
    case 0: stages.push_back(stage(stages.size(), 0xf, 0, Modulate, 0)); break;
    case 1: stages.push_back(stage(stages.size(), 0xf10, 0, AddMult, 0)); break;
    case 2: case 5: case 6: case 7: case 9: stages.push_back(stage(stages.size(), 0xf1, 0, Modulate, 0)); break;
    case 3: stages.push_back(stage(stages.size(), third | 0xf00, 0, AddMult, 0)); break;
    case 4:
        buffer |= 1u << stages.size();
        stages.push_back(stage(stages.size(), third | 0xf00, 0, Add, 0));
        stages.push_back(stage(stages.size(), 0xd1f, 0, AddMult, 0));
        break;
    case 0xd: break;
    default: return out;
    }

    if (bump && out.lightingKey != 0xd && stages.size() < 4) {
        TevStage& last = stages.back();
        if (last.combiner == Modulate) last.combiner = Replace;
        else if (last.combiner == AddMult) last.combiner = Add;
        stages.push_back(stage(stages.size(), 0xefe, 0, MultAdd, 0x7f7f7f));
        stages.push_back(stage(stages.size(), 0xfe, 0, Dot3Rgb, 0x8ecba6));
        stages.push_back(stage(stages.size(), 0xdf, 0, Modulate, 0));
    }
    if (vertexColour && stages.size() < maxStages) stages.push_back(stage(stages.size(), 0xf, 0, Modulate, 0));
    if (materialColour && stages.size() < maxStages) stages.push_back(stage(stages.size(), 0xefe, 0x200, Lerp, packedColour(m)));

    switch (out.textureKey) {
    case 0: break;
    case 1: case 2:
        if (stages.size() < maxStages) stages.push_back(stage(stages.size(), diffuse << 4 | 0xf02, 0x20, MultAdd, 0));
        break;
    case 5:
        if (stages.size() < maxStages) stages.push_back(stage(stages.size(), diffuse << 8 | 0xf3, 0x200, Lerp, 0));
        break;
    case 6:
        if (stages.size() < maxStages) stages.push_back(stage(stages.size(), diffuse << 4 | 0xf03, 0x20, MultAdd, 0));
        break;
    case 7:
        if (stages.size() < 4) {
            buffer |= 1u << stages.size();
            stages.push_back(stage(stages.size(), 0xf33, 0, Modulate, 0));
            stages.push_back(stage(stages.size(), diffuse | 0xf0, 2, Modulate, 0));
            stages.back().scale = 1;
            stages.push_back(stage(stages.size(), 0xd0f, 0, MultAdd, 0));
        }
        break;
    default: return out;
    }

    const std::uint32_t alpha = materialColour ? std::uint32_t(m[0x13]) * (packedColour(m) >> 24) / 0xff : m[0x13];
    stages[0].constant |= alpha << 24;
    TevStage& first = stages[0];
    switch (out.alphaKey) {
    case 0: first.source |= 0xe00000; first.combiner |= 0x10000; break;
    case 1: case 6: first.source |= diffuse << 16 | 0xe00000; first.combiner |= 0x10000; break;
    case 2: case 7:
        if (stages.size() < 2) stages.push_back(stage(stages.size(), 0xf, 0, Replace, 0));
        first.source |= diffuse << 16 | 0xe00000;
        first.combiner |= 0x10000;
        stages[1].combiner |= 0x10000;
        break;
    case 3: first.source |= diffuse << 16 | 0xe000000; first.combiner |= 0x90000; break;
    case 4: first.source |= second << 16 | 0xe00000; first.combiner |= 0x10000; break;
    case 5:
        if (stages.size() < 2) stages.push_back(stage(stages.size(), 0xf, 0, Replace, 0));
        first.source |= second << 16 | 0xe00000;
        first.combiner |= 0x10000;
        stages[1].combiner |= 0x10000;
        break;
    }
    stages.back().scale = (m[0xb] & 0x3f) >> 5;
    out.bufferUpdate = buffer;

    out.alphaTest = m[0xb] & 0x10;
    out.alphaReference = m[6];
    if (m[7] >= 5 || m[8] >= 10 || m[9] >= 10) return out;
    out.blendEquation = blendEquations[m[7]];
    out.blendSource = blendFactors[m[8]];
    out.blendDestination = blendFactors[m[9]];
    out.depthFunction = depthFunctions[m[0xb] & 7];
    out.depthTest = (m[0xb] & 7) != 6;
    out.depthWrite = m[0xb] & 8;
    out.fogged = !(m[0xb] & 0x40) && m[9] != 1;
    const unsigned cull = m[0xc] & 3;
    out.cullMode = cull == 0 ? 1 : cull == 1 ? 2 : 0;
    out.supported = true;
    return out;
}
}

// The audio scanner's render state (src/runtime/scanner_render.cpp) and the material override 0x530834 in assets::materialState.
// Values: 0x2e4858 case 3 / 4 and 0x2ef664 (REA), the bytes 0x530835 = 3, 0x53083c = 0xffab8126, 0x530840 = 0, 0x51641e = 1,
// 0x3760fd = 1 and 0x3772cd = 0 read in the oracle in the view of m12-8-roof-sound-scanner (docs/re/abilities-scanner.md §2026-10-19T14:06:22).
#include "assets/assets.hpp"
#include "runtime/scanner_render.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::ModelPart litMaterial() {
    assets::ModelPart part;
    part.material.assign(0x60, 0);
    part.material[1] = 0;
    part.material[2] = 2;
    part.material[3] = 0;
    part.material[5] = 1;
    part.material[10] = 0x48;
    part.material[0xb] = 0x28;
    part.material[0x13] = 0xff;
    part.textureSlots.assign(8, 0);
    return part;
}

bool stageIs(const assets::TevStage& s, std::uint32_t source, std::uint32_t operand, std::uint32_t combiner, std::uint32_t constant,
             std::uint32_t scale) {
    return s.source == source && s.operand == operand && s.combiner == combiner && s.constant == constant && s.scale == scale;
}

void renderState() {
    const runtime::ScannerRender off = runtime::scannerRender(false);
    require(!off.active && !off.noFog && !off.overrideSky && off.world.flags == 0, "nothing changes outside the audio mode");
    const runtime::ScannerRender on = runtime::scannerRender(true);
    require(on.active && on.noFog && on.overrideSky, "0x2ef664: 0x51641e = 1, 0x3760fd = 1, 0x3772cd = 0");
    require(on.world.flags == 0x95 && on.world.mode == 3 && on.world.texture == 0 && on.world.extra == 0 && on.world.constant == 0xffab8126,
            "pass 3 override of 0x2e4858");
    require(on.drawList.flags == 0xd5 && on.drawList.mode == 3 && on.drawList.constant == 0xffab8126, "pass 4 override adds flag 6");
}

void overriddenLitMaterial() {
    const assets::ModelPart part = litMaterial();
    const assets::MaterialState plain = assets::materialState(part);
    require(plain.supported && plain.stages.size() == 3 && stageIs(plain.stages[0], 0x00e30003, 0, 0x10000, 0xffffffff, 0) &&
                stageIs(plain.stages[1], 0x000f00f1, 0, 1, 0, 0) && stageIs(plain.stages[2], 0x000f000f, 0, 1, 0, 1) && plain.bufferUpdate == 0,
            "the material without override: texture, lighting, vertex colour");
    require(assets::materialState(part, assets::MaterialOverride{}).stages.size() == 3, "an empty override changes nothing");

    const assets::MaterialState s = assets::materialState(part, runtime::scannerRender(true).world);
    require(s.supported && s.mode == 3 && s.textureKey == 0 && s.lightingKey == 2 && s.alphaKey == 1, "keys after the override");
    require(s.stages.size() == 5, "constant, lighting, three bump stages; no vertex colour stage: " + std::to_string(s.stages.size()));
    require(stageIs(s.stages[0], 0x00e3000e, 0, 0x10000, 0xffab8126, 0), "stage 0: the constant 0xab8126, alpha from the texture");
    require(stageIs(s.stages[1], 0x000f00f1, 0, 0, 0, 0), "stage 1: fragment lighting, modulate turned into replace");
    require(stageIs(s.stages[2], 0x000f0efe, 0, 8, 0x7f7f7f, 0), "stage 2: multiply-add with 0x7f7f7f");
    require(stageIs(s.stages[3], 0x000f00fe, 0, 6, 0x8ecba6, 0), "stage 3: dot3 with 0x8ecba6");
    require(stageIs(s.stages[4], 0x000f00df, 0, 1, 0, 1), "stage 4: times the buffer, scale from byte 0xb bit 5");
    require(s.bufferUpdate == 2, "the buffer keeps the output of stage 0");
    double weights[3];
    const unsigned channels[3] = {0xa6, 0xcb, 0x8e};
    for (int c = 0; c < 3; ++c) weights[c] = 4.0 * (0x7f / 255.0) * (channels[c] / 255.0 - 0.5);
    require(std::fabs(weights[0] - 0.3004) < 1e-3 && std::fabs(weights[1] - 0.5898) < 1e-3 && std::fabs(weights[2] - 0.1135) < 1e-3,
            "stages 2 and 3 give the luminance 0.30 r + 0.59 g + 0.11 b");
}

void overrideFlags() {
    assets::ModelPart part = litMaterial();
    part.material[1] = 3;
    require(assets::materialState(part).stages[0].constant == 0xffffffff, "mode 3 of the material itself stays white");
    assets::MaterialOverride o;
    o.flags = 0x04;
    o.texture = 0;
    o.constant = 0xff0000ff;
    require(assets::materialState(part, o).stages[0].constant == 0xffffffff, "the constant needs flag 0");
    part.material[1] = 0;
    part.material[0xb] = 0x08;
    part.material[10] = 0x08;
    require(assets::materialState(part).stages.size() == 2, "no vertex colour stage without byte 10 bit 6");
    o.flags = 0x30;
    o.extra = 3;
    const assets::MaterialState scaled = assets::materialState(part, o);
    require(scaled.stages.back().scale == 1 && scaled.stages.size() == 3, "flags 4 and 5: vertex colour and scale from extra bits 0 and 1");
    o.flags = 0x0a;
    o.lighting = 0;
    o.alpha = 0;
    const assets::MaterialState keys = assets::materialState(part, o);
    require(keys.lightingKey == 0 && keys.alphaKey == 0 && keys.stages[0].source == 0x00e00003, "flags 1 and 3: lighting and alpha keys");
    part.material[3] = 5;
    part.material[10] = 0x10;
    part.textureSlots.assign(16, 0);
    require(assets::materialState(part).reflection, "texture key 5 reflects");
    o.flags = 0x04;
    o.texture = 0;
    require(!assets::materialState(part, o).reflection, "0x129560: the reflection flag follows the overridden texture key");
    part.material[9] = 1;
    require(assets::materialState(part).additive, "blend destination index 1 is additive");
}
}

int main() {
    try {
        renderState();
        overriddenLitMaterial();
        overrideFlags();
    } catch (const std::exception& e) {
        std::cerr << "scanner-render-test: " << e.what() << '\n';
        return 1;
    }
    std::cout << "scanner-render-test: ok\n";
    return 0;
}

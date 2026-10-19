// The glow post-process 0x2bf830 (src/runtime/glow.cpp) and its scanner call. Values: REA pseudocode of 0x2bf830 and the frame function
// 0x2e4858 (call at 0x2e4f28 with the literals 0x2e50c8 = 0.6 and 0x2e50c4 = 1.4), the texture headers 0x52a698 / 0x52a6b8 / 0x52a6d8 and
// the stage tables of modes 0xc, 0xd and 0x12 (0x5957f8 + mode * 0x94) read in the oracle in the view of m12-8-roof-sound-scanner
// (docs/re/abilities-scanner.md, glow entry).
#include "runtime/glow.hpp"
#include "runtime/scanner_render.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

void targets() {
    require(kGlowFirstTargetWidth == 256 && kGlowFirstTargetHeight == 128, "first target: the quad 0..256 x 0..128 of 0x2bf830");
    require(kGlowBlurTargetWidth == 128 && kGlowBlurTargetHeight == 64, "blur targets: texture headers 0x0040 x 0x0080 at 0x52a6d8 / 0x52a6f8");
    require(kGlowBlurIterations == 3, "the loop of 0x2bf830 runs three times");
    require(kGlowCentreWeightByte == 0x80 && kGlowSideWeightByte == 0x40, "stage constants 0x80808080 / 0x40404040 of modes 0xc and 0xd");
}

void blurParameters() {
    require(near(glowBlurOffsetTexels(0), 4.5f) && near(glowBlurOffsetTexels(1), 3.0f) && near(glowBlurOffsetTexels(2), 1.5f),
            "offsets 4.5 - 1.5 i");
    require(near(glowBlurWeight(0), 1.5f) && near(glowBlurWeight(1), 1.22f) && near(glowBlurWeight(2), 1.0f), "weights 1.5 - 0.28 i, at least 1");
    require(glowBlurWeightAlphaByte(0) == 192 && glowBlurWeightAlphaByte(1) == 156 && glowBlurWeightAlphaByte(2) == 128,
            "constant alpha (w * 128 + 0.5)");
}

void callCondition() {
    require(!glowRuns(0.0f, false), "off in normal play: 0x516410 = 0 and 0x52ff94 = 0");
    require(glowRuns(0.6f, false) && glowRuns(0.0f, true), "on with a strength or with 0x52ff94");
}

void composite() {
    const GlowComposite scanner = glowComposite(0.6f, 1.4f);
    require(scanner.doubled && scanner.colour == 77 && scanner.alpha == 179, "(0.6, 1.4): halved to (0.3, 0.7), bytes 77 / 179, last stage x2");
    const GlowComposite small = glowComposite(0.25f, 0.5f);
    require(!small.doubled && small.colour == 64 && small.alpha == 128, "a sum of at most 1 keeps the strengths and the scale");
    require(glowComposite(4.0f, 0.0f).colour == 255, "clamped to 255 after halving");
    require(kGlowBlurredAlpha == 0.0f, "stand-in RENDER-S65");
}

void scannerCall() {
    const ScannerRender off = scannerRender(false);
    require(off.glowStrength == 0.0f && off.glowSecond == 0.0f && !glowRuns(off.glowStrength, false), "no glow outside the audio mode");
    const ScannerRender on = scannerRender(true);
    require(near(on.glowStrength, 0.6f) && near(on.glowSecond, 1.4f) && glowRuns(on.glowStrength, false), "pass 5 of 0x2e4858 in the scanner: (0.6, 1.4)");
}
}

int main() {
    try {
        targets();
        blurParameters();
        callCondition();
        composite();
        scannerCall();
    } catch (const std::exception& e) {
        std::cerr << "glow-test: " << e.what() << '\n';
        return 1;
    }
    std::cout << "glow-test: ok\n";
    return 0;
}

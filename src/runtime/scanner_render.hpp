#pragma once
// The render state of the audio scanner view (docs/re/abilities-scanner.md): the audio mode's enter 0x2ef664 sets 0x51641e = 1,
// 0x3772cd = 0 and the no-fog byte 0x3760fd = 1, its exit 0x2ef694 clears them; while 0x51641e is set the frame function 0x2e4858
// draws pass 3 (world, sky through 0x180fac) with the override 0x95 and pass 4 (the draw list 0x2128c8) with 0xd5.
#include "assets/assets.hpp"

namespace chase::runtime {

struct ScannerRender {
    bool active = false;
    assets::MaterialOverride world;     // pass 3 of 0x2e4858
    assets::MaterialOverride drawList;  // pass 4 of 0x2e4858
    bool overrideSky = false;           // 0x3772cd == 0: 0x180fac keeps the override for the sky
    bool noFog = false;                 // 0x3760fd
    float glowStrength = 0.0f;          // pass 5 of 0x2e4858: 0x2bf830 (0.6, 1.4) instead of (0x516410, 0.7); the literals 0x2e50c8 / 0x2e50c4
    float glowSecond = 0.0f;
};

// 0x2e4858 case 3: flags 0x95, mode 3, texture 0, extra bit 0 cleared (the other bits of 0x530840 read 0 in the oracle), constant 0xffab8126.
constexpr std::uint8_t kScannerWorldOverrideFlags = 0x95;
constexpr std::uint8_t kScannerDrawListOverrideFlags = 0xd5;
constexpr std::uint8_t kScannerOverrideMode = 3;
constexpr std::uint32_t kScannerOverrideConstant = 0xffab8126;
constexpr float kScannerGlowStrength = 0.6f;
constexpr float kScannerGlowSecond = 1.4f;

ScannerRender scannerRender(bool audioModeEntered);

}

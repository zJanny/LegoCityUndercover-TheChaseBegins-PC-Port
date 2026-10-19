#pragma once
// The glow post-process 0x2bf830 (docs/re/rendering-glow.md): the screen is copied to a 256 x 128 target, copied again to
// 128 x 64, blurred three times (vertical table mode 0xc, then horizontal mode 0xd, both through three texture units) and added back with
// table mode 0x12 and blend one / one.
#include <cstdint>

namespace chase::runtime {

constexpr int kGlowFirstTargetWidth = 256;   // quad 0..256 x 0..128 of 0x2bf830 for target 0x52a6b8 (texture header 0x0080 x 0x0100)
constexpr int kGlowFirstTargetHeight = 128;
constexpr int kGlowBlurTargetWidth = 128;    // targets 0x52a6d8 / 0x52a6f8 (texture header 0x0040 x 0x0080)
constexpr int kGlowBlurTargetHeight = 64;
constexpr int kGlowBlurIterations = 3;

// Mode 0xc / 0xd stage constants read at 0x595ee8 / 0x595f7c: stage 0 multiplies texture 0 by 0x80, stages 1 and 2 add texture 1 and 2 times 0x40.
constexpr std::uint8_t kGlowCentreWeightByte = 0x80;
constexpr std::uint8_t kGlowSideWeightByte = 0x40;

// Texel offset of iteration i (0x2bf830: 4.5 - 1.5 i, in texels of the source of the pass; mode 0xc divides by the texture's first
// dimension, mode 0xd by the second).
float glowBlurOffsetTexels(int iteration);
// The weight 1.5 - 0.28 i (at least 1.0) of iteration i goes into the alpha of the stage-0 constant, (w * 128 + 0.5) << 24.
float glowBlurWeight(int iteration);
std::uint8_t glowBlurWeightAlphaByte(int iteration);

// 0x2bf830 runs when the first strength is not 0, when 0x52ff94 (read by 0x1a3888) is not 0, or when a mask texture is given with a
// non-zero alpha; the scanner call has no mask.
bool glowRuns(float strength, bool flag1a3888);

// The final draw: the two strengths are halved and the last stage scales by 2 when they add up to more than 1; the vertex colour is
// (c, c, c, a) with the bytes (s * 255 + 0.5) clamped to 0..255 (mode 0x12: (a * T.alpha + c) * T * T, T the blurred target).
struct GlowComposite {
    std::uint8_t colour = 0;
    std::uint8_t alpha = 0;
    bool doubled = false;
};
GlowComposite glowComposite(float strength, float second);

// STAND-IN RENDER-S65: the alpha of the blurred target. The oracle shows no dependence on the second strength (docs/re/rendering-glow.md),
// so the alpha term of mode 0x12 is taken as 0.
constexpr float kGlowBlurredAlpha = 0.0f;

}

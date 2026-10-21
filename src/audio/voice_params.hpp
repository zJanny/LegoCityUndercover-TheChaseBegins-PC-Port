#pragma once
// The arithmetic of a sound voice's volume (voice +0x50) and rate (voice +0x58) fields: the voice set-up 0x23f82c / 0x23f940,
// the script setters SetSoundVolume 0x219178 -> 0x227ff4 and ScaleSoundFrequency 0x22f770 -> 0x227f4c, and the voice
// synchroniser 0x25c388 with the pitch setter 0x25b3b4. Evidence: docs/re/audio.md. No SDL dependency.
#include <array>
#include <cstdint>

namespace chase::audio {

// 0x227f4c with -1 stores the sound's own source rate instead.
constexpr std::int32_t kResetRate = -1;

// Bit 3 of the sound record's flag byte (tablessoundfx column 3) selects the rate boost together with the byte 0x376ce6.
constexpr std::uint32_t kRateBoostRecordBit = 8;

// 0x23f9b0: the table percent (record byte +9) times 0.01f.
float initialVoiceVolume(std::uint32_t percent);

// 0x23f908 and 0x22f7e0: the source rate plus half of it (logical shift right).
std::uint32_t boostedRate(std::uint32_t sourceRate);

// 0x22f7e4..0x22f7fc: (int)(scale * (float)rate) with the rate boosted when `boost`; vcvt.s32.f32 truncates towards zero,
// saturates and turns NaN into 0.
std::int32_t scaledRate(float scale, std::uint32_t sourceRate, bool boost);

// 0x227f4c: the rate stored at voice +0x58 for a requested rate.
std::int32_t storedRate(std::int32_t requested, std::uint32_t sourceRate);

// 0x25c388 (dirty bit 2): (float)rate * doppler / (float)sourceRate, passed to 0x25b3b4 which keeps it at 0 or above.
float pitchOfRate(std::int32_t rate, float doppler, std::uint32_t sourceRate);

// 0x25c4b8..0x25c52c (dirty bit 3, set by the emitter update 0x107fa0): 1 - dot(emitter velocity - listener velocity,
// normalised direction from listener to emitter) * 0.0029113 * scale, at least 0.001. A direction shorter than 2^-23
// becomes (0, 0, 1) (0x257ea0).
float dopplerFactor(const std::array<float, 3>& emitterVelocity, const std::array<float, 3>& listenerVelocity,
                    const std::array<float, 3>& direction, float scale);

// 0x270fd8 stores 1.0f at 0x37636c + 8 at start-up (2e4230); no other writer exists.
constexpr float kDopplerScale = 1.0f;

} // namespace chase::audio

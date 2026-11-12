#pragma once
// The hearts display of a chased target (docs/re/traffic-driver-states.md): 0x230f04(entity) fills the HUD object 0x517f34 with the target's
// portrait and hearts and switches it on or off. VehicleSetChased 0x1afd38 calls it with the car, the vehicle return 0x22b30c with a car whose
// driver is in state 5, VehicleSetRaceTrim 0x26d8c4 and 0x28548c with 0 (off). The draw of the object is not ported (SCRIPT-455). No SDL dependency.
#include <cstdint>
#include <string>

namespace chase::runtime {

enum class TargetHeartsKind {
    None,       // entity 0
    Character,  // 0x245b00: kind 27 or 149, 0x236b6c, or entity word +0xc bit 14
    Vehicle,    // kind 179, 183 or 243
    Other,
};

// What 0x230f04 reads of the entity. A character: hearts = ctrl+0xe0 byte +0x122, most = +0x123, portrait name = word 0 of row ctrl+0x2cc of the
// characters table 0x372e10 (40-byte rows). A vehicle: hearts = record +0x30d, most = +0x30e, portrait name = word 0 of row hword +0x2ec of the
// vehicle table 0x372e14 (24-byte rows).
struct TargetHeartsSource {
    TargetHeartsKind kind = TargetHeartsKind::None;
    std::string entity;
    std::uint8_t hearts = 0;
    std::uint8_t most = 0;
    std::string portraitName;
};

constexpr const char* kTargetPortraitFormat = "Sprites/portraits/%s.png";  // 0x2310d4
constexpr std::uint16_t kTargetHeartsNoLimit = 255;
constexpr float kTargetHeartsShowTime = -1.0f;  // 0x2310f8: s0 of 0x23cc00(object, 0, s0)

// The fields of the HUD object 0x517f34 that 0x230f04 writes, and the global 0x375734.
struct TargetHeartsDisplay {
    bool shown = false;             // 0x23cc00(object, 0, -1.0) and 0x24581c(object + 0x2c, 1, 0, 1); off: 0x2459d8(object), 0x24581c(.., 0, 0, 1)
    std::uint16_t previous = 0;     // +0x8c
    std::uint16_t hearts = 0;       // +0x8e
    std::uint16_t most = 0;         // +0x90
    float changed = 0.0f;           // +0x84: 1.0 when the hearts changed (or most is 255), else 0.0
    float unchanged = 0.0f;         // +0x88: the other of the two
    std::string portrait;           // the sprite +0x48, loaded by 0x225520 with the formatted path (only a character or a vehicle)
    std::string target;             // 0x375734
};

// 0x230f04(entity). The sprite +0x48 is taken as present (the load is skipped without it).
void setTargetHearts(TargetHeartsDisplay& display, const TargetHeartsSource& source);

}

#pragma once
// The switch base class (docs/re/switches.md): the record at entity +0x7c that the loader 0x23e84c fills for kinds 38, 58, 100, 125, 138, 139, 141, 185,
// 186, 201, 222, 227 and 232, its stable-state update 0x21435c (run by 0x2424d0 while state == target), the state change 0x22c6d0 and the base message
// handler 0x23aac0. Kind 38 (lightonoff) is exactly this base; the other kinds add their own update and handler around it. No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// The state halfword +2 and the target +4: 0 off, 1 used (the use animation runs), 2 on until the reset timer ends, 3 on for good, 4 on until the
// reset timer ends (AlwaysReset), 5 switching off.
enum class SwitchState : std::uint16_t { Off = 0, Used = 1, On = 2, Held = 3, Resetting = 4, SwitchingOff = 5 };

constexpr std::uint8_t kSwitchMessageRestorePose = 6;
constexpr std::uint8_t kSwitchMessageReset = 8;
constexpr std::uint8_t kSwitchMessageUse = 11;
constexpr std::uint8_t kSwitchMessageState = 25;
constexpr std::uint8_t kSwitchMessageOnAtOnce = 54;
constexpr std::uint8_t kSwitchMessageOn = 0xff;
constexpr std::uint8_t kSwitchMessageOff = 0xfe;

// The parameters of the loader 0x23e84c (record offsets in the comments) and the two the state change reads by name.
struct SwitchParams {
    float resetTime = 0.0f;           // ResetTime +0x1c (default 0); -1 sets bit 2 of +0x24: the use ends in state 3, not 2
    bool startOn = false;             // StartOn, bit 3
    bool loopedUseAnim = false;       // LoopedUseAnim, bit 4: switching off does not play the use animation backwards
    bool triggerBeforeAnim = false;   // TriggerBeforeAnim, bit 5: state 1 ends without waiting for the use animation
    bool alwaysReset = false;         // AlwaysReset, bit 6: message 0xff in state 2 goes to 4 (timed) instead of 3
    bool disableAfterUse = false;     // DisableAfterUse, bit 7
    std::uint16_t sfxOn = 0;          // ATTR_SFX_ON +0x26
    std::uint16_t sfxOff = 0;         // ATTR_SFX_OFF +0x28
    std::uint16_t sfxUse = 0;         // ATTR_SFX_USE +0x34
    float useSoundDelay = 0.0f;       // USE_SoundDelay +0x3c
    float numberOfTurns = 1.0f;       // NumberOfTurns (0x241280, fallback 1): the use animation starts only for 1
    bool dontCutUseSound = false;     // DontCutUseSound (0x2451d0, fallback 0)
    bool hasUseAnim = false;          // +0x18: the model's animation "use", loaded by the base link 0x23e5b0
    bool neverReset() const { return resetTime == -1.0f; }
};
// 0x2451d0(entity, name, fallback): a float parameter is cast to an integer, types 5 and 7 return the stored word, anything else the fallback.
std::int32_t switchIntParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback);
SwitchParams readSwitchParams(const assets::LevelObject& object, const assets::Level& cls);

struct SwitchRecord {
    SwitchState state = SwitchState::Off;   // +2
    SwitchState target = SwitchState::Off;  // +4
    bool useRequested = false;              // +0x24 bit 0
    bool usedByOther = false;               // +0x24 bit 1: the use message came from someone other than the player
    float resetTimer = 0.0f;                // +0x20
    float useSoundTimer = 0.0f;             // +0x38
};

enum class SwitchEffectKind {
    StopUseSound,     // 0x23ae50(ATTR_SFX_USE, -1) when state 1 is left
    UseAnimForward,   // 0x23afc0(entity, +0x18, 0, 0, ...): target 1
    UseAnimBackward,  // 0x23afc0(entity, +0x18, 2, ...): target 5 without LoopedUseAnim
    UseAnimToStart,   // 0x23afc0(entity, +0x18, 0, 0, 0, ...): state 5 ends
    UseAnimAtEnd,     // 0x23afc0(entity, +0x18, 0, last frame, ...): messages 6 and 54, the link of a switch that starts on
    UseSound,         // 0x2a27d8: ATTR_SFX_USE unless it already plays
    LightsOn,         // 0x21936c(entity, 1, 0): nodes light_on shown, light_off hidden
    LightsOff,
    SoundOn,          // ATTR_SFX_ON (0x23ae84)
    SoundOff,         // ATTR_SFX_OFF
    StudDrop,         // 0x22ca88(entity, 1, 0, 0, 1): StudValue
    DisableAfterUse,  // 0x229e54 -> 0x21b7cc(scene, node, 1, entity)
};
struct SwitchEffect {
    SwitchEffectKind kind = SwitchEffectKind::UseSound;
    std::uint16_t sound = 0;
};

// 0x242494 (base post-link): StartOn makes state and target 0, the reset timer ResetTime and the reference record Registered (returned true).
bool switchPostLink(SwitchRecord& record, const SwitchParams& params);

// 0x21435c, run while state == target. `useAnimFinished` is 0x245f84(+0x18) == 6 (the animation ended); true when the switch has no use animation.
void switchStableUpdate(SwitchRecord& record, const SwitchParams& params, bool useAnimFinished, std::vector<SwitchEffect>& effects);

// 0x22c6d0. `reference(true)` / `reference(false)` is the call of the step 0x23211c on the record +0xc (activate on a change to 2 or 3 from a state
// other than 2, deactivate on 5); it may send messages back to this switch (message 0xff from a fired join), which change `record.target` before
// state takes it (0x22c998). The timers run after the change with `dt` (0x23602c).
void switchStateChange(SwitchRecord& record, const SwitchParams& params, float dt, const std::function<void(bool)>& reference,
                       std::vector<SwitchEffect>& effects);

// 0x23aac0: the base message handler. `fromPlayer` is the message argument +0 compared with the player (0x3772c0) for message 11, `argument` the
// float +4 (message 11 is taken only with 1.0). Returns what the handler returns (message 25: the state).
struct SwitchMessageResult {
    std::uint32_t returned = 0;
    bool setRegistered = false;     // message 54: the reference record's kind becomes Registered (byte +0xc = 1)
    std::vector<SwitchEffect> effects;
};
// `referenceKind` is the kind of the record +0xc (0 idle, 1 registered, 2 fired) for message 8.
SwitchMessageResult switchBaseMessage(SwitchRecord& record, const SwitchParams& params, std::uint8_t message, bool fromPlayer, float argument,
                                      std::uint8_t referenceKind);

}

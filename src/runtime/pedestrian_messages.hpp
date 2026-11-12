#pragma once
// The message slot 0x19efbc of the pedestrian (kind 197, docs/re/pedestrians-character.md): the same handler as the other characters. It
// requests no dodge, splat or bump state: those come from the blocked test 0x1ffcd4 (pedestrian_blocked). No SDL dependency.
#include "runtime/pedestrian_walk.hpp"
#include "runtime/sound_events.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kPedestrianMessageAnimationEvent = 0x80000001;
constexpr std::uint32_t kPedestrianMessageLeftUpdateList = 0x80000003;
constexpr std::uint32_t kPedestrianMessageFootstep = 50;
constexpr std::uint32_t kPedestrianMessagePreload = 252;

// Byte +0x20 of the kind 197 descriptor built at 0x17868c: 0x22d480 sends 0x80000003 only for a type whose byte is not 0.
constexpr bool kPedestrianHearsUpdateListLeave = false;

struct PedestrianVoice {
    const CharacterSounds* sounds = nullptr;  // the row of byte +0x61 in the character sound table behind 0x375f7c
    std::array<float, 3> listener{};          // 0x227f40() + 0x30: the translation of the listener matrix 0x52ac10
    std::array<float, 3> position{};          // the translation of the entity's node
    std::uint16_t owner = 0;                  // entity hword +0x14
    // Byte +0xb of the 16-byte row of the sound id in the table at *0x375f40 (the hearing distance).
    std::function<std::uint8_t(std::uint16_t soundId)> hearingDistance;
};

struct PedestrianMessageResult {
    bool handled = false;
    std::optional<SoundCall> sound;
    std::vector<std::uint16_t> preload;
};

// 0x80000003: hold 0; state and requested state 0 when either is 1.
PedestrianMessageResult pedestrianLeftUpdateList(PedestrianBody& body);

// Message 50 (a footstep of the walk clip): the footstep slot (5) of the voice row plays at the entity when the squared distance
// between the listener and the node is below the squared hearing distance of that sound.
PedestrianMessageResult pedestrianFootstepMessage(const PedestrianVoice& voice);

// Message 252: the 34 sound ids of the voice row, in slot order, each handed to the callback in the message data.
PedestrianMessageResult pedestrianPreloadMessage(const PedestrianVoice& voice);

// 0x19efbc for the messages 0x80000003, 50 and 252; any other message (0x80000001 is the animation sound events of ClipSounds and
// FootstepDetector) is not handled here.
PedestrianMessageResult pedestrianMessage(PedestrianBody& body, std::uint32_t message, const PedestrianVoice& voice);

}

// The pedestrian's message handler 0x19efbc (docs/re/pedestrians-character.md). Provenance of the pinned values, all from llvm-objdump
// of artifacts/game/game.arm.elf (20261112-traffic-F, 2026-11-12); the sound ids and distances are made up.
// 0x80000003 (0x19f004): hold = 0, state = requested = 0 when either is 1. Message 50 (0x19f02c): the id is the hword at +10 of the
// row byte +0x61 x 0x44 of the table behind 0x375f7c (slot 5); the squared distance 0x24ef70 of the node and the listener matrix
// 0x52ac10 +0x30 (0x227f40) must be below the squared byte +0xb of the row id of the table behind 0x375f40; the play is 0x23ae84 with
// the owner hword +0x14. Message 252 (0x19f3b0): 34 calls of the callback in the message data with the row's hwords in order.
// The descriptor builder 0x17868c stores 0 at sp +0x20, the word 0x246240 copies into the type record +0x20 that 0x22d480 tests before
// it sends 0x80000003 (0x22d50c, the only sender of that literal in the image).
#include "runtime/pedestrian_messages.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

CharacterSounds row() {
    std::array<std::uint16_t, chase::assets::kCharacterSoundSlots> slots{};
    for (std::size_t i = 0; i < slots.size(); ++i) slots[i] = static_cast<std::uint16_t>(1000 + i);
    return CharacterSounds(slots);
}

PedestrianVoice voice(const CharacterSounds& sounds, float listenerX) {
    PedestrianVoice result;
    result.sounds = &sounds;
    result.listener = {listenerX, 0, 0};
    result.position = {0, 0, 0};
    result.owner = 77;
    result.hearingDistance = [](std::uint16_t id) { return static_cast<std::uint8_t>(id == 1005 ? 30 : 0); };
    return result;
}

void leftUpdateList() {
    PedestrianBody body;
    body.hold = 40;
    body.state = 2;
    body.requested = 2;
    const PedestrianMessageResult result = pedestrianMessage(body, kPedestrianMessageLeftUpdateList, {});
    require(result.handled && !result.sound && result.preload.empty(), "message 0x80000003 handled without a sound");
    require(body.hold == 0 && body.state == 2 && body.requested == 2, "state 2 stays");
    body.state = 1;
    body.requested = 2;
    pedestrianMessage(body, kPedestrianMessageLeftUpdateList, {});
    require(body.state == 0 && body.requested == 0, "state 1 resets both");
    body.state = 4;
    body.requested = 1;
    pedestrianMessage(body, kPedestrianMessageLeftUpdateList, {});
    require(body.state == 0 && body.requested == 0, "requested 1 resets both");
    require(!kPedestrianHearsUpdateListLeave, "kind 197 never gets 0x80000003 from the list");
}

void footstep() {
    const CharacterSounds sounds = row();
    PedestrianBody body;
    const PedestrianMessageResult near = pedestrianMessage(body, kPedestrianMessageFootstep, voice(sounds, 29.0f));
    require(near.handled && near.sound && near.sound->id == 1005 && near.sound->owner == 77, "footstep within 30 plays slot 5");
    require(!near.sound->onlyIfIdle, "plain play");
    const PedestrianMessageResult edge = pedestrianMessage(body, kPedestrianMessageFootstep, voice(sounds, 30.0f));
    require(edge.handled && !edge.sound, "at the hearing distance nothing plays");
    const PedestrianMessageResult far = pedestrianMessage(body, kPedestrianMessageFootstep, voice(sounds, -31.0f));
    require(far.handled && !far.sound, "beyond the hearing distance nothing plays");
}

void preload() {
    const CharacterSounds sounds = row();
    PedestrianBody body;
    const PedestrianMessageResult result = pedestrianMessage(body, kPedestrianMessagePreload, voice(sounds, 0.0f));
    require(!result.handled, "message 252 returns 0");
    require(result.preload.size() == 34, "34 slots");
    for (std::size_t i = 0; i < 34; ++i) require(result.preload[i] == 1000 + i, "slot order");
}

void others() {
    PedestrianBody body;
    const CharacterSounds sounds = row();
    require(!pedestrianMessage(body, kPedestrianMessageAnimationEvent, voice(sounds, 0)).handled, "0x80000001 is the animation sounds");
    require(!pedestrianMessage(body, 3, voice(sounds, 0)).handled && !pedestrianMessage(body, 6, voice(sounds, 0)).handled,
            "messages 3, 4 and 6 are not requests of this handler");
}
}

int main() {
    try {
        leftUpdateList();
        footstep();
        preload();
        others();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "pedestrian messages ok\n";
    return 0;
}

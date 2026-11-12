#include "runtime/pedestrian_messages.hpp"

namespace chase::runtime {

PedestrianMessageResult pedestrianLeftUpdateList(PedestrianBody& body) {
    body.hold = 0;
    if (body.state == 1 || body.requested == 1) body.state = body.requested = 0;
    PedestrianMessageResult result;
    result.handled = true;
    return result;
}

PedestrianMessageResult pedestrianFootstepMessage(const PedestrianVoice& voice) {
    PedestrianMessageResult result;
    result.handled = true;
    const std::uint16_t id = voice.sounds->id(kSlotFootstep);
    const PedestrianVec3 listener{voice.listener[0], voice.listener[1], voice.listener[2]};
    const PedestrianVec3 position{voice.position[0], voice.position[1], voice.position[2]};
    const float distance = pedestrianDistanceSquared(position, listener);
    const float reach = static_cast<float>(voice.hearingDistance(id));
    if (reach * reach > distance) result.sound = SoundCall{id, voice.position, voice.owner, false};
    return result;
}

PedestrianMessageResult pedestrianPreloadMessage(const PedestrianVoice& voice) {
    PedestrianMessageResult result;
    for (std::size_t slot = 0; slot < assets::kCharacterSoundSlots; ++slot) result.preload.push_back(voice.sounds->id(slot));
    return result;
}

PedestrianMessageResult pedestrianMessage(PedestrianBody& body, std::uint32_t message, const PedestrianVoice& voice) {
    switch (message) {
    case kPedestrianMessageLeftUpdateList: return pedestrianLeftUpdateList(body);
    case kPedestrianMessageFootstep: return pedestrianFootstepMessage(voice);
    case kPedestrianMessagePreload: return pedestrianPreloadMessage(voice);
    default: return {};
    }
}

}

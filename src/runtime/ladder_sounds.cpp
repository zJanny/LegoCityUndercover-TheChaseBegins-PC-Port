#include "runtime/ladder_sounds.hpp"
#include <cstring>

namespace chase::runtime {
namespace {
float readFloat(const assets::Bytes& data, std::size_t offset) {
    const std::uint32_t bits = assets::readU32(data, offset);
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
}

std::optional<SoundPulseRecord> soundPulseRecord(const assets::Bytes& data) {
    if (data.size() < 17) return std::nullopt;
    SoundPulseRecord record;
    record.id = assets::readU32(data, 0);
    record.volume = readFloat(data, 4);
    record.pitch = readFloat(data, 8);
    record.chance = readFloat(data, 12);
    record.flag = data[16];
    return record;
}

std::optional<CharacterSoundRecord> characterSoundRecord(const assets::Bytes& data) {
    if (data.size() < 13) return std::nullopt;
    CharacterSoundRecord record;
    record.slot = assets::readU32(data, 0);
    record.chance = readFloat(data, 4);
    record.volume = readFloat(data, 8);
    record.flag = data[12];
    return record;
}

float stepTrackValue(const std::vector<assets::AnimTrack>& tracks, std::uint32_t hash, float normalisedTime) {
    for (const assets::AnimTrack& track : tracks)
        if (track.hash == hash) {
            float value = 0.0f;
            for (const assets::CurveKey& key : track.keys)
                if (key.time <= normalisedTime) value = key.value;
            return value;
        }
    return 0.0f;
}

void ClipSounds::sample(const assets::Animation& animation, float normalisedTime) {
    previous_.assign(animation.subAnimations.size(), 0.0f);
    for (std::size_t sub = 0; sub < animation.subAnimations.size(); ++sub) {
        const std::uint32_t aspect = sub < animation.subAspects.size() ? animation.subAspects[sub] : 0;
        const std::uint32_t hash = aspect == kAspectSoundPulse ? kSoundPulseTrackHash : aspect == kAspectCharacterSound ? kCharacterSoundTrackHash : 0;
        if (hash) previous_[sub] = stepTrackValue(animation.subAnimations[sub], hash, normalisedTime);
    }
}

void ClipSounds::restart(const assets::Animation& animation, float normalisedTime) { sample(animation, normalisedTime); }

void ClipSounds::update(const assets::Animation& animation, float normalisedTime, const std::array<float, 3>& position,
                        const CharacterSounds& character, GameRandom& random, std::vector<SoundCall>& plays, std::vector<SoundStop>& stops) {
    if (previous_.size() != animation.subAnimations.size()) {
        sample(animation, normalisedTime);
        return;
    }
    for (std::size_t sub = 0; sub < animation.subAnimations.size(); ++sub) {
        const std::uint32_t aspect = sub < animation.subAspects.size() ? animation.subAspects[sub] : 0;
        if (aspect != kAspectSoundPulse && aspect != kAspectCharacterSound) continue;
        const std::uint32_t hash = aspect == kAspectSoundPulse ? kSoundPulseTrackHash : kCharacterSoundTrackHash;
        const float value = stepTrackValue(animation.subAnimations[sub], hash, normalisedTime);
        const float change = value - previous_[sub];
        previous_[sub] = value;
        const std::uint16_t owner = std::uint16_t(kClipSoundOwnerStandIn + sub);
        if (aspect == kAspectSoundPulse) {
            const std::optional<SoundPulseRecord> record = sub < animation.subData.size() ? soundPulseRecord(animation.subData[sub]) : std::nullopt;
            if (!record || record->id == 0 || record->id > 0xffff) continue;
            const std::uint16_t id = std::uint16_t(record->id);
            if (change > kSoundPulseEdge) {
                if (record->chance < 1.0f && gameRandomUnit(random) >= record->chance) continue;
                SoundCall call{id, position, owner};
                call.onlyIfIdle = looping(id);
                plays.push_back(call);
            } else if (change < -kSoundPulseEdge && looping(id)) {
                stops.push_back({id, owner});
            }
        } else {
            const std::optional<CharacterSoundRecord> record = sub < animation.subData.size() ? characterSoundRecord(animation.subData[sub]) : std::nullopt;
            if (!record) continue;
            const std::size_t slot = record->slot & 0xff;
            const std::uint16_t id = slot < assets::kCharacterSoundSlots ? character.id(slot) : std::uint16_t(0);
            if (change > kCharacterSoundEdge) {
                const float draw = gameRandomUnit(random);
                if (record->chance < 1.0f && draw >= record->chance) continue;
                if (id) plays.push_back({id, position, 0});
            } else if (change < -kCharacterSoundEdge && id && looping(id)) {
                stops.push_back({id, 0});
            }
        }
    }
}

}

#include "audio/soundplayer.hpp"
#include <algorithm>
#include <stdexcept>

namespace chase::audio {
namespace {
EmitterShape shapeOf(const assets::SoundEffect& effect) {
    return {initialVoiceVolume(effect.volumePercent), float(effect.paramA), float(effect.paramB)};
}

const std::array<float, 3> kStaticEmitterVelocityStandIn = {0, 0, 0};
} // namespace

SoundPlayer::SoundPlayer(assets::FuseArchive& archive, Mixer& mixer) : archive_(archive), mixer_(mixer), voices_(mixer) {
    const assets::FuseEntry* entry = archive.find("tables/tablessoundfx.bxls");
    if (!entry) throw std::runtime_error("not in archive: tables/tablessoundfx.bxls");
    const assets::Table table = assets::parseTable(archive.read(*entry));
    effects_ = assets::parseSoundEffects(table);
    tables_ = assets::parseSoundTables(table);
}

std::shared_ptr<const assets::Sound> SoundPlayer::sound(const assets::SoundEffect& effect) {
    auto found = decoded_.find(effect.id);
    if (found != decoded_.end()) return found->second;
    const assets::FuseEntry* entry = archive_.find(assets::soundEffectPath(effect));
    if (!entry) throw std::runtime_error("not in archive: " + assets::soundEffectPath(effect));
    auto decoded = std::make_shared<const assets::Sound>(assets::decodeSound(archive_.read(*entry)));
    decoded_[effect.id] = decoded;
    return decoded;
}

void SoundPlayer::setMasterVolume(float volume) {
    master_ = volume;
    prune();
    for (const Live& voice : live_) applyGains(voice);
}

SoundPlayer::Live* SoundPlayer::live(int voice) {
    prune();
    const auto found = std::find_if(live_.begin(), live_.end(), [&](const Live& v) { return v.voice == voice; });
    return found == live_.end() ? nullptr : &*found;
}

void SoundPlayer::prune() {
    live_.erase(std::remove_if(live_.begin(), live_.end(), [&](const Live& v) { return !mixer_.playing(v.voice); }), live_.end());
}

StereoGains SoundPlayer::gainsOf(const Live& voice) const {
    return voice.positional ? positionalGains(voice.shape, listener_, voice.position, master_) : flatGains(voice.shape, master_);
}

float SoundPlayer::dopplerOf(const Live& voice) const {
    if (!voice.positional) return 1.0f;
    const std::array<float, 3> direction = {voice.position[0] - listener_.position[0], voice.position[1] - listener_.position[1],
                                            voice.position[2] - listener_.position[2]};
    return dopplerFactor(kStaticEmitterVelocityStandIn, listener_.velocity, direction, kDopplerScale);
}

void SoundPlayer::applyGains(const Live& voice) {
    const StereoGains gains = gainsOf(voice);
    mixer_.setGains(voice.voice, gains.left, gains.right);
}

void SoundPlayer::applyPitch(const Live& voice, float doppler) {
    mixer_.setPitch(voice.voice, pitchOfRate(voice.rate, doppler, voice.sourceRate));
}

int SoundPlayer::play(std::uint32_t id, const std::array<float, 3>* position, std::uint16_t owner) {
    const assets::SoundEffect* effect = assets::findSoundEffect(effects_, id);
    if (!effect || effect->name.empty()) return -1;
    const EmitterShape shape = shapeOf(*effect);
    const StereoGains gains = position ? positionalGains(shape, listener_, *position, master_) : flatGains(shape, master_);
    if (position && !(gains.left > 0 || gains.right > 0)) return -1;
    const std::shared_ptr<const assets::Sound> decoded = sound(*effect);
    const int voice = mixer_.play(decoded, gains.left, gains.right);
    prune();
    const std::uint32_t sourceRate = std::uint32_t(decoded->rate);
    const bool boost = (effect->column3 & kRateBoostRecordBit) && rateBoostByteStandIn_;
    live_.push_back({voice, shape, position != nullptr, position ? *position : std::array<float, 3>{0, 0, 0}, sourceRate,
                     storedRate(std::int32_t(boost ? boostedRate(sourceRate) : sourceRate), sourceRate), boost});
    applyPitch(live_.back(), dopplerOf(live_.back()));
    voices_.add(voice, id, owner);
    return voice;
}

bool SoundPlayer::stop(std::uint32_t id, std::uint16_t owner, float fadeSeconds) {
    const assets::SoundEffect* effect = assets::findSoundEffect(effects_, id);
    if (!effect || effect->name.empty()) return false;
    return voices_.stop(id, owner, fadeSeconds < 0.0f ? effect->column16 : fadeSeconds);
}

bool SoundPlayer::setVolume(std::uint32_t id, std::uint16_t owner, float volume) {
    Live* voice = live(voices_.find(id, owner));
    if (!voice) return false;
    voice->shape.volume = volume;
    applyGains(*voice);
    return true;
}

bool SoundPlayer::scaleFrequency(std::uint32_t id, std::uint16_t owner, float scale) {
    Live* voice = live(voices_.find(id, owner));
    if (!voice) return false;
    voice->rate = storedRate(scaledRate(scale, voice->sourceRate, voice->boost), voice->sourceRate);
    applyPitch(*voice, dopplerOf(*voice));
    return true;
}

void SoundPlayer::update() {
    prune();
    for (const Live& voice : live_) {
        if (!voice.positional) continue;
        applyGains(voice);
        applyPitch(voice, dopplerOf(voice));
    }
}

} // namespace chase::audio

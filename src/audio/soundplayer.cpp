#include "audio/soundplayer.hpp"
#include <algorithm>
#include <stdexcept>

namespace chase::audio {
namespace {
EmitterShape shapeOf(const assets::SoundEffect& effect) {
    return {float(effect.volumePercent) * 0.01f, float(effect.paramA), float(effect.paramB)};
}
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

int SoundPlayer::play(std::uint32_t id, const std::array<float, 3>* position, std::uint16_t owner) {
    const assets::SoundEffect* effect = assets::findSoundEffect(effects_, id);
    if (!effect || effect->name.empty()) return -1;
    const EmitterShape shape = shapeOf(*effect);
    const StereoGains gains = position ? positionalGains(shape, listener_, *position, master_) : flatGains(shape, master_);
    if (position && !(gains.left > 0 || gains.right > 0)) return -1;
    const int voice = mixer_.play(sound(*effect), gains.left, gains.right);
    if (position) positional_.push_back({voice, shape, *position});
    voices_.add(voice, id, owner);
    return voice;
}

bool SoundPlayer::stop(std::uint32_t id, std::uint16_t owner, float fadeSeconds) {
    const assets::SoundEffect* effect = assets::findSoundEffect(effects_, id);
    if (!effect || effect->name.empty()) return false;
    return voices_.stop(id, owner, fadeSeconds < 0.0f ? effect->column16 : fadeSeconds);
}

bool SoundPlayer::setVolume(std::uint32_t id, std::uint16_t owner, float volume) {
    const int voice = voices_.find(id, owner);
    return voice >= 0 && mixer_.setVolume(voice, volume);
}

bool SoundPlayer::scaleFrequency(std::uint32_t id, std::uint16_t owner, float scale) {
    const int voice = voices_.find(id, owner);
    const assets::SoundEffect* effect = assets::findSoundEffect(effects_, id);
    if (voice < 0 || !effect) return false;
    const double rate = double(scale) * double(sound(*effect)->rate);
    return mixer_.setRate(voice, rate >= 2147483647.0 ? 2147483647 : rate <= 0.0 ? 0 : int(rate));
}

void SoundPlayer::update() {
    positional_.erase(std::remove_if(positional_.begin(), positional_.end(),
                                     [&](const Positional& p) { return !mixer_.playing(p.voice); }),
                      positional_.end());
    for (const Positional& p : positional_) {
        const StereoGains gains = positionalGains(p.shape, listener_, p.position, master_);
        mixer_.setGains(p.voice, gains.left, gains.right);
    }
}

} // namespace chase::audio

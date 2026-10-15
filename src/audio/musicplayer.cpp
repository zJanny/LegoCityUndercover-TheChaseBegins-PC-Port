#include "audio/musicplayer.hpp"
#include <memory>

namespace chase::audio {

std::string musicPath(const std::string& name) { return name.empty() ? std::string() : "music/" + name + ".bwav"; }

MusicPlayer::MusicPlayer(assets::FuseArchive& archive, Mixer& mixer, std::vector<std::string> trackNames)
    : archive_(archive), mixer_(mixer), names_(std::move(trackNames)) {}

void MusicPlayer::setMasterVolume(float volume) {
    master_ = volume < 0 ? 0 : (volume > 1 ? 1 : volume);
    if (playing()) mixer_.setGains(voice_, volume_ * master_, volume_ * master_);
}

bool MusicPlayer::start(unsigned track, float volume) {
    stop();
    if (track >= names_.size()) return false;
    const std::string path = musicPath(names_[track]);
    if (path.empty()) return false;
    const assets::FuseEntry* entry = archive_.find(path);
    if (!entry) return false;
    volume_ = volume;
    voice_ = mixer_.play(std::make_shared<const assets::Sound>(assets::decodeSound(archive_.read(*entry))), volume_ * master_);
    current_ = track;
    return true;
}

void MusicPlayer::stop() {
    if (voice_ >= 0) mixer_.stop(voice_);
    voice_ = -1;
    current_ = 0;
}

void MusicPlayer::fadeOut(float seconds) {
    if (voice_ < 0) return;
    mixer_.fadeOut(voice_, seconds);
    voice_ = -1;
    current_ = 0;
}

} // namespace chase::audio

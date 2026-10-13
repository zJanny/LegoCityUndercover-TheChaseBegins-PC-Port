#pragma once
// SDL audio output of chase-game: one mixer, pulled by the SDL audio thread and guarded by one lock, with the sound
// effect player and the music player on top. Silent (and reports why) when no audio device can be opened.
#include "assets/assets.hpp"
#include "audio/mixer.hpp"
#include "audio/musicplayer.hpp"
#include "audio/soundplayer.hpp"
#include "runtime/sound_events.hpp"
#include <SDL.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>

class GameAudio {
public:
    GameAudio(chase::assets::FuseArchive& archive, std::uint8_t soundOption, std::uint8_t musicOption) : mixer_(kRate) {
        sounds_.emplace(archive, mixer_);
        music_.emplace(archive, mixer_, sounds_->tables().musicNames);
        sounds_->setMasterVolume(chase::audio::volumeFromOption(soundOption));
        music_->setMasterVolume(chase::audio::volumeFromOption(musicOption));
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            std::cout << "audio: unavailable (" << SDL_GetError() << ")\n";
            return;
        }
        SDL_AudioSpec want{}, have{};
        want.freq = kRate;
        want.format = AUDIO_F32SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = &GameAudio::callback;
        want.userdata = this;
        device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (!device_) {
            std::cout << "audio: unavailable (" << SDL_GetError() << ")\n";
            return;
        }
        SDL_PauseAudioDevice(device_, 0);
    }
    ~GameAudio() {
        if (device_) SDL_CloseAudioDevice(device_);
    }
    GameAudio(const GameAudio&) = delete;
    GameAudio& operator=(const GameAudio&) = delete;

    bool open() const { return device_ != 0; }

    void setListener(const std::array<float, 3>& position, const std::array<float, 3>& forward) {
        std::lock_guard<std::mutex> guard(lock_);
        sounds_->setListener(chase::audio::listenerFromForward(position, forward));
    }
    void play(const chase::runtime::SoundCall& call) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        sounds_->play(call.id, &call.position, call.owner);
        ++calls_;
    }
    void stop(std::uint16_t id, std::uint16_t owner) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        sounds_->stop(id, owner);
    }
    void playUi(unsigned id) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        sounds_->play(id, nullptr);
    }
    void update() {
        std::lock_guard<std::mutex> guard(lock_);
        sounds_->update();
    }
    bool startMusic(unsigned track) {
        if (!device_) return false;
        std::lock_guard<std::mutex> guard(lock_);
        return music_->start(track);
    }
    std::size_t calls() const { return calls_; }

private:
    static constexpr int kRate = 48000;
    static void callback(void* user, Uint8* stream, int bytes) {
        auto* audio = static_cast<GameAudio*>(user);
        std::lock_guard<std::mutex> guard(audio->lock_);
        audio->mixer_.mix(reinterpret_cast<float*>(stream), std::size_t(bytes) / (2 * sizeof(float)));
    }

    chase::audio::Mixer mixer_;
    std::mutex lock_;
    std::optional<chase::audio::SoundPlayer> sounds_;
    std::optional<chase::audio::MusicPlayer> music_;
    SDL_AudioDeviceID device_ = 0;
    std::size_t calls_ = 0;
};

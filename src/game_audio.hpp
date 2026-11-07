#pragma once
// SDL audio output of chase-game: one mixer, pulled by the SDL audio thread and guarded by one lock, with the sound
// effect player and the music player on top. Silent (and reports why) when no audio device can be opened.
#include "assets/assets.hpp"
#include "audio/mixer.hpp"
#include "audio/musicplayer.hpp"
#include "audio/soundplayer.hpp"
#include "runtime/music.hpp"
#include "runtime/music_block.hpp"
#include "runtime/natives_music.hpp"
#include "runtime/natives_sound.hpp"
#include "save/slot_progress.hpp"
#include "runtime/sound_events.hpp"
#include <SDL.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>

class MusicStreamDevice : public chase::runtime::MusicStreams {
public:
    explicit MusicStreamDevice(chase::audio::MusicPlayer& player) : player_(player) {}
    bool open(unsigned track, bool) override { return player_.start(track, 0.0f); }
    void close() override { player_.stop(); }
    void setVolume(float volume) override { player_.setVolume(volume); }
    bool finished() override { return player_.finished(); }

private:
    chase::audio::MusicPlayer& player_;
};

class GameAudio {
public:
    GameAudio(chase::assets::FuseArchive& archive, std::uint8_t soundOption, std::uint8_t musicOption) : mixer_(kRate) {
        sounds_.emplace(archive, mixer_);
        music_.emplace(archive, mixer_, sounds_->tables().musicNames);
        musicStreams_.emplace(*music_);
        musicBlock_.emplace(*musicStreams_);
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
    void applySound(const chase::runtime::SoundRequest& request) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        using Kind = chase::runtime::SoundRequest::Kind;
        const bool ownerFits = request.owner >= 0 && request.owner <= 0xffff;
        const std::uint16_t owner = std::uint16_t(request.owner);
        if (request.kind == Kind::Play) {
            sounds_->play(request.id, request.positional ? &request.position : nullptr, owner);
            ++calls_;
        } else if (ownerFits && request.kind == Kind::Stop) sounds_->stop(request.id, owner, request.value);
        else if (ownerFits && request.kind == Kind::SetVolume) sounds_->setVolume(request.id, owner, request.value);
        else if (ownerFits && request.kind == Kind::SetFrequency) sounds_->scaleFrequency(request.id, owner, request.value);
    }
    void pauseVoices(bool pause) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        if (pause) sounds_->pauseVoices();
        else sounds_->resumeVoices();
    }
    bool playing(std::uint32_t id, std::uint16_t owner) {
        if (!device_) return false;
        std::lock_guard<std::mutex> guard(lock_);
        return sounds_->playing(id, owner);
    }
    // The option gains: sound effects 0x1f4408 -> 0x1dca5c, music 0x1f36ec.
    void setOptionGains(std::optional<float> sound, std::optional<float> music) {
        std::lock_guard<std::mutex> guard(lock_);
        if (sound) sounds_->setMasterVolume(*sound);
        if (music) music_->setMasterVolume(*music);
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
    void applyMusic(const chase::runtime::MusicRequest& request) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        using Kind = chase::runtime::MusicRequest::Kind;
        if (request.kind == Kind::Start) musicBlock_->startTrack(request.track, request.loop, request.crossfade, request.fade);
        else if (request.kind == Kind::StopNow) musicBlock_->stopNow();
        else if (request.kind == Kind::FadeOut) musicBlock_->fadeOut(request.step);
        else musicBlock_->clearOverride();
    }
    void updateMusic(float dt) {
        if (!device_) return;
        std::lock_guard<std::mutex> guard(lock_);
        musicBlock_->update(dt);
    }
    void startLevelMusic(chase::assets::FuseArchive& archive, const std::string& sceneName) {
        if (!device_) return;
        const chase::assets::FuseEntry* entry = archive.find("tables/tableslevels.bxls");
        if (!entry) return;
        const chase::assets::Table levels = chase::assets::parseTable(archive.read(*entry));
        const std::optional<unsigned> row = chase::save::levelIdForScene(levels, sceneName);
        if (!row) return;
        std::lock_guard<std::mutex> guard(lock_);
        musicBlock_->setMusicSet(chase::runtime::musicSetOfLevel(sounds_->tables(), chase::assets::levelMusicSet(levels, *row)));
        musicBlock_->moodRoute(chase::runtime::kLevelStartMood, true, true, chase::runtime::kLevelStartMusicFade);
    }
    int playBuffer(std::shared_ptr<const chase::assets::Sound> sound) {
        if (!device_) return 0;
        std::lock_guard<std::mutex> guard(lock_);
        return mixer_.play(std::move(sound));
    }
    void stopBuffer(int voice) {
        if (!device_ || voice == 0) return;
        std::lock_guard<std::mutex> guard(lock_);
        mixer_.stop(voice);
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
    std::optional<MusicStreamDevice> musicStreams_;
    std::optional<chase::runtime::MusicBlock> musicBlock_;
    SDL_AudioDeviceID device_ = 0;
    std::size_t calls_ = 0;
};

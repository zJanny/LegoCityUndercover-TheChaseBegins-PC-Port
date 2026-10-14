#pragma once
// SDL audio output for the sound track of a movie (queued signed 16-bit stereo PCM).
#include "movie/movie.hpp"
#include <SDL.h>
#include <cstdint>
#include <vector>

class MovieSound {
public:
    MovieSound() {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
        SDL_AudioSpec want{}, have{};
        want.freq = chase::movie::kAudioRate;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (device_) SDL_PauseAudioDevice(device_, 0);
    }
    ~MovieSound() {
        if (device_) SDL_CloseAudioDevice(device_);
    }
    MovieSound(const MovieSound&) = delete;
    MovieSound& operator=(const MovieSound&) = delete;
    void queue(const std::vector<std::uint8_t>& pcm) {
        if (device_ && !pcm.empty()) SDL_QueueAudio(device_, pcm.data(), Uint32(pcm.size()));
    }

private:
    SDL_AudioDeviceID device_ = 0;
};

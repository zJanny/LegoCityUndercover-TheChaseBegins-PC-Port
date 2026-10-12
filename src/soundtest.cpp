// chase-soundtest: plays decoded .bwav sounds through the mixer and SDL audio
// (plan step S4d). Development tool, not gameplay audio routing.
#include "assets/assets.hpp"
#include "audio/mixer.hpp"
#include <SDL.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

struct Output {
    audio::Mixer* mixer;
    std::mutex lock;
};

void callback(void* user, Uint8* stream, int bytes) {
    auto* output = static_cast<Output*>(user);
    std::lock_guard<std::mutex> guard(output->lock);
    output->mixer->mix(reinterpret_cast<float*>(stream), std::size_t(bytes) / (2 * sizeof(float)));
}

int run(int argc, char** argv) {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    double seconds = -1;
    std::vector<std::string> paths;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--fib") fib = value();
        else if (arg == "--names") names = value();
        else if (arg == "--seconds") seconds = std::stod(value());
        else paths.push_back(arg);
    }
    if (paths.empty())
        throw std::runtime_error("usage: chase-soundtest [--fib F] [--names CSV] [--seconds S] sounds/<name>.bwav...");
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    if (SDL_Init(SDL_INIT_AUDIO) != 0) throw std::runtime_error(SDL_GetError());
    SDL_AudioSpec want{}, have{};
    want.freq = 48000;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = callback;
    audio::Mixer mixer(48000);
    Output output{&mixer, {}};
    want.userdata = &output;
    SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!device) throw std::runtime_error(SDL_GetError());
    double longest = 0;
    {
        std::lock_guard<std::mutex> guard(output.lock);
        for (const auto& path : paths) {
            const assets::FuseEntry* entry = archive.find(path);
            if (!entry) throw std::runtime_error("not in archive: " + path);
            auto sound = std::make_shared<assets::Sound>(assets::decodeSound(archive.read(*entry)));
            double length = double(sound->frames) / sound->rate;
            std::cout << path << ": " << sound->rate << " Hz, " << sound->channels.size() << " channel(s), "
                      << length << " s" << (sound->loop ? ", loops from frame " + std::to_string(sound->loopStart) : "")
                      << '\n';
            longest = std::max(longest, sound->loop ? 10.0 : length);
            mixer.play(sound);
        }
    }
    SDL_PauseAudioDevice(device, 0);
    Uint32 deadline = SDL_GetTicks() + Uint32(1000 * (seconds >= 0 ? seconds : longest + 0.2));
    while (SDL_GetTicks() < deadline) {
        {
            std::lock_guard<std::mutex> guard(output.lock);
            if (!mixer.activeVoices()) break;
        }
        SDL_Delay(20);
    }
    SDL_CloseAudioDevice(device);
    SDL_Quit();
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "chase-soundtest: " << error.what() << '\n';
        return 1;
    }
}

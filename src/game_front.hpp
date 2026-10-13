#pragma once
#include "assets/assets.hpp"
#include "frontend/frontend.hpp"
#include "frontend/movie_host.hpp"
#include "frontend/script.hpp"
#include "game_audio.hpp"
#include "input.hpp"
#include "render/scene.hpp"
#include "runtime/scheduler.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>

namespace game_front {

namespace fe = chase::frontend;

class Presenter {
public:
    Presenter() {
        glGenTextures(2, textures_);
        glGenFramebuffers(2, framebuffers_);
        const int widths[2] = {fe::kTopWidth, fe::kBottomWidth};
        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, textures_[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, widths[i], fe::kScreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers_[i]);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures_[i], 0);
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
    ~Presenter() {
        glDeleteFramebuffers(2, framebuffers_);
        glDeleteTextures(2, textures_);
    }
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    void present(const fe::Canvas& top, const fe::Canvas& bottom, int width, int height) {
        upload(0, top);
        upload(1, bottom);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        const float scale = std::min(float(width) / fe::kTopWidth, float(height) / (2 * fe::kScreenHeight));
        const float originX = (float(width) - scale * fe::kTopWidth) / 2, originY = (float(height) - scale * 2 * fe::kScreenHeight) / 2;
        blit(0, originX, originY, scale * fe::kTopWidth, scale * fe::kScreenHeight, fe::kTopWidth, height);
        blit(1, originX + scale * (fe::kTopWidth - fe::kBottomWidth) / 2, originY + scale * fe::kScreenHeight, scale * fe::kBottomWidth,
             scale * fe::kScreenHeight, fe::kBottomWidth, height);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

private:
    void upload(int index, const fe::Canvas& canvas) {
        glBindTexture(GL_TEXTURE_2D, textures_[index]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, canvas.width(), canvas.height(), GL_RGBA, GL_UNSIGNED_BYTE, canvas.rgba().data());
    }
    void blit(int index, float x, float yFromTop, float width, float height, int sourceWidth, int windowHeight) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers_[index]);
        const int left = int(std::lround(x)), right = int(std::lround(x + width));
        const int bottom = windowHeight - int(std::lround(yFromTop + height)), top = windowHeight - int(std::lround(yFromTop));
        glBlitFramebuffer(0, fe::kScreenHeight, sourceWidth, 0, left, bottom, right, top, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    GLuint textures_[2] = {0, 0}, framebuffers_[2] = {0, 0};
};

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

struct Options {
    std::string cutsceneDirectory;
    int language = 1;
    bool firstRun = true;
    std::string script;
    std::string screenshot;
};

inline std::string directoryOf(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}

struct Result {
    bool quit = false;
    bool scriptOnly = false;
    fe::Handoff handoff;
};

inline Result run(SDL_Window* window, chase::InputState& input, chase::assets::FuseArchive& archive, GameAudio* audio,
                  const Options& options, const std::function<std::uint32_t()>& clockNow) {
    fe::Resources resources(archive);
    fe::MoviePictureHost movies(options.cutsceneDirectory);
    fe::FlowConfig config;
    config.language = options.language;
    config.firstRun = options.firstRun;
    fe::FrontEnd front(resources, &movies, config);
    fe::Canvas top(fe::kTopWidth, fe::kScreenHeight), bottom(fe::kBottomWidth, fe::kScreenHeight);
    Presenter presenter;
    MovieSound movieSound;
    int width = 0, height = 0;

    if (!options.script.empty()) {
        fe::ScriptHooks hooks;
        const unsigned updates = fe::runScript(front, options.script, hooks);
        Result result;
        result.handoff = front.flow().handoff();
        if (front.flow().done()) return result;
        front.draw(top, bottom);
        SDL_GL_GetDrawableSize(window, &width, &height);
        presenter.present(top, bottom, width, height);
        glFinish();
        if (!options.screenshot.empty()) chase::render::saveScreenshot(options.screenshot, width, height);
        std::cout << "front end: " << updates << " update(s), screen " << fe::screenName(front.flow().screen()) << ", "
                  << (options.screenshot.empty() ? "" : "saved " + options.screenshot) << '\n';
        SDL_GL_SwapWindow(window);
        result.scriptOnly = true;
        return result;
    }

    chase::runtime::Scheduler scheduler({}, clockNow());
    bool wasUp = false, wasDown = false, wasA = false, wasB = false, wasX = false, wasY = false, wasStart = false;
    bool running = true;
    while (running && !front.flow().done()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: running = false; break;
            case SDL_KEYDOWN: case SDL_KEYUP:
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
                input.event(chase::Source::Key, event.key.keysym.sym, event.type == SDL_KEYDOWN, event.key.repeat != 0);
                break;
            case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
                input.event(chase::Source::Mouse, event.button.button, event.type == SDL_MOUSEBUTTONDOWN);
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) input.reset();
                break;
            default: break;
            }
        }
        scheduler.advance(clockNow(), [&](float dt) {
            using chase::Action;
            const bool up = input.vertical() < 0, down = input.vertical() > 0;
            const bool a = input.active(Action::A), b = input.active(Action::B), x = input.active(Action::X),
                       y = input.active(Action::Y), start = input.active(Action::Start);
            fe::FrontInput pad;
            if (up && !wasUp) pad.pressed |= fe::kButtonUp;
            if (down && !wasDown) pad.pressed |= fe::kButtonDown;
            if (a && !wasA) pad.pressed |= fe::kButtonA;
            if (b && !wasB) pad.pressed |= fe::kButtonB;
            if (x && !wasX) pad.pressed |= fe::kButtonX;
            if (y && !wasY) pad.pressed |= fe::kButtonY;
            if (start && !wasStart) pad.pressed |= fe::kButtonStart;
            wasUp = up; wasDown = down; wasA = a; wasB = b; wasX = x; wasY = y; wasStart = start;
            front.update(dt, pad);
            if (audio)
                for (const int id : front.takeSounds()) audio->playUi(unsigned(id));
            movieSound.queue(front.takeMovieAudio());
        });
        if (audio) audio->update();
        front.draw(top, bottom);
        SDL_GL_GetDrawableSize(window, &width, &height);
        presenter.present(top, bottom, width, height);
        SDL_GL_SwapWindow(window);
    }
    Result result;
    result.quit = !running;
    result.handoff = front.flow().handoff();
    return result;
}

}

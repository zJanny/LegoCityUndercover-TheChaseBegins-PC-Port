// Frame keep (render/frame_keep.hpp) in a real GL 4.6 core context (hidden SDL window). Exit code 77 without a display or GL 4.6.
// Expected values by hand computation, 20261101-qolctl-C, 2026-11-01: without a request the presenters leave the frame as before
// and keep nothing; with a request the frame is kept once at the end of the presenter and drawn back unchanged; a dim of alpha
// 0.72 (spec qol-settings-drawing.md 2026-10-30T21:56:04, answer 29) over a white kept frame reads 255 * (1 - 0.72) = 71.4.
// boxesEndFrame (20261102-qolctl2-F, 2026-11-02): white screens in a full-width top box and a centred half-width panel box leave
// white inside both boxes and black left of the panel (hand computation from the box rectangles).
#include "render/frame_keep.hpp"
#include "render/front_presenter.hpp"
#include "render/hud_overlay.hpp"
#include "render/ui_draw.hpp"
#include "render/window.hpp"
#include "render/window_creation.hpp"
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace {

using namespace chase::render;

constexpr int W = 200, H = 160, ScreenW = 40, ScreenH = 40;
int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

std::vector<std::uint8_t> pattern(int salt) {
    std::vector<std::uint8_t> rgba(std::size_t(ScreenW) * ScreenH * 4);
    for (int y = 0; y < ScreenH; ++y)
        for (int x = 0; x < ScreenW; ++x) {
            std::uint8_t* p = &rgba[(std::size_t(y) * ScreenW + x) * 4];
            p[0] = std::uint8_t(x * 6 + salt);
            p[1] = std::uint8_t(y * 6);
            p[2] = std::uint8_t((x + y) * 3);
            p[3] = 255;
        }
    return rgba;
}

std::vector<std::uint8_t> solid(std::uint8_t value) { return std::vector<std::uint8_t>(std::size_t(ScreenW) * ScreenH * 4, value); }

std::vector<std::uint8_t> present(FrontPresenter& presenter, const std::vector<std::uint8_t>& top, const std::vector<std::uint8_t>& bottom) {
    presenter.present({ScreenW, ScreenH, top.data()}, {ScreenW, ScreenH, bottom.data()}, W, H);
    return readWindowPixels(W, H);
}

bool near(const std::vector<std::uint8_t>& pixels, int x, int y, int value, int tolerance = 2) {
    const std::uint8_t* p = &pixels[(std::size_t(y) * W + x) * 4];
    return std::abs(p[0] - value) <= tolerance && std::abs(p[1] - value) <= tolerance && std::abs(p[2] - value) <= tolerance;
}

void defaultFramesUnchanged() {
    FrontPresenter presenter(ScreenW, ScreenW, ScreenH);
    const std::vector<std::uint8_t> top = pattern(0), bottom = pattern(40);
    const std::vector<std::uint8_t> plain = present(presenter, top, bottom);
    expect(!frameKept() && !frameKeepRequested(), "nothing is kept or requested by default");
    requestFrameKeep();
    const std::vector<std::uint8_t> armed = present(presenter, top, bottom);
    expect(plain == armed, "a frame presented with a request is identical to one without");
    expect(frameKept() && !frameKeepRequested(), "the request is served once at the end of the presenter");
    releaseKeptFrame();
    expect(!frameKept(), "release forgets the frame");
    const std::vector<std::uint8_t> after = present(presenter, top, bottom);
    expect(plain == after && !frameKept(), "after release nothing is kept again until asked");
}

void keptFrameDrawsBack() {
    FrontPresenter presenter(ScreenW, ScreenW, ScreenH);
    const std::vector<std::uint8_t> top = pattern(0), bottom = pattern(40);
    requestFrameKeep();
    const std::vector<std::uint8_t> shown = present(presenter, top, bottom);
    clearWindow({0, 0, 1, 1}, false);
    drawKeptFrame(W, H);
    expect(readWindowPixels(W, H) == shown, "the kept frame is drawn back pixel for pixel");
    releaseKeptFrame();
    clearWindow({0, 0, 0, 1}, false);
    drawKeptFrame(W, H);
    expect(near(readWindowPixels(W, H), 10, 10, 0, 0), "without a kept frame drawing it changes nothing");
}

void dimmedOverKeptFrame() {
    FrontPresenter presenter(ScreenW, ScreenW, ScreenH);
    const std::vector<std::uint8_t> white = solid(255);
    requestFrameKeep();
    present(presenter, white, white);
    clearWindow({0, 0, 0, 1}, false);
    drawKeptFrame(W, H);
    UiDraw draw;
    UiDrawList list;
    list.dim({0, 0, 0, 0.72f});
    draw.draw(buildUiBatch(list, W, H), W, H);
    const std::vector<std::uint8_t> pixels = readWindowPixels(W, H);
    expect(near(pixels, W / 2, H / 4, 71) && near(pixels, W / 2, 3 * H / 4, 71), "a dim of 0.72 over the kept white frame reads 71");
    releaseKeptFrame();
}

void boxesEndFrame() {
    FrontPresenter presenter(ScreenW, ScreenW, ScreenH);
    const std::vector<std::uint8_t> white = solid(255);
    const Pixels image{ScreenW, ScreenH, white.data()};
    const FrontBox topBox{0, 0, float(W), float(H) / 2}, panelBox{float(W) / 4, float(H) / 2, float(W) / 2, float(H) / 2};
    presenter.presentBoxes(image, image, W, H, topBox, panelBox);
    expect(!frameKept(), "presenting boxes keeps nothing without a request");
    requestFrameKeep();
    presenter.presentBoxes(image, image, W, H, topBox, panelBox);
    expect(frameKept() && !frameKeepRequested(), "presenting boxes serves a request at its end");
    const std::vector<std::uint8_t> shown = readWindowPixels(W, H);
    clearWindow({0, 0, 1, 1}, false);
    drawKeptFrame(W, H);
    const std::vector<std::uint8_t> back = readWindowPixels(W, H);
    expect(back == shown, "the kept boxes frame draws back pixel for pixel");
    expect(near(back, W / 2, H / 4, 255) && near(back, W / 2, 3 * H / 4, 255) && near(back, 5, 3 * H / 4, 0, 0),
        "the boxes frame has the top box, the centred panel box and black beside the panel");
    releaseKeptFrame();
}

void overlayEndsFrame() {
    HudOverlay overlay(ScreenW, ScreenW, ScreenH);
    const std::vector<std::uint8_t> white = solid(255);
    const Pixels image{ScreenW, ScreenH, white.data()};
    clearWindow({0, 0, 0, 1}, true);
    overlay.draw(image, image, {0, H / 2, W, H / 2}, {0, 0, W, H / 2}, W, H);
    expect(!frameKept(), "the overlay keeps nothing without a request");
    requestFrameKeep();
    clearWindow({0, 0, 0, 1}, true);
    overlay.draw(image, image, {0, H / 2, W, H / 2}, {0, 0, W, H / 2}, W, H);
    expect(frameKept(), "the overlay serves a request at its end");
    clearWindow({0, 0, 0, 1}, false);
    drawKeptFrame(W, H);
    expect(near(readWindowPixels(W, H), W / 2, H / 4, 255), "the kept overlay frame draws back");
    releaseKeptFrame();
}

}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::printf("skip: no video (%s)\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window* window = SDL_CreateWindow("frame keep window test", 0, 0, W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
    const std::string problem = context ? currentGlContextProblem() : SDL_GetError();
    if (!context || !problem.empty()) {
        std::printf("skip: %s\n", noGlContextMessage(problem.c_str()).c_str());
        if (context) SDL_GL_DeleteContext(context);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    int result = 0;
    try {
        defaultFramesUnchanged();
        keptFrameDrawsBack();
        dimmedOverKeptFrame();
        boxesEndFrame();
        overlayEndsFrame();
        result = failures ? 1 : 0;
    } catch (const std::exception& error) {
        std::printf("FAIL: %s\n", error.what());
        result = 1;
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

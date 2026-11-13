// UiDraw in a real GL 4.6 core context (hidden SDL window), read back at the window's pixel size. Exit code 77 without a display or GL 4.6.
// Expected colours by hand computation, 20261030-qolui2-B, 2026-10-30: opaque fills read back as their byte colour; alpha 0.5 black over
// white (255) gives 127 or 128; a fill outside the scissor box keeps the background.
#include "render/ui_draw.hpp"
#include "render/window.hpp"
#include "render/window_creation.hpp"
#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

using namespace chase::render;

constexpr int W = 200, H = 120;
int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

struct Frame {
    std::vector<std::uint8_t> pixels;
    const std::uint8_t* at(int x, int y) const { return &pixels[(std::size_t(y) * W + x) * 4]; }
    bool is(int x, int y, int r, int g, int b, int tolerance = 2) const {
        const std::uint8_t* p = at(x, y);
        return std::abs(p[0] - r) <= tolerance && std::abs(p[1] - g) <= tolerance && std::abs(p[2] - b) <= tolerance;
    }
};

Frame render(UiDraw& draw, const UiDrawList& list, const ClearColour& background) {
    clearWindow(background, true);
    draw.draw(buildUiBatch(list, W, H), W, H);
    return {readWindowPixels(W, H)};
}

void shapes(UiDraw& draw) {
    UiDrawList list;
    list.fill({10, 10, 40, 30}, {1, 0, 0, 1});
    list.fill({60, 10, 40, 30}, {0, 1, 0, 1}, 8);
    list.border({110, 10, 40, 30}, {0, 0, 1, 1}, 4);
    list.line(10, 60, 90, 60, 4, {1, 1, 0, 1});
    const Frame f = render(draw, list, {0, 0, 0, 1});
    expect(f.is(30, 25, 255, 0, 0), "a filled rectangle reads back as its colour");
    expect(f.is(5, 25, 0, 0, 0) && f.is(30, 45, 0, 0, 0), "outside the rectangle keeps the background");
    expect(f.is(80, 25, 0, 255, 0), "a rounded rectangle is filled in its middle");
    expect(f.is(60, 10, 0, 0, 0) && f.is(99, 39, 0, 0, 0), "the corners of a rounded rectangle are cut");
    expect(f.is(111, 25, 0, 0, 255) && f.is(130, 11, 0, 0, 255), "a border colours its edge");
    expect(f.is(130, 25, 0, 0, 0), "the inside of a border keeps the background");
    expect(f.is(50, 60, 255, 255, 0) && f.is(50, 70, 0, 0, 0) && f.is(50, 50, 0, 0, 0), "a line is as thick as asked");
}

void clipsAndDim(UiDraw& draw) {
    UiDrawList list;
    list.fill({0, 0, W, H}, {1, 1, 1, 1});
    list.pushClip({20, 20, 100, 60});
    list.pushClip({60, 30, 100, 100});
    list.fill({0, 0, W, H}, {1, 0, 0, 1});
    list.popClip();
    list.popClip();
    Frame f = render(draw, list, {0, 0, 0, 1});
    expect(f.is(80, 50, 255, 0, 0), "inside both nested clips is painted");
    expect(f.is(40, 50, 255, 255, 255) && f.is(80, 25, 255, 255, 255) && f.is(130, 50, 255, 255, 255) && f.is(80, 95, 255, 255, 255),
           "outside the intersection of the nested clips keeps the earlier colour");
    list.clear();
    list.fill({0, 0, W, H}, {1, 1, 1, 1});
    list.dim({0, 0, 0, 0.5f});
    f = render(draw, list, {0, 0, 0, 1});
    expect(f.is(0, 0, 128, 128, 128) && f.is(W - 1, H - 1, 128, 128, 128), "the dim overlay covers the whole window with its alpha");
}

int litPixels(const Frame& f, int x0, int y0, int x1, int y1) {
    int count = 0;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            if (!f.is(x, y, 0, 0, 0, 8)) ++count;
    return count;
}

void texts(UiDraw& draw) {
    UiDrawList list;
    list.text("HHHH", 40, 10, 10, {1, 1, 1, 1});
    Frame f = render(draw, list, {0, 0, 0, 1});
    const int inside = litPixels(f, 10, 10, 170, 60), total = litPixels(f, 0, 0, W, H);
    expect(inside > 200, "text lights pixels inside its glyph area");
    expect(total == inside, "text lights nothing outside its glyph area");

    list.clear();
    list.pushClip({10, 10, 30, 60});
    list.text("HHHH", 40, 10, 10, {1, 1, 1, 1});
    list.popClip();
    f = render(draw, list, {0, 0, 0, 1});
    expect(litPixels(f, 10, 10, 40, 70) > 50 && litPixels(f, 0, 0, W, H) == litPixels(f, 10, 10, 40, 70), "a clipped text lights nothing outside its clip");

    list.clear();
    list.fill({0, 0, W, H}, {1, 0, 0, 1});
    list.text("HHHH", 40, 10, 10, {1, 1, 1, 1});
    f = render(draw, list, {0, 0, 0, 1});
    int white = 0;
    for (int y = 10; y < 60; ++y)
        for (int x = 10; x < 170; ++x)
            if (f.is(x, y, 255, 255, 255, 8)) ++white;
    expect(white > 100 && f.is(190, 100, 255, 0, 0), "text is drawn over an earlier fill");

    list.clear();
    list.text("HHHH", 40, 10, 10, {1, 1, 1, 1});
    list.fill({0, 0, W, H}, {1, 0, 0, 1});
    f = render(draw, list, {0, 0, 0, 1});
    expect(litPixels(f, 0, 0, W, H) == W * H && f.is(20, 30, 255, 0, 0) && f.is(100, 30, 255, 0, 0), "a later fill covers an earlier text");

    list.clear();
    list.fill({0, 0, 100, H}, {0, 0, 1, 1});
    list.text("HHHH", 40, 10, 10, {1, 1, 1, 1});
    list.fill({0, 40, W, 40}, {0, 1, 0, 1});
    f = render(draw, list, {0, 0, 0, 1});
    expect(f.is(50, 60, 0, 255, 0) && f.is(150, 60, 0, 255, 0) && f.is(50, 100, 0, 0, 255), "a fill after a text of the same clip keeps the draw-list order");
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
    SDL_Window* window = SDL_CreateWindow("ui draw window test", 0, 0, W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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
        UiDraw draw;
        shapes(draw);
        clipsAndDim(draw);
        texts(draw);
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

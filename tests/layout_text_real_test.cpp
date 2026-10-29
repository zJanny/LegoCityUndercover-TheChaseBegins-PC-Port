// Layout text drawn at the node spacing 0 and glyph scale 1 (0x2b4dc4, docs/re/ui-layouts.md): the lit extent of four texts against the
// original. Provenance of the pinned values: 20261029-qolmenu-E, 2026-10-29, columns whose smallest colour channel is at least 200 in the
// screenshots of RPC 20261029-qolmenu-A-2, recorded from save states (the helipad super build-it prompt with enough super bricks on the top
// screen; the pause Options page with the music at 5 on the bottom screen). Needs the extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/resources.hpp"
#include "frontend/super_build_prompt.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr int kLit = 200;

std::pair<int, int> litColumns(const chase::frontend::Canvas& canvas, int x0, int y0, int x1, int y1) {
    int first = -1, last = -1;
    for (int x = x0; x < x1; ++x)
        for (int y = y0; y < y1; ++y) {
            const std::uint8_t* p = canvas.pixel(x, y);
            if (p[0] >= kLit && p[1] >= kLit && p[2] >= kLit) {
                if (first < 0) first = x;
                last = x;
                break;
            }
        }
    return {first, last};
}

struct Measured {
    const char* text;
    int width;
};

constexpr Measured kOriginal[] = {
    {"Build", 28},     // top screen columns 222..249
    {"Paused", 44},    // bottom screen columns 46..89 of the stacked screenshot
    {"Options", 47},   // 47..93
    {"Music: 5", 53},  // 172..224
};

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: layout-text-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "layout-text-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        frontend::Resources resources(archive);
        const frontend::BitmapFont& font = resources.font("frontendfont");
        const int spacing = frontend::LayoutScene{}.textSpacing;
        for (const Measured& measured : kOriginal) {
            frontend::Canvas canvas(200, 40);
            canvas.clear(0, 0, 0);
            frontend::drawText(canvas, font, resources.extendedCharacters(), measured.text, 10, 8, frontend::Align::Left,
                               frontend::Colour{1, 1, 1, 1}, spacing);
            const auto [first, last] = litColumns(canvas, 0, 0, canvas.width(), canvas.height());
            const int width = last - first + 1;
            std::cout << measured.text << ": " << width << '\n';
            require(width == measured.width, std::string(measured.text) + " is " + std::to_string(width) + " lit pixels wide, the original " +
                                                 std::to_string(measured.width));
        }

        frontend::SuperBuildPromptView view;
        view.name = "Helipad";
        view.cost = "300";
        view.prompt = "Build";
        view.enough = true;
        view.on = true;
        view.time = 10;
        frontend::Canvas top(frontend::kTopWidth, frontend::kScreenHeight);
        top.clear(0, 0, 0);
        frontend::drawSuperBuildPrompt(top, resources, view);
        const auto [first, last] = litColumns(top, 215, 205, 265, 232);
        std::cout << "Build prompt columns " << first << ".." << last << '\n';
        require(first == 222 && last == 249, "the prompt text covers the original's columns 222..249");
    } catch (const std::exception& error) {
        std::cerr << "layout-text-real-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

// chase-hudshot: draws the in-game HUD offscreen for a scripted sequence and saves the transparent top-screen canvas as a
// BMP over black plus an alpha mask BMP. Script: "run N; hearts N; studs TYPE TOTAL; shot NAME; map LEVEL X Z CAMERAYAW PLAYERYAW NAME" (updates are 1/30 s).
#include "frontend/hud.hpp"
#include "frontend/minimap.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;

struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string script, output = ".";
    double spinPerUpdate = 1.0 / 30;
};

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--fib") options.fib = value();
        else if (arg == "--names") options.names = value();
        else if (arg == "--script") options.script = value();
        else if (arg == "--output") options.output = value();
        else throw std::runtime_error("usage: chase-hudshot --script \"run 60; studs 0 10; run 10; shot a\" [--output DIR] [--fib F] [--names CSV]");
    }
    if (options.script.empty()) throw std::runtime_error("--script is required");
    return options;
}

void write(const Canvas& canvas, const std::string& path, bool mask) {
    Canvas out(canvas.width(), canvas.height());
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x) {
            const std::uint8_t* in = canvas.pixel(x, y);
            std::uint8_t* target = out.pixel(x, y);
            for (int c = 0; c < 3; ++c) target[c] = mask ? in[3] : std::uint8_t(in[c] * in[3] / 255);
        }
    out.writeBmp(path);
}

int run(const Options& options) {
    chase::assets::FuseArchive archive(options.fib);
    archive.loadNames(options.names);
    Resources resources(archive);
    Hud hud(resources, {});
    Canvas top(kTopWidth, kScreenHeight);
    double spin = 0;
    std::stringstream script(options.script);
    std::string step;
    while (std::getline(script, step, ';')) {
        std::stringstream fields(step);
        std::string command;
        fields >> command;
        if (command.empty()) continue;
        if (command == "run") {
            unsigned count = 0;
            fields >> count;
            for (unsigned i = 0; i < count; ++i) {
                hud.update(1.0f / 30);
                spin += options.spinPerUpdate;
            }
        } else if (command == "hearts") {
            unsigned hearts = 0;
            fields >> hearts;
            hud.setHearts(hearts);
        } else if (command == "studs") {
            unsigned type = 0;
            std::uint64_t total = 0;
            fields >> type >> total;
            hud.studsBanked(type, total);
        } else if (command == "map") {
            std::string level, name;
            MinimapView view;
            fields >> level >> view.x >> view.z >> view.cameraYaw >> view.playerYaw >> name;
            Canvas bottom(kBottomWidth, kScreenHeight);
            drawMinimap(bottom, resources, level, view);
            bottom.writeBmp(options.output + "/" + name + ".bmp");
        } else if (command == "shot") {
            std::string name;
            fields >> name;
            top.clearTransparent();
            hud.draw(top, spin);
            write(top, options.output + "/" + name + ".bmp", false);
            write(top, options.output + "/" + name + "-mask.bmp", true);
            const StudSink sink = hud.studSink();
            std::cout << name << ": hearts " << hud.hearts() << ", studs " << hud.studDisplay() << ", bar state " << int(hud.studBarState())
                      << ", sink " << sink.x << ' ' << sink.y << '\n';
        } else {
            throw std::runtime_error("unknown command: " + command);
        }
    }
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        return run(parse(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "chase-hudshot: " << error.what() << '\n';
        return 1;
    }
}

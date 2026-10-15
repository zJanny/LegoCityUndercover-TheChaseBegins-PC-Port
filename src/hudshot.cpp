// chase-hudshot: draws the in-game HUD offscreen for a scripted sequence and saves the transparent top-screen canvas as a
// BMP over black plus an alpha mask BMP. Script: "run N; hearts N; studs TYPE TOTAL; shot NAME; marker X Z; ring SECONDS; map LEVEL X Z CAMERAYAW PLAYERYAW NAME" (updates are 1/30 s).
#include "frontend/hud.hpp"
#include "frontend/minimap.hpp"
#include "strict_parse.hpp"
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::frontend;

struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string script, output = ".";
    double spinPerUpdate = 1.0 / 30;
};

constexpr long long kMaxRunUpdates = 1000000;
constexpr int kMapSettleUpdates = 45;

struct Command {
    enum class Kind { Run, Hearts, Studs, Map, Shot, Marker, Ring };
    Kind kind = Kind::Run;
    unsigned count = 0;
    unsigned type = 0;
    std::uint64_t total = 0;
    float x = 0, z = 0, seconds = 0;
    std::string level, name;
    MinimapView view;
};

void requireFields(const std::vector<std::string>& fields, std::size_t count, const char* grammar, const std::string& text) {
    if (fields.size() != count) strict::reject(std::string("expected ") + grammar, text);
}

Command parseCommand(const std::string& text) {
    const std::vector<std::string> fields = strict::words(text);
    if (fields.empty()) strict::reject("empty command", text);
    const std::string& verb = fields[0];
    Command command;
    if (verb == "run") {
        requireFields(fields, 2, "run N", text);
        command.kind = Command::Kind::Run;
        command.count = unsigned(strict::integer(fields[1], 0, kMaxRunUpdates));
    } else if (verb == "hearts") {
        requireFields(fields, 2, "hearts N", text);
        command.kind = Command::Kind::Hearts;
        command.count = unsigned(strict::integer(fields[1], 0, kHudHeartSlots));
    } else if (verb == "studs") {
        requireFields(fields, 3, "studs TYPE TOTAL", text);
        command.kind = Command::Kind::Studs;
        command.type = unsigned(strict::integer(fields[1], 0, 3));
        command.total = std::uint64_t(strict::integer(fields[2], 0, std::numeric_limits<long long>::max()));
    } else if (verb == "map") {
        requireFields(fields, 7, "map LEVEL X Z CAMERAYAW PLAYERYAW NAME", text);
        command.kind = Command::Kind::Map;
        command.level = fields[1];
        command.view.x = float(strict::real(fields[2], -strict::kAnyFloat, strict::kAnyFloat));
        command.view.z = float(strict::real(fields[3], -strict::kAnyFloat, strict::kAnyFloat));
        command.view.cameraYaw = float(strict::real(fields[4], -strict::kAnyFloat, strict::kAnyFloat));
        command.view.playerYaw = float(strict::real(fields[5], -strict::kAnyFloat, strict::kAnyFloat));
        command.name = fields[6];
    } else if (verb == "marker") {
        requireFields(fields, 3, "marker X Z", text);
        command.kind = Command::Kind::Marker;
        command.x = float(strict::real(fields[1], -strict::kAnyFloat, strict::kAnyFloat));
        command.z = float(strict::real(fields[2], -strict::kAnyFloat, strict::kAnyFloat));
    } else if (verb == "ring") {
        requireFields(fields, 2, "ring SECONDS", text);
        command.kind = Command::Kind::Ring;
        command.seconds = float(strict::real(fields[1], 0.0, kRingCycleSeconds));
    } else if (verb == "shot") {
        requireFields(fields, 2, "shot NAME", text);
        command.kind = Command::Kind::Shot;
        command.name = fields[1];
    } else {
        strict::reject("unknown command", verb);
    }
    return command;
}

std::vector<Command> parseScript(const std::string& script) {
    std::vector<Command> commands;
    for (const std::string& text : strict::split(script, ';')) commands.push_back(parseCommand(text));
    return commands;
}

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() { return strict::nextValue(argc, argv, i); };
        strict::forOption(arg, [&] {
            if (arg == "--fib") options.fib = value();
            else if (arg == "--names") options.names = value();
            else if (arg == "--script") {
                options.script = value();
                parseScript(options.script);
            }
            else if (arg == "--output") options.output = value();
            else strict::reject("unknown option", arg);
        });
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
    MapMarkerField markerField;
    std::vector<MinimapMarker> pendingMarkers;
    float ringSeconds = 0;
    for (const Command& command : parseScript(options.script)) {
        if (command.kind == Command::Kind::Run) {
            for (unsigned i = 0; i < command.count; ++i) {
                hud.update(1.0f / 30);
                spin += options.spinPerUpdate;
            }
        } else if (command.kind == Command::Kind::Marker) {
            pendingMarkers.push_back({int(pendingMarkers.size()), command.x, command.z});
        } else if (command.kind == Command::Kind::Ring) {
            ringSeconds = command.seconds;
        } else if (command.kind == Command::Kind::Hearts) {
            hud.setHearts(command.count);
        } else if (command.kind == Command::Kind::Studs) {
            hud.studsBanked(command.type, command.total);
        } else if (command.kind == Command::Kind::Map) {
            Canvas bottom(kBottomWidth, kScreenHeight);
            MinimapView view = command.view;
            view.markers = pendingMarkers;
            for (int update = 0; update < kMapSettleUpdates; ++update) markerField.advance(1.0f / 30, view.x, view.z, view.cameraYaw, view.markers);
            markerField.setRingClock(ringSeconds);
            view.markerField = &markerField;
            drawMinimap(bottom, resources, command.level, view);
            bottom.writeBmp(options.output + "/" + command.name + ".bmp");
        } else {
            top.clearTransparent();
            hud.draw(top, spin);
            write(top, options.output + "/" + command.name + ".bmp", false);
            write(top, options.output + "/" + command.name + "-mask.bmp", true);
            const StudSink sink = hud.studSink();
            std::cout << command.name << ": hearts " << hud.hearts() << ", studs " << hud.studDisplay() << ", bar state " << int(hud.studBarState())
                      << ", sink " << sink.x << ' ' << sink.y << '\n';
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

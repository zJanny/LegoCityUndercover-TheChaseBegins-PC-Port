// chase-hudshot: draws the in-game HUD offscreen for a scripted sequence and saves the transparent top-screen canvas as a
// BMP over black plus an alpha mask BMP. Script: "run N; hearts N; studs TYPE TOTAL; brickcount TOTAL; brick; brickbank TOTAL; shot NAME; marker X Z; ring SECONDS; map LEVEL X Z CAMERAYAW PLAYERYAW NAME;
// pause; press START|A|B|LEFT|RIGHT|UP|DOWN; touch X Y N" (updates are 1/30 s). After `pause` the pause screen runs and `shot` saves both screens (400x480).
#include "frontend/hud.hpp"
#include "frontend/pause_menu.hpp"
#include "frontend/minimap.hpp"
#include "strict_parse.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
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
    enum class Kind { Run, Hearts, Studs, Map, Shot, Marker, Ring, BrickCount, BrickPickup, BrickBank, Pause, Press, Touch };
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
    } else if (verb == "brickcount" || verb == "brickbank") {
        requireFields(fields, 2, "brickcount TOTAL / brickbank TOTAL", text);
        command.kind = verb == "brickcount" ? Command::Kind::BrickCount : Command::Kind::BrickBank;
        command.total = std::uint64_t(strict::integer(fields[1], 0, 4294967295LL));
    } else if (verb == "brick") {
        requireFields(fields, 1, "brick", text);
        command.kind = Command::Kind::BrickPickup;
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
    } else if (verb == "pause") {
        requireFields(fields, 1, "pause", text);
        command.kind = Command::Kind::Pause;
    } else if (verb == "press") {
        requireFields(fields, 2, "press START|A|B|LEFT|RIGHT|UP|DOWN", text);
        command.kind = Command::Kind::Press;
        const std::string buttons[] = {"START", "A", "B", "LEFT", "RIGHT", "UP", "DOWN"};
        const auto found = std::find(std::begin(buttons), std::end(buttons), fields[1]);
        if (found == std::end(buttons)) strict::reject("unknown button", fields[1]);
        command.name = fields[1];
    } else if (verb == "touch") {
        requireFields(fields, 4, "touch X Y N", text);
        command.kind = Command::Kind::Touch;
        command.x = float(strict::real(fields[1], 0.0, double(kBottomWidth - 1)));
        command.z = float(strict::real(fields[2], 0.0, double(kScreenHeight - 1)));
        command.count = unsigned(strict::integer(fields[3], 1, kMaxRunUpdates));
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
    std::optional<PauseMenu> pause;
    const auto pauseStep = [&](const PauseInput& input) {
        pause->update(1.0f / 30, input);
        for (const int id : pause->takeSounds()) std::cout << "pause sound " << id << '\n';
        for (const PauseEntry entry : pause->takeUnported()) std::cout << "pause entry " << int(entry) << " not ported\n";
        if (pause->takeSaveRequest()) {
            std::cout << "pause options save\n";
            pause->saveDone();
        }
    };
    for (const Command& command : parseScript(options.script)) {
        if (command.kind == Command::Kind::Run) {
            for (unsigned i = 0; i < command.count; ++i) {
                hud.update(1.0f / 30);
                spin += options.spinPerUpdate;
                if (pause) pauseStep({});
            }
        } else if (command.kind == Command::Kind::Pause) {
            pause.emplace(pauseMenuSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc"));
            pause->open();
        } else if (command.kind == Command::Kind::Press || command.kind == Command::Kind::Touch) {
            if (!pause) throw std::runtime_error(command.kind == Command::Kind::Press ? "press needs pause first" : "touch needs pause first");
            PauseInput input;
            if (command.kind == Command::Kind::Press) {
                input.start = command.name == "START";
                input.a = command.name == "A";
                input.b = command.name == "B";
                input.left = command.name == "LEFT";
                input.right = command.name == "RIGHT";
                input.up = command.name == "UP";
                input.down = command.name == "DOWN";
                pauseStep(input);
            } else {
                input.touch = {true, command.x, command.z};
                for (unsigned i = 0; i < command.count; ++i) pauseStep(input);
                pauseStep({});
            }
        } else if (command.kind == Command::Kind::Marker) {
            pendingMarkers.push_back({int(pendingMarkers.size()), command.x, command.z});
        } else if (command.kind == Command::Kind::Ring) {
            ringSeconds = command.seconds;
        } else if (command.kind == Command::Kind::Hearts) {
            hud.setHearts(command.count);
        } else if (command.kind == Command::Kind::Studs) {
            hud.studsBanked(command.type, command.total);
        } else if (command.kind == Command::Kind::BrickCount) {
            hud.setSuperBrickCount(std::uint32_t(command.total));
        } else if (command.kind == Command::Kind::BrickPickup) {
            hud.superBrickPickedUp();
        } else if (command.kind == Command::Kind::BrickBank) {
            hud.superBrickBanked(std::uint32_t(command.total));
        } else if (command.kind == Command::Kind::Map) {
            Canvas bottom(kBottomWidth, kScreenHeight);
            MinimapView view = command.view;
            view.markers = pendingMarkers;
            for (int update = 0; update < kMapSettleUpdates; ++update) markerField.advance(1.0f / 30, view.x, view.z, view.cameraYaw, view.markers);
            markerField.setRingClock(ringSeconds);
            view.markerField = &markerField;
            drawMinimap(bottom, resources, command.level, view);
            bottom.writeBmp(options.output + "/" + command.name + ".bmp");
        } else if (pause) {
            Canvas pauseTop(kTopWidth, kScreenHeight), pauseBottom(kBottomWidth, kScreenHeight);
            pause->drawTop(pauseTop, resources);
            pause->drawBottom(pauseBottom, resources);
            composeScreens(pauseTop, pauseBottom).writeBmp(options.output + "/" + command.name + ".bmp");
            std::cout << command.name << ": pause selected " << pause->round().selected() << ", menu state " << int(pause->round().state())
                      << (pause->finished() ? ", closed" : "")
                      << (pause->round().items().empty() ? std::string() : ", text " + pause->round().items()[std::size_t(pause->round().selected())].text)
                      << '\n';
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

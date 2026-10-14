#include "game_options.hpp"
#include "runtime/pad.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace game_options {
namespace runtime = chase::runtime;

// "x y count [buttons];...": raw circle-pad values (+x right, +y up, 145 = full) held for count updates, with
// optional 3DS button letters (A B X Y L R) held as well.
std::vector<ScriptStep> parseScript(const std::string& text) {
    std::vector<ScriptStep> steps;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, ';')) {
        std::stringstream fields(part);
        ScriptStep step;
        if (!(fields >> step.stickX >> step.stickY >> step.updates)) throw std::runtime_error("bad --script step: " + part);
        std::string letters;
        fields >> letters;
        for (const char letter : letters) {
            const std::pair<char, runtime::PadButton> buttons[] = {{'A', runtime::PadA}, {'B', runtime::PadB}, {'X', runtime::PadX},
                                                                  {'Y', runtime::PadY}, {'L', runtime::PadL}, {'R', runtime::PadR}};
            const auto found = std::find_if(std::begin(buttons), std::end(buttons), [&](const auto& entry) { return entry.first == letter; });
            if (found == std::end(buttons)) throw std::runtime_error("bad button in --script step: " + part);
            step.buttons |= found->second;
        }
        steps.push_back(step);
    }
    return steps;
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--fib") o.fib = value();
        else if (arg == "--names") o.names = value();
        else if (arg == "--config") o.config = value();
        else if (arg == "--level") { o.level = value(); o.gameplay = true; }
        else if (arg == "--gameplay") o.gameplay = true;
        else if (arg == "--front-script") o.frontScript = value();
        else if (arg == "--language") o.language = std::max(1u, std::min(9u, unsigned(std::stoul(value()))));
        else if (arg == "--returning") o.returningPlayer = true;
        else if (arg == "--window") {
            std::stringstream fields(value());
            char separator = 0;
            if (!(fields >> o.windowWidth >> separator >> o.windowHeight) || separator != 'x' || o.windowWidth < 200 || o.windowHeight < 240)
                throw std::runtime_error("bad --window, expected WIDTHxHEIGHT");
        }
        else if (arg == "--screenshot") o.screenshot = value();
        else if (arg == "--frames") o.frames = unsigned(std::stoul(value()));
        else if (arg == "--script") { o.script = value(); o.gameplay = true; }
        else if (arg == "--call-script") o.callScripts.push_back(value());
        else if (arg == "--save") { o.saveFile = value(); o.gameplay = true; }
        else if (arg == "--slot") o.saveSlot = unsigned(std::stoul(value()));
        else if (arg == "--no-camera-collision") o.cameraCollision = false;
        else if (arg == "--sound-volume") o.soundVolume = std::min(10u, unsigned(std::stoul(value())));
        else if (arg == "--music-volume") o.musicVolume = std::min(10u, unsigned(std::stoul(value())));
        else if (arg == "--facing") o.facing = std::stof(value());
        else if (arg == "--start") {
            std::stringstream fields(value());
            std::array<float, 3> position;
            if (!(fields >> position[0] >> position[1] >> position[2])) throw std::runtime_error("bad --start, expected \"X Y Z\"");
            o.start = position;
            o.gameplay = true;
        }
        else throw std::runtime_error("usage: chase-game [--fib F] [--names CSV] [--config CFG] [--gameplay] "
                                      "[--level levels/levels/<name>/<name>_main.blvl] [--screenshot OUT.bmp] [--frames N] "
                                      "[--script \"X Y COUNT [BUTTONS];...\"] [--save savegame.dat] [--slot 0|1] [--start \"X Y Z\"] [--facing DEGREES] [--no-camera-collision] [--sound-volume 0-10] [--music-volume 0-10] "
                                      "[--front-script \"run 5; press A; until slot; ...\"] [--language 1-9] [--returning] [--window WIDTHxHEIGHT]");
    }
    return o;
}

}

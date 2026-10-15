#include "game_options.hpp"
#include "frontend/script.hpp"
#include "runtime/pad.hpp"
#include "strict_parse.hpp"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <limits>
#include <stdexcept>

namespace game_options {
namespace runtime = chase::runtime;

namespace {

constexpr long long kCirclePadLimit = 156;
constexpr long long kMaxCount = 1000000;
constexpr long long kMaxRoomNumber = 99;
constexpr long long kMaxWindowSize = 16384;
constexpr long long kMinWindowWidth = 200, kMinWindowHeight = 240;
constexpr double kAnyRealLow = std::numeric_limits<double>::lowest(), kAnyRealHigh = std::numeric_limits<double>::max();

const char* const kUsage = "usage: chase-game [--fib F] [--names CSV] [--config CFG] [--gameplay] "
                           "[--level levels/levels/<name>/<name>_main.blvl] [--screenshot OUT.bmp] [--frames N] "
                           "[--script \"X Y COUNT [BUTTONS];...\"] [--save savegame.dat] [--save-out FILE] [--front-save savegame.dat] [--slot 0|1] [--start \"X Y Z\"] [--facing DEGREES] [--hide-room N] [--no-camera-collision] [--no-start-cutscenes] [--set-variable NAME=VALUE] [--debug-overlay] [--sound-volume 0-10] [--music-volume 0-10] "
                           "[--front-script \"run 5; press A; until slot; ...\"] [--language 1-9] [--returning] [--window WIDTHxHEIGHT]";

std::uint32_t buttonOf(char letter) {
    const std::pair<char, runtime::PadButton> buttons[] = {{'A', runtime::PadA}, {'B', runtime::PadB}, {'X', runtime::PadX},
                                                          {'Y', runtime::PadY}, {'L', runtime::PadL}, {'R', runtime::PadR}};
    const auto found = std::find_if(std::begin(buttons), std::end(buttons), [&](const auto& entry) { return entry.first == letter; });
    if (found == std::end(buttons)) strict::reject("unknown button", std::string(1, letter));
    return found->second;
}

ScriptStep parseStep(const std::string& text) {
    const std::vector<std::string> fields = strict::words(text);
    if (fields.size() < 3 || fields.size() > 4) strict::reject("expected X Y COUNT [BUTTONS]", text);
    ScriptStep step;
    step.stickX = int(strict::integer(fields[0], -kCirclePadLimit, kCirclePadLimit));
    step.stickY = int(strict::integer(fields[1], -kCirclePadLimit, kCirclePadLimit));
    step.updates = unsigned(strict::integer(fields[2], 0, kMaxCount));
    if (fields.size() == 4)
        for (const char letter : fields[3]) step.buttons |= buttonOf(letter);
    return step;
}

void parseWindow(Options& o, const std::string& text) {
    const std::size_t separator = text.find('x');
    if (separator == std::string::npos) strict::reject("expected WIDTHxHEIGHT", text);
    o.windowWidth = int(strict::integer(text.substr(0, separator), kMinWindowWidth, kMaxWindowSize));
    o.windowHeight = int(strict::integer(text.substr(separator + 1), kMinWindowHeight, kMaxWindowSize));
}

void parseStart(Options& o, const std::string& text) {
    const std::vector<std::string> fields = strict::words(text);
    if (fields.size() != 3) strict::reject("expected X Y Z", text);
    std::array<float, 3> position{};
    for (std::size_t i = 0; i < 3; ++i) position[i] = float(strict::real(fields[i], kAnyRealLow, kAnyRealHigh));
    o.start = position;
    o.gameplay = true;
}

void parseVariable(Options& o, const std::string& text) {
    const std::size_t separator = text.find('=');
    if (separator == std::string::npos) strict::reject("expected NAME=VALUE", text);
    const std::string name = text.substr(0, separator);
    if (name.empty() || name.find_first_of(" \t") != std::string::npos) strict::reject("expected a variable name before '='", text);
    o.variables.emplace_back(name, float(strict::real(text.substr(separator + 1), kAnyRealLow, kAnyRealHigh)));
}

bool applyOption(Options& o, const std::string& arg, const std::function<std::string()>& value) {
    if (arg == "--fib") o.fib = value();
    else if (arg == "--names") o.names = value();
    else if (arg == "--config") o.config = value();
    else if (arg == "--level") { o.level = value(); o.levelGiven = true; o.gameplay = true; }
    else if (arg == "--gameplay") o.gameplay = true;
    else if (arg == "--front-script") {
        const std::string script = value();
        chase::frontend::parseFrontScript(script);
        o.frontScript = script;
    }
    else if (arg == "--language") o.language = unsigned(strict::integer(value(), 1, 9));
    else if (arg == "--returning") o.returningPlayer = true;
    else if (arg == "--window") parseWindow(o, value());
    else if (arg == "--screenshot") o.screenshot = value();
    else if (arg == "--frames") o.frames = unsigned(strict::integer(value(), 0, kMaxCount));
    else if (arg == "--script") {
        const std::string script = value();
        parseScript(script);
        o.script = script;
        o.gameplay = true;
    }
    else if (arg == "--call-script") o.callScripts.push_back(value());
    else if (arg == "--set-variable") parseVariable(o, value());
    else if (arg == "--save") { o.saveFile = value(); o.gameplay = true; }
    else if (arg == "--save-out") o.saveOut = value();
    else if (arg == "--front-save") o.frontSave = value();
    else if (arg == "--slot") o.saveSlot = unsigned(strict::integer(value(), 0, 1));
    else if (arg == "--no-camera-collision") o.cameraCollision = false;
    else if (arg == "--no-start-cutscenes") o.startCutscenes = false;
    else if (arg == "--debug-overlay") o.debugOverlay = true;
    else if (arg == "--sound-volume") o.soundVolume = unsigned(strict::integer(value(), 0, 10));
    else if (arg == "--music-volume") o.musicVolume = unsigned(strict::integer(value(), 0, 10));
    else if (arg == "--hide-room") o.hideRoom = int(strict::integer(value(), 1, kMaxRoomNumber));
    else if (arg == "--facing") o.facing = float(strict::real(value(), kAnyRealLow, kAnyRealHigh));
    else if (arg == "--start") parseStart(o, value());
    else return false;
    return true;
}

}

// "x y count [buttons];...": raw circle-pad values (+x right, +y up, 145 = full) held for count updates, with
// optional 3DS button letters (A B X Y L R) held as well.
std::vector<ScriptStep> parseScript(const std::string& text) {
    std::vector<ScriptStep> steps;
    for (const std::string& part : strict::split(text, ';')) steps.push_back(parseStep(part));
    return steps;
}

void checkSaveOut(const Options& o) {
    if (o.saveOut.empty()) return;
    if (o.saveFile.empty()) throw std::runtime_error("--save-out: needs --save, the slot it writes comes from that file");
    namespace fs = std::filesystem;
    std::error_code ignored;
    const bool same = fs::absolute(o.saveOut).lexically_normal() == fs::absolute(o.saveFile).lexically_normal() ||
                      fs::equivalent(o.saveOut, o.saveFile, ignored);
    if (same)
        throw std::runtime_error("--save-out: must not be the file given with --save: '" + o.saveOut + "'");
}

void checkFrontSave(const Options& o) {
    if (o.frontSave.empty()) return;
    if (o.gameplay) throw std::runtime_error("--front-save: only the front end reads it, not a level run (--gameplay, --script, --level, --start, --save): '" + o.frontSave + "'");
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const std::function<std::string()> value = [&]() -> std::string {
            if (i + 1 >= argc) strict::reject("missing value", "");
            const std::string text = argv[++i];
            if (text.empty()) strict::reject("empty value", text);
            return text;
        };
        bool recognised = false;
        try {
            recognised = applyOption(o, arg, value);
        } catch (const std::runtime_error& error) {
            throw std::runtime_error(arg + ": " + error.what());
        }
        if (!recognised) throw std::runtime_error(arg + ": unknown option: '" + arg + "'\n" + kUsage);
    }
    checkSaveOut(o);
    checkFrontSave(o);
    return o;
}

}

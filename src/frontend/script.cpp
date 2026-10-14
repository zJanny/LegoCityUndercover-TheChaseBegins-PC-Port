#include "frontend/script.hpp"
#include "strict_parse.hpp"
#include <map>
#include <stdexcept>

namespace chase::frontend {

namespace {

constexpr float kMaxScriptSeconds = 3600;
constexpr float kDefaultUntilSeconds = 120;

const std::map<std::string, Screen>& screens() {
    static const std::map<std::string, Screen> table = {
        {"boot", Screen::Boot},         {"logos", Screen::Logos},       {"fusion", Screen::FusionMovie},
        {"language", Screen::Language}, {"prompt", Screen::Prompt},     {"autosave", Screen::Autosave},
        {"slot", Screen::SlotSelect},   {"saving", Screen::SavingGame}, {"loading", Screen::Loading},
        {"intro", Screen::IntroMovie},  {"card", Screen::MissionCard},  {"done", Screen::Done}};
    return table;
}

std::uint32_t buttonOf(const std::string& name) {
    static const std::map<std::string, std::uint32_t> buttons = {
        {"A", kButtonA}, {"B", kButtonB}, {"X", kButtonX}, {"Y", kButtonY}, {"START", kButtonStart}, {"UP", kButtonUp}, {"DOWN", kButtonDown}};
    const auto found = buttons.find(name);
    if (found == buttons.end()) strict::reject("unknown button", name);
    return found->second;
}

void expectFields(const std::vector<std::string>& fields, std::size_t low, std::size_t high, const std::string& text) {
    if (fields.size() < low || fields.size() > high) strict::reject("wrong number of fields", text);
}

}

Screen screenFromName(const std::string& name) {
    const auto found = screens().find(name);
    if (found == screens().end()) strict::reject("unknown screen", name);
    return found->second;
}

const char* screenName(Screen screen) {
    for (const auto& entry : screens())
        if (entry.second == screen) return entry.first.c_str();
    return "?";
}

std::vector<ScriptCommand> parseFrontScript(const std::string& script) {
    std::vector<ScriptCommand> commands;
    for (const std::string& text : strict::split(script, ';')) {
        const std::vector<std::string> fields = strict::words(text);
        if (fields.empty()) strict::reject("empty command", text);
        ScriptCommand command;
        const std::string& verb = fields[0];
        if (verb == "run") {
            expectFields(fields, 2, 2, text);
            command.verb = ScriptCommand::Verb::Run;
            command.seconds = float(strict::real(fields[1], 0, kMaxScriptSeconds));
        } else if (verb == "press") {
            expectFields(fields, 2, 2, text);
            command.verb = ScriptCommand::Verb::Press;
            command.buttons = buttonOf(fields[1]);
        } else if (verb == "until") {
            expectFields(fields, 2, 3, text);
            command.verb = ScriptCommand::Verb::Until;
            command.name = fields[1];
            command.screen = screenFromName(fields[1]);
            command.seconds = fields.size() == 3 ? float(strict::real(fields[2], 0, kMaxScriptSeconds)) : kDefaultUntilSeconds;
        } else if (verb == "shot") {
            expectFields(fields, 2, 2, text);
            command.verb = ScriptCommand::Verb::Shot;
            command.name = fields[1];
        } else {
            strict::reject("unknown command", text);
        }
        commands.push_back(command);
    }
    return commands;
}

unsigned runScript(FrontEnd& front, const std::string& script, const ScriptHooks& hooks) {
    unsigned updates = 0;
    auto step = [&](std::uint32_t pressed) {
        front.update(kScriptStep, FrontInput{pressed});
        ++updates;
    };
    for (const ScriptCommand& command : parseFrontScript(script)) {
        switch (command.verb) {
        case ScriptCommand::Verb::Run:
            for (unsigned i = 0, count = unsigned(command.seconds / kScriptStep + 0.5f); i < count; ++i) step(0);
            break;
        case ScriptCommand::Verb::Press:
            step(command.buttons);
            break;
        case ScriptCommand::Verb::Until:
            for (unsigned i = 0; front.flow().screen() != command.screen; ++i) {
                if (float(i) * kScriptStep > command.seconds)
                    throw std::runtime_error("screen " + command.name + " not reached in " + std::to_string(command.seconds) + " s");
                step(0);
            }
            break;
        case ScriptCommand::Verb::Shot:
            if (hooks.shot) hooks.shot(command.name);
            break;
        }
    }
    return updates;
}

}

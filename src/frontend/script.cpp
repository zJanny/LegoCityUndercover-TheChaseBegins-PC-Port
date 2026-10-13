#include "frontend/script.hpp"
#include <map>
#include <sstream>
#include <stdexcept>

namespace chase::frontend {

namespace {

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
    if (found == buttons.end()) throw std::runtime_error("unknown button " + name);
    return found->second;
}

}

Screen screenFromName(const std::string& name) {
    const auto found = screens().find(name);
    if (found == screens().end()) throw std::runtime_error("unknown screen " + name);
    return found->second;
}

const char* screenName(Screen screen) {
    for (const auto& entry : screens())
        if (entry.second == screen) return entry.first.c_str();
    return "?";
}

unsigned runScript(FrontEnd& front, const std::string& script, const ScriptHooks& hooks) {
    unsigned updates = 0;
    auto step = [&](std::uint32_t pressed) {
        front.update(kScriptStep, FrontInput{pressed});
        ++updates;
    };
    std::stringstream commands(script);
    std::string command;
    while (std::getline(commands, command, ';')) {
        std::stringstream fields(command);
        std::string verb;
        if (!(fields >> verb)) continue;
        if (verb == "run") {
            float seconds = 0;
            fields >> seconds;
            for (unsigned i = 0, count = unsigned(seconds / kScriptStep + 0.5f); i < count; ++i) step(0);
        } else if (verb == "press") {
            std::string name;
            fields >> name;
            step(buttonOf(name));
        } else if (verb == "until") {
            std::string name;
            float limit = 120;
            fields >> name >> limit;
            const Screen wanted = screenFromName(name);
            for (unsigned i = 0; front.flow().screen() != wanted; ++i) {
                if (float(i) * kScriptStep > limit) throw std::runtime_error("screen " + name + " not reached in " + std::to_string(limit) + " s");
                step(0);
            }
        } else if (verb == "shot") {
            std::string name;
            fields >> name;
            if (hooks.shot) hooks.shot(name);
        } else {
            throw std::runtime_error("unknown command " + verb);
        }
    }
    return updates;
}

}

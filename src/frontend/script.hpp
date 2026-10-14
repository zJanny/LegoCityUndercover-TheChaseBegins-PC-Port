#pragma once
// Scripted pad for the front end, used by chase-frontshot and chase-game --front-script. Commands are separated by ';':
//   run SECONDS            advance the front end by SECONDS at 30 updates per second
//   press BUTTON           one update with the button pressed (A B X Y START UP DOWN)
//   until SCREEN [LIMIT]   advance until the screen is shown (boot logos fusion language prompt autosave slot saving
//                          loading intro card done); fails after LIMIT seconds (default 120)
//   shot NAME              calls the hook with NAME
// A malformed script is rejected before anything runs; the error is "reason: 'text'".
#include "frontend/frontend.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::frontend {

constexpr float kScriptStep = 1.0f / 30.0f;

struct ScriptCommand {
    enum class Verb { Run, Press, Until, Shot };
    Verb verb = Verb::Run;
    float seconds = 0;
    std::uint32_t buttons = 0;
    Screen screen = Screen::Boot;
    std::string name;
};

struct ScriptHooks {
    std::function<void(const std::string&)> shot;
};

std::vector<ScriptCommand> parseFrontScript(const std::string& script);
// Runs the script and returns the number of updates it made.
unsigned runScript(FrontEnd& front, const std::string& script, const ScriptHooks& hooks);
Screen screenFromName(const std::string& name);
const char* screenName(Screen screen);

}

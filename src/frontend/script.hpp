#pragma once
// Scripted pad for the front end, used by chase-frontshot and chase-game --front-script. Commands are separated by ';':
//   run SECONDS            advance the front end by SECONDS at 30 updates per second
//   press BUTTON           one update with the button pressed (A B X Y START UP DOWN)
//   until SCREEN [LIMIT]   advance until the screen is shown (boot logos fusion language prompt autosave slot saving
//                          loading intro card done); fails after LIMIT seconds (default 120)
//   shot NAME              calls the hook with NAME
#include "frontend/frontend.hpp"
#include <functional>
#include <string>

namespace chase::frontend {

constexpr float kScriptStep = 1.0f / 30.0f;

struct ScriptHooks {
    std::function<void(const std::string&)> shot;
};

// Runs the script and returns the number of updates it made.
unsigned runScript(FrontEnd& front, const std::string& script, const ScriptHooks& hooks);
Screen screenFromName(const std::string& name);
const char* screenName(Screen screen);

}

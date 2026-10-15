#include "runtime/script_thread_log.hpp"

namespace chase::runtime {

ScriptThreadLog::ScriptThreadLog(const assets::Level& level) {
    for (const assets::LevelScript& script : level.scripts)
        for (const assets::ScriptCall& call : script.calls)
            for (const std::string& argument : call.arguments) learn(argument);
    for (const assets::LevelTrigger& trigger : level.triggers) learn(trigger.scriptName);
}

void ScriptThreadLog::learn(const std::string& scriptName) {
    if (!scriptName.empty()) names_.emplace(assets::nameHash(scriptName), scriptName);
}

std::vector<std::string> ScriptThreadLog::lines(const std::vector<ScriptThreadEvent>& events, unsigned update) const {
    std::vector<std::string> lines;
    for (const ScriptThreadEvent& event : events) {
        const auto name = names_.find(event.nameHash);
        lines.push_back("script " + (name == names_.end() ? "#" + std::to_string(event.nameHash) : name->second) +
                        (event.started ? " started at update " : " ended at update ") + std::to_string(update));
    }
    return lines;
}

}

#pragma once
#include "assets/assets.hpp"
#include "runtime/script_runner.hpp"
#include <map>
#include <string>
#include <vector>

namespace chase::runtime {

class ScriptThreadLog {
public:
    explicit ScriptThreadLog(const assets::Level& level);
    void learn(const std::string& scriptName);
    std::vector<std::string> lines(const std::vector<ScriptThreadEvent>& events, unsigned update) const;

private:
    std::map<std::uint32_t, std::string> names_;
};

}

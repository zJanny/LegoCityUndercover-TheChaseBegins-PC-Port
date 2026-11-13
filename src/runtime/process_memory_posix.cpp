#include "runtime/process_memory.hpp"
#include "runtime/debug_metrics.hpp"
#include <fstream>
#include <sstream>

namespace chase::runtime {

std::optional<ProcessMemory> currentProcessMemory() {
    std::ifstream status("/proc/self/status");
    std::stringstream text;
    text << status.rdbuf();
    return parseProcessStatus(text.str());
}

}

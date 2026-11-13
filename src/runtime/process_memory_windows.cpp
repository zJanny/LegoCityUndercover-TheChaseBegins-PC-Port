#include "runtime/process_memory.hpp"
#include <windows.h>
#include <psapi.h>

namespace chase::runtime {

std::optional<ProcessMemory> currentProcessMemory() {
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof counters;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)) return std::nullopt;
    return ProcessMemory{long(counters.WorkingSetSize / 1024), long(counters.PeakWorkingSetSize / 1024)};
}

}

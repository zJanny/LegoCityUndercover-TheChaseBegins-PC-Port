#include "runtime/process_cpu_time.hpp"
#include <windows.h>

namespace chase::runtime {
namespace {
double seconds(const FILETIME& time) {
    ULARGE_INTEGER ticks;
    ticks.LowPart = time.dwLowDateTime;
    ticks.HighPart = time.dwHighDateTime;
    return double(ticks.QuadPart) * 1e-7;
}
}

double processCpuSeconds() {
    FILETIME creation{}, exit{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) return 0;
    return seconds(user) + seconds(kernel);
}

}

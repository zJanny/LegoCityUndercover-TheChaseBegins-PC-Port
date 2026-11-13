#pragma once
#include <optional>

namespace chase::runtime {

struct ProcessMemory {
    long residentKilobytes = 0;
    long peakKilobytes = 0;
};

// Resident size now and highest resident size so far of this process in kilobytes (1024 bytes), on the platform file
// of the build: VmRSS and VmHWM on POSIX, working set and peak working set on Windows. Nothing when it cannot be read.
std::optional<ProcessMemory> currentProcessMemory();

}

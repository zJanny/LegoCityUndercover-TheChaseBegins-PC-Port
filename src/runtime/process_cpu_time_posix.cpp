#include "runtime/process_cpu_time.hpp"
#include <sys/resource.h>

namespace chase::runtime {

double processCpuSeconds() {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return double(usage.ru_utime.tv_sec) + double(usage.ru_utime.tv_usec) * 1e-6 +
           double(usage.ru_stime.tv_sec) + double(usage.ru_stime.tv_usec) * 1e-6;
}

}

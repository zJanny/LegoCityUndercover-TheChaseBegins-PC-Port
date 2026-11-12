// Process CPU time of the debug overlay, on the platform file of the build (POSIX or Windows).
#include "runtime/process_cpu_time.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    using chase::runtime::processCpuSeconds;
    try {
        const double start = processCpuSeconds();
        require(start >= 0, "CPU time is not negative");

        const auto wallStart = std::chrono::steady_clock::now();
        volatile double sink = 0;
        double busy = start;
        while (busy - start < 0.1 && std::chrono::steady_clock::now() - wallStart < std::chrono::seconds(3)) {
            for (int i = 1; i < 100000; ++i) sink = sink + std::sqrt(double(i));
            busy = processCpuSeconds();
        }
        require(busy - start >= 0.1, "busy work adds CPU time: " + std::to_string(busy - start));

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        const double slept = processCpuSeconds() - busy;
        require(slept >= 0 && slept < 0.15, "sleeping adds almost no CPU time: " + std::to_string(slept));
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "process CPU time tests passed\n";
    return 0;
}

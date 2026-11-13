// Process memory of the debug overlay, on the platform file of the build (POSIX or Windows).
#include "runtime/process_memory.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    using chase::runtime::currentProcessMemory;
    try {
        const auto before = currentProcessMemory();
        require(before.has_value(), "the process memory can be read");
        require(before->residentKilobytes > 1024 && before->residentKilobytes < 16L * 1024 * 1024,
                "resident size is between 1 MB and 16 GB: " + std::to_string(before->residentKilobytes));
        require(before->peakKilobytes >= before->residentKilobytes,
                "peak is at least the resident size: " + std::to_string(before->peakKilobytes));

        constexpr std::size_t kBytes = 64u * 1024 * 1024;
        std::vector<char> block(kBytes);
        volatile char* bytes = block.data();
        for (std::size_t at = 0; at < kBytes; at += 4096) bytes[at] = 1;
        const auto during = currentProcessMemory();
        require(during.has_value(), "the process memory can be read again");
        require(during->residentKilobytes >= before->residentKilobytes + 48L * 1024,
                "touching 64 MB raises the resident size: " + std::to_string(before->residentKilobytes) + " to " +
                    std::to_string(during->residentKilobytes));
        require(during->peakKilobytes >= during->residentKilobytes, "peak stays at least the resident size");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "process memory tests passed\n";
    return 0;
}

#include "runtime/mission13_end.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        chase::runtime::AiMoveWatch watch;
        const float at[3] = {1.0f, 2.0f, 3.0f};
        if (watch.observe(false, at, 10)) throw std::runtime_error("no line while nothing moves");
        const auto started = watch.observe(true, at, 11);
        if (!started || started->find("started at update 11 from 1 2 3") == std::string::npos) throw std::runtime_error("start line");
        if (watch.observe(true, at, 12)) throw std::runtime_error("no line while the move goes on");
        const auto ended = watch.observe(false, at, 40);
        if (!ended || ended->find("ended at update 40 at 1 2 3, 29 updates") == std::string::npos) throw std::runtime_error("end line");
        if (watch.observe(false, at, 99)) throw std::runtime_error("no line before the settle update");
        const auto settled = watch.observe(false, at, 100);
        if (!settled || settled->find("60 updates after the end") == std::string::npos) throw std::runtime_error("settle line");
        if (watch.observe(false, at, 101)) throw std::runtime_error("the settle line comes once");
    } catch (const std::exception& error) {
        std::cerr << "mission13-end-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "mission13-end-test: ok\n";
    return 0;
}

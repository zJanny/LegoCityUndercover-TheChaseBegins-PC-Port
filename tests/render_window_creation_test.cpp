// render/window_creation.hpp: the CHASE_HIDDEN_WINDOW switch and the missing-context message (no OpenGL context needed).
// Argument "environment-hidden" or "environment-rejected" checks the value CTest put into the environment; no argument
// checks the parser. Expected values from the CHASE_HIDDEN_WINDOW contract of docs/plans/features/windows-tests.md
// section 3 (0 shows the window, 1 hides it, anything else stops with an error), 20261113-winopen-A, 2026-11-13.
#include "render/window_creation.hpp"
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

std::string rejection(const char* value) {
    try {
        chase::render::hiddenWindowFlags(value);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

void parser() {
    check(chase::render::hiddenWindowFlags(nullptr) == 0, "unset shows the window");
    check(chase::render::hiddenWindowFlags("0") == 0, "0 shows the window");
    check(chase::render::hiddenWindowFlags("1") == SDL_WINDOW_HIDDEN, "1 hides the window");
    for (const char* bad : {"", "2", "01", "yes", "true", " 1", "1 ", "-1", "hidden"}) {
        const std::string message = rejection(bad);
        check(message == std::string("CHASE_HIDDEN_WINDOW: expected 0 or 1, got '") + bad + "'", bad);
    }
    check(chase::render::noGlContextMessage("driver text") == "no OpenGL 4.6 context: driver text", "missing context message");
}

}

int main(int argc, char** argv) {
    if (argc == 1) parser();
    else if (argc == 2 && std::strcmp(argv[1], "environment-hidden") == 0)
        check(chase::render::hiddenWindowFlagsFromEnvironment() == SDL_WINDOW_HIDDEN, "environment 1 hides the window");
    else if (argc == 2 && std::strcmp(argv[1], "environment-rejected") == 0) {
        bool thrown = false;
        try {
            chase::render::hiddenWindowFlagsFromEnvironment();
        } catch (const std::runtime_error& error) {
            thrown = std::string(error.what()) == "CHASE_HIDDEN_WINDOW: expected 0 or 1, got 'maybe'";
        }
        check(thrown, "environment maybe is rejected naming the switch and the text");
    } else {
        std::printf("FAIL: unknown argument\n");
        return 2;
    }
    if (failures) return 1;
    std::puts("ok");
    return 0;
}

#include "render/window_creation.hpp"
#include <SDL.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace chase::render {

std::uint32_t hiddenWindowFlags(const char* value) {
    if (!value || std::strcmp(value, "0") == 0) return 0;
    if (std::strcmp(value, "1") == 0) return SDL_WINDOW_HIDDEN;
    throw std::runtime_error(std::string("CHASE_HIDDEN_WINDOW: expected 0 or 1, got '") + value + "'");
}

std::uint32_t hiddenWindowFlagsFromEnvironment() { return hiddenWindowFlags(std::getenv("CHASE_HIDDEN_WINDOW")); }

std::string noGlContextMessage(const char* sdlError) { return std::string("no OpenGL 4.6 context: ") + sdlError; }

}

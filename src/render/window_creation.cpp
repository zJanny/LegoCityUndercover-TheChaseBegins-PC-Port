#include "render/window_creation.hpp"
#include <SDL.h>
#include <epoxy/gl.h>
#include <cstdio>
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

std::string glVersionProblem(const char* glVersion) {
    if (!glVersion) return "the driver reports no OpenGL version";
    const std::string text(glVersion);
    int major = 0, minor = 0;
    if (text.empty() || text[0] < '0' || text[0] > '9' || std::sscanf(text.c_str(), "%d.%d", &major, &minor) != 2)
        return "the OpenGL version '" + text + "' is not major.minor";
    if (major > 4 || (major == 4 && minor >= 6)) return {};
    return "the context is OpenGL " + std::to_string(major) + "." + std::to_string(minor) + ", 4.6 is needed";
}

std::string currentGlContextProblem() { return glVersionProblem(reinterpret_cast<const char*>(glGetString(GL_VERSION))); }

}

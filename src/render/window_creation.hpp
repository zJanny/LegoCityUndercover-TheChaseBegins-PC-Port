#pragma once
// Window creation helpers shared by chase-game and chase-viewer: the CHASE_HIDDEN_WINDOW switch the test setup uses to
// run with a real video driver without showing a window, and the message for a missing OpenGL 4.6 context.
#include <cstdint>
#include <string>

namespace chase::render {

// SDL window flags for a value of CHASE_HIDDEN_WINDOW: nullptr (unset) and "0" give none, "1" gives SDL_WINDOW_HIDDEN.
// Any other value throws std::runtime_error naming the switch and the text.
std::uint32_t hiddenWindowFlags(const char* value);

std::uint32_t hiddenWindowFlagsFromEnvironment();

// "no OpenGL 4.6 context: <SDL error>"; CTest turns this line into a skip.
std::string noGlContextMessage(const char* sdlError);

}

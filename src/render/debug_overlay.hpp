#pragma once
// Developer debug overlay drawing (docs/debug-overlay.md): the text panel in the top-left corner of the window,
// drawn with the HUD shaders. Not game behaviour.
#include <epoxy/gl.h>
#include <string>
#include <vector>

namespace chase::render {

class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();
    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    void draw(const std::vector<std::string>& lines, int windowWidth, int windowHeight);

private:
    GLuint program_ = 0, array_ = 0, buffer_ = 0, texture_ = 0;
};

}

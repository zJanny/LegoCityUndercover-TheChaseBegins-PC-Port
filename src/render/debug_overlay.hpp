#pragma once
// Developer debug overlay drawing (docs/debug-overlay.md): the text panel in the top-left corner of the window,
// drawn with the HUD shaders. Not game behaviour.
#include <memory>
#include <string>
#include <vector>

namespace chase::render {

class DebugOverlayImpl;

class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();
    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    void draw(const std::vector<std::string>& lines, int windowWidth, int windowHeight);

private:
    std::unique_ptr<DebugOverlayImpl> impl_;
};

}

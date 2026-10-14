#pragma once
// Developer debug overlay drawing (docs/debug-overlay.md): the text panel in the top-left corner of the window,
// drawn with the HUD shaders. Not game behaviour.
#include "render/frame_stats.hpp"
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

    void beginFrame() { timer_.begin(); }
    FrameStats endFrame() { return timer_.end(); }
    void draw(const std::vector<std::string>& lines, int windowWidth, int windowHeight);

private:
    std::unique_ptr<DebugOverlayImpl> impl_;
    GpuFrameTimer timer_;
};

}

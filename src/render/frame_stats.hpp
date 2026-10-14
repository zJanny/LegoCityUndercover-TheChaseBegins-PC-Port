#pragma once
// Developer render statistics for the debug overlay (docs/debug-overlay.md): draw calls per frame and GPU time from
// timer queries. Not game behaviour. The interface uses plain types only; every GL call is in frame_stats.cpp.
#include <array>
#include <memory>

namespace chase::render {

struct FrameStats {
    unsigned drawCalls = 0;
    bool gpuValid = false;
    double gpuMilliseconds = 0;
};

void countDrawCall();

class GpuFrameTimerImpl;

class GpuFrameTimer {
public:
    GpuFrameTimer();
    ~GpuFrameTimer();
    GpuFrameTimer(const GpuFrameTimer&) = delete;
    GpuFrameTimer& operator=(const GpuFrameTimer&) = delete;

    void begin();
    // Draw calls since begin() and the newest GPU time that the driver has finished (a frame or two behind).
    FrameStats end();

private:
    std::unique_ptr<GpuFrameTimerImpl> impl_;
};

}

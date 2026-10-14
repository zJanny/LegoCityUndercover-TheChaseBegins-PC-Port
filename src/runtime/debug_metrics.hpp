#pragma once
// Developer debug overlay values (docs/debug-overlay.md). Not game behaviour. No SDL, no OpenGL.
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

inline constexpr double kMetricsWindowSeconds = 1.0;

struct FrameTimeStats {
    unsigned frames = 0;
    double lastMs = 0, averageMs = 0, minimumMs = 0, maximumMs = 0;
};

// Frames whose end lies within the last kMetricsWindowSeconds of the newest frame.
class FrameTimeWindow {
public:
    void add(double endSeconds, double durationSeconds);
    FrameTimeStats stats() const;
private:
    struct Sample { double end, duration; };
    std::deque<Sample> samples_;
};

struct ProcessMemory {
    long residentKilobytes = 0;
    long peakKilobytes = 0;
};

// Reads VmRSS and VmHWM (kB) from the text of /proc/<pid>/status; nothing when either line is missing or malformed.
std::optional<ProcessMemory> parseProcessStatus(const std::string& text);

// Process CPU time over wall time between two samples at least kMetricsWindowSeconds apart; 100 means one full core.
class CpuMeter {
public:
    void sample(double wallSeconds, double cpuSeconds);
    std::optional<double> percent() const { return percent_; }
private:
    std::optional<double> baseWall_, baseCpu_, percent_;
};

struct DebugGameValues {
    unsigned updatesPerSecond = 0;
    std::string level;
    bool hasPlayer = false;
    float position[3] = {0, 0, 0};
    float headingDegrees = 0;
    std::size_t scriptThreads = 0;
    std::optional<std::size_t> loadedRooms;
};

struct DebugReadout {
    FrameTimeStats frame;
    std::optional<ProcessMemory> memory;
    std::optional<double> cpuPercent;
    std::optional<double> gpuMilliseconds;
    std::optional<unsigned> drawCalls;
};

// Frame timing is recorded on every frame (one clock read); /proc and getrusage are read only while visible.
class DebugMetrics {
public:
    void frame();
    void setVisible(bool visible);
    DebugReadout readout();
private:
    FrameTimeWindow window_;
    CpuMeter cpu_;
    std::optional<double> lastFrameEnd_;
    std::optional<ProcessMemory> memory_;
    std::optional<double> lastSlowRefresh_;
    bool visible_ = false;
};

std::vector<std::string> debugOverlayLines(const DebugReadout& readout, const DebugGameValues& game);
// The timing and process lines plus the current front-end screen (docs/debug-overlay.md).
std::vector<std::string> debugFrontEndLines(const DebugReadout& readout, const std::string& screen);

}

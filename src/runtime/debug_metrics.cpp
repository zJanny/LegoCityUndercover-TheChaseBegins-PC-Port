#include "runtime/debug_metrics.hpp"
#include <sys/resource.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace chase::runtime {
namespace {

double wallSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

double processCpuSeconds() {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return double(usage.ru_utime.tv_sec) + double(usage.ru_utime.tv_usec) * 1e-6 +
           double(usage.ru_stime.tv_sec) + double(usage.ru_stime.tv_usec) * 1e-6;
}

std::optional<long> kilobytesOf(const std::string& text, const std::string& key) {
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.compare(0, key.size(), key) != 0) continue;
        std::istringstream fields(line.substr(key.size()));
        long value = 0;
        std::string unit, extra;
        if (!(fields >> value >> unit) || unit != "kB" || (fields >> extra) || value < 0) return std::nullopt;
        return value;
    }
    return std::nullopt;
}

std::string format(const char* pattern, double a = 0, double b = 0, double c = 0) {
    char buffer[96];
    std::snprintf(buffer, sizeof buffer, pattern, a, b, c);
    return buffer;
}

}

void FrameTimeWindow::add(double endSeconds, double durationSeconds) {
    samples_.push_back({endSeconds, durationSeconds});
    while (samples_.size() > 1 && samples_.front().end <= endSeconds - kMetricsWindowSeconds) samples_.pop_front();
}

FrameTimeStats FrameTimeWindow::stats() const {
    FrameTimeStats stats;
    if (samples_.empty()) return stats;
    double total = 0;
    stats.minimumMs = stats.maximumMs = samples_.front().duration * 1000;
    for (const Sample& sample : samples_) {
        const double ms = sample.duration * 1000;
        total += ms;
        stats.minimumMs = std::min(stats.minimumMs, ms);
        stats.maximumMs = std::max(stats.maximumMs, ms);
    }
    stats.frames = unsigned(samples_.size());
    stats.lastMs = samples_.back().duration * 1000;
    stats.averageMs = total / double(samples_.size());
    return stats;
}

std::optional<ProcessMemory> parseProcessStatus(const std::string& text) {
    const std::optional<long> resident = kilobytesOf(text, "VmRSS:"), peak = kilobytesOf(text, "VmHWM:");
    if (!resident || !peak) return std::nullopt;
    return ProcessMemory{*resident, *peak};
}

void CpuMeter::sample(double wallSeconds, double cpuSeconds) {
    if (!baseWall_) {
        baseWall_ = wallSeconds;
        baseCpu_ = cpuSeconds;
        return;
    }
    const double elapsed = wallSeconds - *baseWall_;
    if (elapsed < kMetricsWindowSeconds) return;
    percent_ = (cpuSeconds - *baseCpu_) / elapsed * 100.0;
    baseWall_ = wallSeconds;
    baseCpu_ = cpuSeconds;
}

void DebugMetrics::frame() {
    const double now = wallSeconds();
    if (lastFrameEnd_) window_.add(now, now - *lastFrameEnd_);
    lastFrameEnd_ = now;
}

void DebugMetrics::setVisible(bool visible) {
    if (visible && !visible_) {
        cpu_ = CpuMeter();
        memory_.reset();
        lastSlowRefresh_.reset();
    }
    visible_ = visible;
}

DebugReadout DebugMetrics::readout() {
    const double now = wallSeconds();
    if (!lastSlowRefresh_ || now - *lastSlowRefresh_ >= kMetricsWindowSeconds) {
        lastSlowRefresh_ = now;
        std::ifstream status("/proc/self/status");
        std::stringstream text;
        text << status.rdbuf();
        memory_ = parseProcessStatus(text.str());
        cpu_.sample(now, processCpuSeconds());
    }
    return {window_.stats(), memory_, cpu_.percent()};
}

std::vector<std::string> debugOverlayLines(const DebugReadout& readout, const DebugGameValues& game) {
    std::vector<std::string> lines;
    const FrameTimeStats& frame = readout.frame;
    lines.push_back(format("FPS %.0f", double(frame.frames)));
    lines.push_back(format("FRAME MS LAST %.1f AVG %.1f", frame.lastMs, frame.averageMs));
    lines.push_back(format("FRAME MS MIN %.1f MAX %.1f", frame.minimumMs, frame.maximumMs));
    if (readout.memory)
        lines.push_back(format("MEM MB %.1f PEAK %.1f", double(readout.memory->residentKilobytes) / 1024.0,
                               double(readout.memory->peakKilobytes) / 1024.0));
    else lines.push_back("MEM MB N/A");
    lines.push_back(readout.cpuPercent ? format("CPU %.1f %%", *readout.cpuPercent) : "CPU ...");
    lines.push_back(format("UPDATES/S %.0f", double(game.updatesPerSecond)));
    lines.push_back("LEVEL " + game.level);
    if (game.hasPlayer) {
        lines.push_back(format("POS %.2f %.2f %.2f", game.position[0], game.position[1], game.position[2]));
        lines.push_back(format("HEADING %.1f DEG", game.headingDegrees));
    } else lines.push_back("NO PLAYER");
    lines.push_back("SCRIPT THREADS " + std::to_string(game.scriptThreads));
    if (game.loadedRooms) lines.push_back("ROOMS LOADED " + std::to_string(*game.loadedRooms));
    return lines;
}

}

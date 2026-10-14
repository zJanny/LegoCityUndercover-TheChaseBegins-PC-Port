#include "runtime/debug_metrics.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace chase::runtime;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }
}
int main() {
    try {
        FrameTimeWindow window;
        require(window.stats().frames == 0, "empty window");
        window.add(0.01, 0.010);
        window.add(0.03, 0.020);
        window.add(0.06, 0.030);
        FrameTimeStats stats = window.stats();
        require(stats.frames == 3 && near(stats.lastMs, 30) && near(stats.averageMs, 20) && near(stats.minimumMs, 10) && near(stats.maximumMs, 30),
                "frame statistics over three frames");
        window.add(1.02, 0.015);
        stats = window.stats();
        require(stats.frames == 3 && near(stats.minimumMs, 15) && near(stats.maximumMs, 30),
                "frames that ended more than one second before the newest are dropped");
        window.add(5.0, 0.5);
        stats = window.stats();
        require(stats.frames == 1 && near(stats.lastMs, 500) && near(stats.averageMs, 500), "a long gap leaves only the newest frame");

        const std::string status = "Name:\tchase-game\nVmPeak:\t  900000 kB\nVmHWM:\t  204800 kB\nVmRSS:\t  102400 kB\nThreads:\t4\n";
        const auto memory = parseProcessStatus(status);
        require(memory && memory->residentKilobytes == 102400 && memory->peakKilobytes == 204800, "VmRSS and VmHWM parsed");
        require(!parseProcessStatus("VmRSS:\t 5 kB\n"), "missing VmHWM rejected");
        require(!parseProcessStatus("VmRSS:\t 5 MB\nVmHWM:\t 6 kB\n"), "wrong unit rejected");
        require(!parseProcessStatus("VmRSS:\t abc kB\nVmHWM:\t 6 kB\n"), "non-number rejected");
        require(!parseProcessStatus(""), "empty text rejected");

        CpuMeter cpu;
        cpu.sample(10.0, 5.0);
        require(!cpu.percent(), "first sample only sets the baseline");
        cpu.sample(10.5, 5.4);
        require(!cpu.percent(), "less than one second apart gives no value");
        cpu.sample(11.0, 5.5);
        require(cpu.percent() && near(*cpu.percent(), 50.0), "0.5 s CPU in 1 s wall is 50 percent");
        cpu.sample(13.0, 9.5);
        require(near(*cpu.percent(), 200.0), "4 s CPU in 2 s wall is 200 percent (two cores)");

        DebugReadout readout;
        readout.frame = {60, 16.7, 16.6, 16.0, 17.5};
        readout.memory = ProcessMemory{102400, 204800};
        readout.cpuPercent = 12.34;
        DebugGameValues game;
        game.updatesPerSecond = 60;
        game.level = "lvl_01_cherrytreehills";
        game.hasPlayer = true;
        game.position[0] = 886.764f;
        game.position[1] = 16.589f;
        game.position[2] = 1111.166f;
        game.headingDegrees = 90.0f;
        game.scriptThreads = 3;
        game.loadedRooms = 9;
        const std::vector<std::string> expected = {
            "FPS 60", "FRAME MS LAST 16.7 AVG 16.6", "FRAME MS MIN 16.0 MAX 17.5", "MEM MB 100.0 PEAK 200.0", "CPU 12.3 %",
            "UPDATES/S 60", "LEVEL lvl_01_cherrytreehills", "POS 886.76 16.59 1111.17", "HEADING 90.0 DEG", "SCRIPT THREADS 3",
            "ROOMS LOADED 9"};
        require(debugOverlayLines(readout, game) == expected, "overlay lines");
        readout.memory.reset();
        readout.cpuPercent.reset();
        game.hasPlayer = false;
        game.loadedRooms.reset();
        require(debugOverlayLines(readout, game).back() == "SCRIPT THREADS 3", "no room line without room streaming");
        const std::vector<std::string> missing = debugOverlayLines(readout, game);
        require(missing[3] == "MEM MB N/A" && missing[4] == "CPU ..." && missing[7] == "NO PLAYER", "missing values are labelled");
        std::cout << "Debug metrics tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

// Synthetic cases of the director camera (src/runtime/dcam.cpp): the rules come from the disassembly of 0x21cc80,
// 0x2665e4, 0x2028c8 / 0x202814, 0x236104 and 0x196784 (docs/re/dcam.md); the paths here are made up.
#include "runtime/dcam.hpp"
#include "runtime/script_runner.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Level;
using chase::assets::LevelPath;
using chase::assets::LevelScript;
using chase::assets::PlacedObjectState;
using chase::assets::ScriptCall;
using chase::runtime::DirectorCamera;
using chase::runtime::DirectorRequest;
using chase::runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

LevelPath path(const std::string& name, std::vector<std::array<float, 3>> points, bool closed = false) {
    LevelPath p;
    p.nameHash = chase::assets::nameHash(name);
    p.closed = closed;
    p.points = std::move(points);
    return p;
}

void splineHitsItsPoints() {
    const LevelPath p = path("p", {{0, 0, 0}, {10, 0, 0}, {10, 10, 0}, {0, 10, 0}});
    float out[3];
    chase::runtime::sampleDirectorPath(p, 0.0f, out);
    require(near(out[0], 0) && near(out[1], 0), "start point");
    chase::runtime::sampleDirectorPath(p, 1.0f, out);
    require(near(out[0], 10) && near(out[1], 0), "second point");
    chase::runtime::sampleDirectorPath(p, 3.0f, out);
    require(near(out[0], 0) && near(out[1], 10), "last point");
    chase::runtime::sampleDirectorPath(p, 9.0f, out);
    require(near(out[0], 0) && near(out[1], 10), "an open path clamps past its end");
    chase::runtime::sampleDirectorPath(p, -2.0f, out);
    require(near(out[0], 0) && near(out[1], 0), "an open path clamps before its start");
}

void onePointAndTwoPointPaths() {
    float out[3];
    chase::runtime::sampleDirectorPath(path("one", {{4, 5, 6}}), 0.7f, out);
    require(near(out[0], 4) && near(out[1], 5) && near(out[2], 6), "a one point path stays at its point");
    chase::runtime::sampleDirectorPath(path("two", {{0, 0, 0}, {10, 0, 0}}), 0.5f, out);
    require(near(out[0], 5), "two points: the midpoint");
}

void closedPathWraps() {
    const LevelPath p = path("c", {{0, 0, 0}, {10, 0, 0}, {10, 10, 0}, {0, 10, 0}}, true);
    float out[3];
    chase::runtime::sampleDirectorPath(p, 4.0f, out);
    require(near(out[0], 0) && near(out[1], 0), "position 4 of 4 points wraps to the first point");
    chase::runtime::sampleDirectorPath(p, -1.0f, out);
    require(near(out[0], 0) && near(out[1], 10), "position -1 wraps to the last point");
}

void runsForItsDurationThenHandsBack() {
    const LevelPath eye = path("eye", {{0, 0, 0}, {10, 0, 0}});
    const LevelPath look = path("look", {{0, 5, 0}, {0, 5, 10}});
    DirectorCamera director;
    require(!director.busy(), "idle at the start");
    require(director.startPaths(&eye, &look, 1.0f, 0), "start");
    require(director.busy(), "busy from the call on");
    auto requests = director.takeRequests();
    require(requests.size() == 1 && requests[0].kind == DirectorRequest::Kind::Start && near(requests[0].fadeSeconds, 1.6f) && !requests[0].instant,
            "a start blends in over 1.6 s");
    int updates = 0;
    while (director.busy() && updates < 100) {
        director.update(0.033f, nullptr);
        ++updates;
    }
    require(updates == 32, "the done flag is set by the first update with 1000 ms elapsed, the end test of the next update ends it: " + std::to_string(updates));
    require(near(director.pose().eye[0], 10) && near(director.pose().look[2], 10), "the last update shows the path end");
    requests = director.takeRequests();
    require(requests.size() == 1 && requests[0].kind == DirectorRequest::Kind::End && near(requests[0].fadeSeconds, 1.6f) && !requests[0].instant,
            "the end blends back over 1.6 s");
}

void snapFlagsSetTheBlendTimes() {
    const LevelPath eye = path("eye", {{0, 0, 0}, {1, 0, 0}});
    const LevelPath look = path("look", {{0, 0, 1}});
    DirectorCamera director;
    director.startPaths(&eye, &look, 0.05f, 3);
    auto requests = director.takeRequests();
    require(requests.size() == 1 && requests[0].instant && requests[0].fadeSeconds == 0.0f, "snap start: no blend in");
    director.update(0.033f, nullptr);
    director.update(0.033f, nullptr);
    director.update(0.033f, nullptr);
    requests = director.takeRequests();
    require(requests.size() == 1 && requests[0].kind == DirectorRequest::Kind::End && requests[0].fadeSeconds == 0.0f, "snap end: no blend out");
}

void waitsApplyToTheNextStartOnly() {
    const LevelPath eye = path("eye", {{0, 0, 0}, {10, 0, 0}});
    const LevelPath look = path("look", {{0, 0, 1}});
    DirectorCamera director;
    director.setStartWait(0.2f);
    director.startPaths(&eye, &look, 0.1f, 0);
    director.setEndWait(0.1f);
    int updates = 0;
    while (director.busy() && updates < 100) {
        director.update(0.033f, nullptr);
        ++updates;
        if (updates == 3) require(near(director.pose().eye[0], 0) && director.phase() == DirectorCamera::Phase::StartWait, "start wait holds the first point");
    }
    require(updates == 16, "0.2 s start wait + 0.1 s run + 0.1 s end wait, each ended by the first update at or past it, plus the end test: " + std::to_string(updates));
    director.startPaths(&eye, &look, 0.1f, 0);
    updates = 0;
    while (director.busy() && updates < 100) {
        director.update(0.033f, nullptr);
        ++updates;
    }
    require(updates == 5, "the waits are cleared by the start: " + std::to_string(updates));
}

void endRequestEndsAtOnceWithoutBlend() {
    const LevelPath eye = path("eye", {{0, 0, 0}, {10, 0, 0}});
    const LevelPath look = path("look", {{0, 0, 1}});
    DirectorCamera director;
    director.requestEnd();
    require(!director.busy(), "EndDCam does nothing while idle");
    director.startPaths(&eye, &look, 5.0f, 0);
    director.takeRequests();
    director.update(0.033f, nullptr);
    director.requestEnd();
    director.update(0.033f, nullptr);
    const auto requests = director.takeRequests();
    require(!director.busy() && requests.size() == 1 && requests[0].instant, "EndDCam ends without a blend");
}

void trackLooksAtAnObject() {
    const LevelPath eye = path("eye", {{0, 0, 0}, {10, 0, 0}});
    DirectorCamera director;
    director.startTrack(&eye, "thing", 1.0f, 0);
    director.update(0.033f, [](const std::string& name, float out[3]) {
        out[0] = 7;
        out[1] = 8;
        out[2] = 9;
        return name == "thing";
    });
    require(near(director.pose().look[0], 7) && near(director.pose().look[2], 9), "the look point is the object's point");
}

void scriptNativesBlockUntilTheDirectorEnds() {
    Level level;
    level.paths.push_back(path("camera", {{0, 0, 0}, {10, 0, 0}}));
    level.paths.push_back(path("target", {{0, 0, 5}}));
    LevelScript script;
    script.nameHash = chase::assets::nameHash("s");
    script.calls = {{"StartDCam", {"Camera", "Target", "0", "0.1", "0"}}, {"WaitForDCam", {}}, {"=", {"Done", "1"}}};
    level.scripts.push_back(script);
    PlacedObjectState state;
    ScriptRunner runner(level, state);
    runner.start("s");
    require(runner.director().busy() && runner.variable("Done") == 0, "WaitForDCam blocks while the director is busy");
    for (int i = 0; i < 5; ++i) {
        runner.update(0.033f);
        require(runner.variable("Done") == 0, "blocked until the update after the end test");
        runner.director().update(0.033f, nullptr);
    }
    runner.update(0.033f);
    require(!runner.director().busy() && runner.variable("Done") == 1, "WaitForDCam continues once the director has ended");
    require(runner.unhandledFunctions().empty(), "the DCam functions have handlers");
}

void startWithMissingPathDoesNothing() {
    DirectorCamera director;
    const LevelPath eye = path("eye", {{0, 0, 0}});
    require(!director.startPaths(&eye, nullptr, 1.0f, 0) && !director.busy(), "a start without a look path is not a start");
    require(!director.startTrack(nullptr, "x", 1.0f, 0) && !director.busy(), "a start without a camera path is not a start");
}
}

int main() {
    try {
        splineHitsItsPoints();
        onePointAndTwoPointPaths();
        closedPathWraps();
        runsForItsDurationThenHandsBack();
        snapFlagsSetTheBlendTimes();
        waitsApplyToTheNextStartOnly();
        endRequestEndsAtOnceWithoutBlend();
        trackLooksAtAnObject();
        scriptNativesBlockUntilTheDirectorEnds();
        startWithMissingPathDoesNothing();
    } catch (const std::exception& error) {
        std::cerr << "dcam-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "dcam-test: ok\n";
    return 0;
}

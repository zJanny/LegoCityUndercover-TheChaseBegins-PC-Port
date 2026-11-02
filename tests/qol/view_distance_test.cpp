// The View distance slider (src/qol/view_distance.*) and its hooks.
// Spec: docs/design/qol-view-distance.md entry 2026-11-02T18:43:07 (the user's decisions of 2026-11-02): Display tab, x1.0 to x20.0 (0.25 steps to x4.0,
// whole steps above) shown as "x1.0", fog start and end scale with the far plane, far plane, fog and pedestrian distances scale, Original always
// x1.0, all cameras, Reset to defaults sets x1.0, `view_distance <factor>` in config/qol.cfg (strict, off-step rejected), greyed out at
// x1.0 in Original. The original far plane 260 is kFollowFarPlane (src/runtime/clip_planes.hpp); the fog distances and the distance fade
// table are fixtures or the port's defaults (DistanceFadeTable). Cases written by 20261102-qolmenu-E on 2026-11-02.
#include "qol/active_settings.hpp"
#include "qol/settings.hpp"
#include "qol/view_distance.hpp"
#include "runtime/clip_planes.hpp"
#include "runtime/fog.hpp"
#include "runtime/node_fade.hpp"
#include "runtime/pedestrian_manager.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace rt = chase::runtime;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

qol::Settings with(qol::Master master, int step) {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, false);
    settings.setMaster(master);
    settings.setViewDistanceStep(step);
    return settings;
}

std::string written(const qol::Settings& settings) {
    std::ostringstream out;
    qol::writeSettings(out, settings);
    return out.str();
}

std::string readError(const std::string& text) {
    std::istringstream in(text);
    try {
        qol::readSettings(in);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

void stepsAndLabels() {
    std::vector<std::string> labels;
    std::vector<float> factors;
    for (int quarters = 4; quarters <= 16; ++quarters) factors.push_back(static_cast<float>(quarters) / 4.0f);
    for (int whole = 5; whole <= 20; ++whole) factors.push_back(static_cast<float>(whole));
    const std::vector<std::string> fine{"x1.0", "x1.25", "x1.5", "x1.75", "x2.0", "x2.25", "x2.5", "x2.75", "x3.0", "x3.25", "x3.5", "x3.75", "x4.0"};
    labels = fine;
    for (int whole = 5; whole <= 20; ++whole) labels.push_back("x" + std::to_string(whole) + ".0");
    require(qol::viewDistanceSteps + 1 == 29 && labels.size() == 29, "29 positions: 0.25 steps to x4.0, whole steps to x20.0");
    for (int step = 0; step <= qol::viewDistanceSteps; ++step) {
        require(qol::viewDistanceLabel(step) == labels[static_cast<std::size_t>(step)], "label of step " + std::to_string(step));
        require(qol::viewDistanceFactor(step) == factors[static_cast<std::size_t>(step)], "factor of step " + std::to_string(step));
        require(qol::parseViewDistance(qol::viewDistanceText(step)) == step, "the text of step " + std::to_string(step) + " parses back");
    }
    require(qol::viewDistanceLabel(qol::viewDistanceSteps) == "x20.0", "the last label is x20.0");
    for (int bad : {-1, 29}) {
        bool threw = false;
        try { qol::viewDistanceFactor(bad); } catch (const std::invalid_argument&) { threw = true; }
        require(threw && !qol::viewDistanceStepValid(bad), "step " + std::to_string(bad) + " is rejected");
    }
}

void configKeyIsStrict() {
    qol::Settings settings;
    require(settings.viewDistanceStep() == 0, "the default is x1.0");
    require(written(settings).find("view_distance") == std::string::npos, "no view_distance line at x1.0 (like resolution at its default)");
    settings.setViewDistanceStep(4);
    require(written(settings).find("view_distance 2.0\n") != std::string::npos, "x2.0 is written as view_distance 2.0");
    std::istringstream back(written(settings));
    require(qol::readSettings(back).viewDistanceStep() == 4, "round trip keeps x2.0");
    std::istringstream quarter("view_distance 3.75\n");
    require(qol::readSettings(quarter).viewDistanceStep() == 11, "view_distance 3.75 is step 11");
    std::istringstream top("view_distance 20.0\n");
    require(qol::readSettings(top).viewDistanceStep() == qol::viewDistanceSteps, "view_distance 20.0 is the last step");
    std::istringstream coarse("view_distance 7.0\n");
    require(qol::readSettings(coarse).viewDistanceStep() == 15, "view_distance 7.0 is step 15");
    std::istringstream one("view_distance 1.0\n");
    require(qol::readSettings(one).viewDistanceStep() == 0, "view_distance 1.0 is accepted");

    const std::vector<std::string> rejected{
        "view_distance 1.1",    "view_distance 2.2",  "view_distance 4.25", "view_distance 0.75", "view_distance 5.5",
        "view_distance -1.0",   "view_distance 1",    "view_distance 2.50", "view_distance x2.0", "view_distance far",
        "view_distance",        "view_distance 2.0 3.0", "view_distance 20.25", "view_distance 21.0",
        "view_distance 20",     "view_distance 0",    "view_distance 4.5", "view_distance 19.5"};
    for (const std::string& line : rejected)
        require(readError("master Original\n" + line + "\n") == "Invalid QoL configuration at line 2: " + line, "rejects: " + line);
    require(readError("view_distance 2.0\nview_distance 2.0\n") == "Invalid QoL configuration at line 2: view_distance 2.0",
        "a second view_distance line is rejected");
}

void originalKeepsOne() {
    const qol::Settings original = with(qol::Master::Original, 12);
    require(original.viewDistanceStep() == 12, "Original keeps the stored value");
    require(qol::effectiveViewDistanceStep(original) == 0 && qol::effectiveViewDistanceFactor(original) == 1.0f, "Original always gives x1.0");
    require(qol::effectiveViewDistanceFactor(with(qol::Master::Improved, 12)) == 4.0f, "Improved uses the stored x4.0");
    require(qol::effectiveViewDistanceFactor(with(qol::Master::Custom, 2)) == 1.5f, "Custom uses the stored x1.5");
    require(qol::effectiveViewDistanceFactor(with(qol::Master::Improved, 0)) == 1.0f, "Improved at x1.0 is x1.0");
}

void originalNumbersAtOne() {
    const rt::ClipPlanes planes = rt::followClipPlanes(0.5f);
    const rt::FogSetting fog{true, 197.0f, 260.0f, {200, 210, 220}};
    for (const qol::Settings& settings : {qol::Settings{}, with(qol::Master::Original, 12), with(qol::Master::Improved, 0)}) {
        qol::setActiveSettings(settings);
        const rt::ClipPlanes hooked = qol::withFarPlane(planes);
        const rt::FogSetting fogHooked = qol::withFogDistance(fog);
        require(hooked.nearZ == planes.nearZ && hooked.farZ == rt::kFollowFarPlane, "the far plane stays 260 at x1.0");
        require(qol::viewDistance(4000.0f) == 4000.0f && qol::viewDistance(25.0f) == 25.0f, "the cutscene and free camera far planes stay at x1.0");
        require(fogHooked.start == fog.start && fogHooked.end == fog.end && fogHooked.colour == fog.colour, "the fog stays at x1.0");
        require(rt::buildFogLut(fogHooked.start, fogHooked.end, hooked) == rt::buildFogLut(fog.start, fog.end, planes),
            "the fog table is the unhooked one at x1.0");
        const rt::DistanceFadeTable table = qol::withViewDistance(rt::DistanceFadeTable{});
        const rt::DistanceFadeTable original{};
        require(table.radius == original.radius && table.fadeIn == original.fadeIn && table.fadeOut == original.fadeOut,
            "the distance fade table is unchanged at x1.0");
    }
}

void fogAndFarScaleTogether() {
    qol::setActiveSettings(with(qol::Master::Improved, 4));
    const rt::ClipPlanes planes = rt::followClipPlanes(1.0f);
    const rt::FogSetting fog{true, 30.0f, 120.0f, {200, 210, 220}};
    const rt::ClipPlanes hooked = qol::withFarPlane(planes);
    const rt::FogSetting fogHooked = qol::withFogDistance(fog);
    require(hooked.nearZ == 1.0f && hooked.farZ == 520.0f, "x2.0: far plane 520, near plane unchanged");
    require(fogHooked.start == 60.0f && fogHooked.end == 240.0f, "x2.0: fog start and end doubled");
    require(rt::buildFogLut(fogHooked.start, fogHooked.end, hooked) == rt::buildFogLut(60.0f, 240.0f, {1.0f, 520.0f}),
        "x2.0: the fog table is built from the doubled fog and far plane");
    require(rt::buildFogLut(fogHooked.start, fogHooked.end, hooked) != rt::buildFogLut(fog.start, fog.end, planes), "x2.0 changes the fog table");
    qol::setActiveSettings(with(qol::Master::Custom, 12));
    require(qol::viewDistance(rt::kFollowFarPlane) == 1040.0f && qol::viewDistance(4000.0f) == 16000.0f, "x4.0 scales every camera's far plane");
    qol::setActiveSettings(with(qol::Master::Improved, qol::viewDistanceSteps));
    require(qol::viewDistance(rt::kFollowFarPlane) == 5200.0f && qol::viewDistance(4000.0f) == 80000.0f, "x20.0: far planes 5200 and 80000 (20261102-qolmore-C)");
    const rt::FogSetting fog20 = qol::withFogDistance(rt::FogSetting{true, 30.0f, 120.0f, {200, 210, 220}});
    require(fog20.start == 600.0f && fog20.end == 2400.0f, "x20.0: fog start and end scale together");
    const rt::DistanceFadeTable table20 = qol::withViewDistance(rt::DistanceFadeTable{});
    require(table20.fadeIn == std::array<float, 3>{720.0f, 1440.0f, 2160.0f} && table20.fadeOut == std::array<float, 3>{760.0f, 1520.0f, 2280.0f}, "x20.0: fade tables");
}

void pedestriansAtTwo() {
    qol::setActiveSettings(with(qol::Master::Improved, 4));
    const rt::DistanceFadeTable original{};
    const rt::DistanceFadeTable table = qol::withViewDistance(original);
    require(table.radius == original.radius, "the radius tiers are sizes, not distances, and stay");
    require(table.fadeIn == std::array<float, 3>{72.0f, 144.0f, 216.0f} && table.fadeOut == std::array<float, 3>{76.0f, 152.0f, 228.0f},
        "x2.0 doubles the fade-in and fade-out distances");
    const rt::PedestrianSpawnWindow window = rt::pedestrianSpawnWindow(original, 0);
    const rt::PedestrianSpawnWindow scaled = rt::pedestrianSpawnWindow(table, 0);
    require(scaled.minimum == 2.0f * window.minimum && scaled.maximum == 2.0f * window.maximum, "the spawn window of tier 0 doubles (18..57 to 36..114)");

    const float distance = 60.0f;
    rt::NodeFadeFrame frame;
    frame.tier = 0;
    frame.radius = 3.0f;
    frame.distanceSquared = distance * distance;
    rt::NodeFade atOne;
    rt::nodeFadeStart(atOne);
    rt::nodeFadeStep(atOne, frame, original);
    rt::NodeFade atTwo;
    rt::nodeFadeStart(atTwo);
    rt::nodeFadeStep(atTwo, frame, table);
    require(atOne.step < 0 && atTwo.step > 0, "a pedestrian 60 m away fades in at x2.0 but not at x1.0");
    qol::setActiveSettings(qol::Settings{});
}

}

int main() {
    stepsAndLabels();
    configKeyIsStrict();
    originalKeepsOne();
    originalNumbersAtOne();
    fogAndFarScaleTogether();
    pedestriansAtTwo();
    qol::setActiveSettings(qol::Settings{});
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

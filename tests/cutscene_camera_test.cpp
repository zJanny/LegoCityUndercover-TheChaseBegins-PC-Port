#include "runtime/cutscene_camera.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace chase;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    }
}
bool near(double a, double b, double tolerance) { return std::fabs(a - b) <= tolerance; }

// Keys of the 1_1_d camera tracks (loadCutscene over csdata/models/1_1_d, recorded by 20261027-graphics-F on 2026-10-27): the focal
// length 0xc251f0c2 and the horizontal aperture 0x45218801 carry two keys with the same value, zero handle values and handle deltas
// 0 / 1 / -1; camera x 0x191a8df3 has one-third handles.
assets::AnimTrack flatTrack(std::uint32_t hash, float value) { return {hash, {{0, 0, 0, value, 0, 0}, {1, 1, 0, value, -1, 0}}}; }

assets::Animation camera1_1_d() {
    assets::Animation camera;
    camera.frames = 198;
    camera.headerRateIndex = 1;
    camera.subAnimations.push_back({flatTrack(0xc251f0c2, 55.0210f), flatTrack(0x45218801, 1.6630f),
                                    {0x191a8df3, {{0, 0, -826.2057f, -826.2057f, 1.0f / 3, -826.6672f}, {1, 1.0f / 3, -827.5905f, -827.5905f, 0, -827.5905f}}}});
    return camera;
}
}

int main() {
    const assets::Animation camera = camera1_1_d();

    // 0x252a7c with u linear in time: zero handle values pull both lens tracks to (1-u)^3 + u^3 of their value, 0.25 at the middle.
    const assets::CutsceneCamera middle = assets::cutsceneCamera(camera, 0.5f);
    check(near(middle.focalLength, 55.0210 * 0.25, 1e-4) && near(middle.horizontalAperture, 1.6630 * 0.25, 1e-5), "lens tracks at the middle");
    check(near(middle.position[0], -827.0712, 1e-3), "camera x with one-third handles is the plain cubic Bezier");

    // 0x2cd650: 2 atan(1.6630 * 25.4 / 2 / 55.0210) = 41.999 degrees across, times 4 / (3 * 400 / 240) = 33.599 degrees of pose FOV; the
    // ratio of the two lens tracks keeps it the same over the whole cutscene. Oracle (20261027-graphics-servant for 20261027-graphics-F,
    // RPC F-1, 2026-10-27, recorded from a save state just after the cutscene 1_1_d loaded): pose FOV C+0x7c = 33.5993 in every update.
    for (int frame : {0, 30, 120, 137, 197}) {
        const assets::CutsceneCamera shot = assets::cutsceneCamera(camera, float(assets::cutsceneNormalised(camera, frame)));
        check(near(runtime::cutscenePoseFieldOfView(shot), 33.5993, 2e-3), "pose FOV of 1_1_d at camera frame " + std::to_string(frame));
    }
    // 0x23458c: y scale cot(33.599 / 2) * 4 / 3, so the picture spans 2 atan(0.75 tan(16.80 degrees)) = 25.517 degrees vertically.
    // Same oracle run: camera node y scale N+0xc0 = 4.41631 (x scale 2.64979, ratio 5/3).
    check(near(1 / std::tan(runtime::cutsceneVerticalFieldOfView(middle) / 2), 4.41631, 2e-4), "projection y scale of 1_1_d");
    // 1_1_b lens: focal length 60 and aperture 1.575 (loadCutscene over csdata/models/1_1_b, 20261027-graphics-F); the same RPC run, from
    // a save state late in the cutscene 1_1_b (camera frames 298 to 391), read pose FOV 29.499 and y scale 5.0645.
    assets::CutsceneCamera lens1_1_b;
    lens1_1_b.focalLength = 60.0f;
    lens1_1_b.horizontalAperture = 1.575f;
    check(near(runtime::cutscenePoseFieldOfView(lens1_1_b), 29.499, 1e-3), "pose FOV of 1_1_b");
    check(near(1 / std::tan(runtime::cutsceneVerticalFieldOfView(lens1_1_b) / 2), 5.0645, 2e-4), "projection y scale of 1_1_b");
    check(near(runtime::cutscenePoseFieldOfView(assets::CutsceneCamera{}, 4.0f / 3.0f), 54.432, 2e-3), "handler defaults: 35 mm lens, 36 mm aperture");

    // The camera clip plays at its header rate (index 1: 30 per second, 0x108190) and holds its last frame.
    check(near(runtime::cutsceneCameraFrame(camera, 4.0), 120, 1e-9), "camera frame after 4 s");
    check(near(runtime::cutsceneCameraFrame(camera, 10.0), 197, 1e-9), "last camera frame held");
    check(runtime::cutsceneCameraFrame(camera, -1.0) == 0, "no frame before the start");

    if (failures) return EXIT_FAILURE;
    std::cout << "cutscene camera: ok\n";
    return EXIT_SUCCESS;
}

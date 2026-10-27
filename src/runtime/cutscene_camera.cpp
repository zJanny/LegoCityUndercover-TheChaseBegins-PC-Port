#include "runtime/cutscene_camera.hpp"
#include "runtime/follow_camera.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

double cutsceneCameraFrame(const assets::Animation& camera, double seconds) {
    const double last = camera.frames > 1 ? double(camera.frames - 1) : 0.0;
    return std::min(std::max(seconds, 0.0) * assets::animationFramesPerSecond(camera), last);
}

float cutscenePoseFieldOfView(const assets::CutsceneCamera& camera, float screenAspect) {
    const float halfAperture = camera.horizontalAperture * 25.4f * 0.5f;
    const float horizontal = std::atan(halfAperture / camera.focalLength) * 180.0f * 0.31830987f * 2.0f;
    return 4.0f * horizontal / (3.0f * screenAspect);
}

float cutsceneVerticalFieldOfView(const assets::CutsceneCamera& camera) {
    return FollowCamera::verticalFieldOfView(cutscenePoseFieldOfView(camera));
}

}

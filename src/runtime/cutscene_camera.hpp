#pragma once
// The camera of an in-engine cutscene: the camera frame the cutscene shows and the field of view of its camera entity.
// The cutscene camera command 0x2663c4 (pushed by 0x2a79d4 from the cutscene start 0x2d1800) takes the eye and axes of the
// camera entity's node and the pose FOV from 0x2cd650; the camera manager projects that pose FOV with 0x23458c.
// docs/re/cutscenes.md. No SDL dependency.
#include "assets/assets.hpp"

namespace chase::runtime {

// STAND-IN MOVIE-S36: the screen size 0x2cd650 divides (u32 at 0x529dc8 +4 and +8) is taken as the top screen, 400 x 240 (the oracle
// pose FOV and projection of 1_1_d and 1_1_b give 5/3; the words were not read).
constexpr float kCutsceneScreenWidthStandIn = 400.0f, kCutsceneScreenHeightStandIn = 240.0f;

// Camera frame `seconds` after the start: the camera clip plays at its own rate (0x108190) and holds its last frame.
double cutsceneCameraFrame(const assets::Animation& camera, double seconds);

// Pose FOV in degrees (0x2cd650): the horizontal angle of the lens, 2 atan(aperture * 25.4 / 2 / focal length), scaled by
// 4 / (3 * screen aspect).
float cutscenePoseFieldOfView(const assets::CutsceneCamera& camera, float screenAspect = kCutsceneScreenWidthStandIn / kCutsceneScreenHeightStandIn);

// Vertical field of view in radians of the projection 0x23458c for that pose FOV.
// STAND-IN MOVIE-S36: the director camera's FOV factor [[0x3768c0] + 0x1c] that 0x2663c4 multiplies in is taken as 1 (the oracle
// pose FOV of 1_1_d and 1_1_b gives 1; the word was not read).
float cutsceneVerticalFieldOfView(const assets::CutsceneCamera& camera);

}

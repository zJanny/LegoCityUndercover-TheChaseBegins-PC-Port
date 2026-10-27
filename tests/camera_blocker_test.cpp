// Blocker records with made-up level bounds. Provenance: 0x298cd8 copies the pose block 0x58d42c when the first record
// of its set has byte +0x4b (ldrb at 0x298cf8), and 0x181fe0 (post-update slot +0x10 of the preset 0x377110, called at
// 0x23f288) copies C+0x4c..0x88 into 0x58d42c after every camera update (llvm-objdump, 20261027-graphics-E, 2026-10-27).
#include "runtime/camera_blocker.hpp"
#include "runtime/camera_rig.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace chase;
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelBound box(const char* name, float x) {
    assets::LevelBound bound;
    bound.name = name;
    bound.shape = 3;
    bound.enabled = bound.active = 1;
    bound.centre[0] = x;
    bound.half[0] = bound.half[1] = bound.half[2] = 10;
    bound.extent = std::sqrt(300.0f);
    return bound;
}

const char* kCam =
    "CameraFollow\n{\n\tBound\n\t{\n\t\tName(CameraA)\n\t\tType(0)\n\t\tRotationY(0)\n\t\tRotationX(0)\n\t\tDistance(0)\n\t\tLookat(0,0,0)\n\t\tLocation(0,5,8)\n"
    "\t\tOffset(0,2,0)\n\t\tFOV(43)\n\t\tFilters(32,28)\n\t\tBlendTime(0)\n\t\tShakeAmp(0)\n\t\tShakeRate(0)\n\t\tShakeRoll(0)\n\t\t3DOffset(5)\n\t}\n"
    "\tBound\n\t{\n\t\tName(CameraBound_Blocker)\n\t\tType(7)\n\t\tRotationY(0)\n\t\tRotationX(0)\n\t\tDistance(0)\n\t\tLookat(0,0,0)\n\t\tLocation(0,30,0)\n"
    "\t\tOffset(0,0,0)\n\t\tFOV(60)\n\t\tFilters(32,28)\n\t\tBlendTime(0)\n\t\tShakeAmp(0)\n\t\tShakeRate(0)\n\t\tShakeRoll(0)\n\t\t3DOffset(5)\n\t}\n}\n";

FollowCameraInput at(float x) {
    FollowCameraInput input;
    input.dt = 1.0f / 30.0f;
    input.target[0] = x;
    return input;
}

bool samePose(const CameraRig& rig, const float eye[3], const float look[3], float fov) {
    for (int a = 0; a < 3; ++a)
        if (rig.eye()[a] != eye[a] || rig.lookAt()[a] != look[a]) return false;
    return rig.fieldOfViewDegrees() == fov;
}
}

int main() {
    try {
        assets::Level level;
        level.bounds.push_back(box("CameraA", 0));
        level.bounds.push_back(box("CameraBound_Blocker", 30));
        const CameraRecords records(level, kCam);
        const float inBlocker[3] = {25, 0, 0};
        const float inA[3] = {-5, 0, 0};
        require(blocksCamera(records, queryCameraRecords(records, inBlocker)), "the Blocker record blocks");
        require(!blocksCamera(records, queryCameraRecords(records, inA)), "an ordinary record does not block");
        const float outside[3] = {100, 0, 0};
        require(!blocksCamera(records, queryCameraRecords(records, outside)), "the Room record does not block");

        CameraRig rig(level, kCam);
        rig.reset(inA, 0.0f, 0.0f);
        for (int step = 0; step < 6; ++step) rig.update(at(-5.0f + 2.0f * float(step)));
        float eye[3], look[3];
        for (int a = 0; a < 3; ++a) {
            eye[a] = rig.eye()[a];
            look[a] = rig.lookAt()[a];
        }
        const float fov = rig.fieldOfViewDegrees();
        require(eye[0] < 4.9f, "the filtered camera still trails the record pose before the Blocker volume");

        for (float x : {25.0f, 28.0f, 31.0f, 35.0f, 38.0f}) {
            rig.update(at(x));
            require(rig.locked(), "the Blocker pose is a record pose command");
            require(samePose(rig, eye, look, fov), "the Blocker record keeps the last camera pose");
        }

        rig.update(at(5.0f));
        require(rig.eye()[0] > eye[0] && rig.fieldOfViewDegrees() == 43.0f, "back in an ordinary record the camera moves again");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "camera-blocker: ok\n";
    return 0;
}

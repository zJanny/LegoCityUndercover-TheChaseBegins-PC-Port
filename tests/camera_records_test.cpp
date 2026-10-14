// Synthetic checks of the camera record selection (0x1d34ac, 0x1b4a30, 0x19b8c8), the record pose (0x298cd8) and the lock
// icon timing (0x23cc00). Expected values are derived by hand from the recovered formulas.
#include "runtime/camera_lock_icon.hpp"
#include "runtime/camera_records.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace chase;
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

assets::LevelBound box(const char* name, float x, float y, float z, float hx, float hy, float hz) {
    assets::LevelBound bound;
    bound.name = name;
    bound.shape = 3;
    bound.enabled = bound.active = 1;
    bound.centre[0] = x, bound.centre[1] = y, bound.centre[2] = z;
    bound.half[0] = hx, bound.half[1] = hy, bound.half[2] = hz;
    bound.extent = std::sqrt(hx * hx + hy * hy + hz * hz);
    return bound;
}

const char* kCam = "CameraFollow\n{\n\tBound\n\t{\n\t\tName(CameraA)\n\t\tType(0)\n\t\tRotationY(0)\n\t\tRotationX(0)\n\t\tDistance(0)\n\t\tLookat(0,0,0)\n\t\tLocation(0,5,8)\n\t\tOffset(0,2,0)\n\t\tFOV(43)\n"
                   "\t\tFilters(32,28)\n\t\tBlendTime(1.0)\n\t\tShakeAmp(0)\n\t\tShakeRate(0)\n\t\tShakeRoll(0)\n\t\t3DOffset(5)\n\t}\n"
                   "\tBound\n\t{\n\t\tName(CameraB)\n\t\tType(7)\n\t\tRotationY(0)\n\t\tRotationX(0)\n\t\tDistance(0)\n\t\tLookat(0,0,0)\n\t\tLocation(10,20,30)\n\t\tOffset(1,2,3)\n\t\tFOV(50)\n"
                   "\t\tFilters(32,28)\n\t\tBlendTime(1.0)\n\t\tShakeAmp(0)\n\t\tShakeRate(0)\n\t\tShakeRoll(0)\n\t\t3DOffset(5)\n\t}\n}\n";
}

int main() {
    try {
        assets::Level level;
        level.bounds.push_back(box("Other", 0, 0, 0, 100, 100, 100));
        level.bounds.push_back(box("CameraA", 0, 0, 0, 10, 10, 10));
        level.bounds.push_back(box("CameraLookAt_x", 0, 0, 0, 10, 10, 10));
        level.bounds.push_back(box("CameraB", 5, 0, 0, 10, 10, 10));
        CameraRecords records(level, kCam);
        require(records.records().size() == 2, "records are the Camera bounds without LookAt");
        require(records.records()[0].type == 0x80 && records.records()[1].type == (7 | 0x80), "Type is stored with the loaded bit");

        const float outside[3] = {50, 0, 0};
        CameraRecordSelector selector(records);
        require(selector.update(outside) == CameraCommand::Follow, "a point in no volume selects the follow controller");
        require(queryCameraRecords(records, outside).records[0] == kRoomRecord, "the Room record is the fallback");

        const float inA[3] = {-8, 0, 0};
        require(selector.update(inA) == CameraCommand::RecordPose, "a point in a volume selects the record pose");
        const RecordPose pose = evaluateRecordPose(records, selector.blend(), inA);
        require(near(pose.eye[0], -8) && near(pose.eye[1], 5) && near(pose.eye[2], 8), "Type 0: eye = target + Location");
        require(near(pose.look[0], -8) && near(pose.look[1], 2) && near(pose.look[2], 0), "Type 0: look = target + Offset");
        require(near(pose.fovDegrees, 43), "FOV");
        require(selector.update(outside) == CameraCommand::Follow, "leaving the volume returns to the follow controller");

        const float inBoth[3] = {0, 0, 0};
        const CameraRecordSet both = queryCameraRecords(records, inBoth);
        require(both.records.size() == 2, "overlap depth 15 keeps both records");
        require(near(both.weights[0] + both.weights[1], 1.0f), "weights sum to 1");

        assets::Level shallow;
        shallow.bounds.push_back(box("CameraA", 0, 0, 0, 10, 10, 10));
        shallow.bounds.push_back(box("CameraB", 19.8f, 0, 0, 10, 10, 10));
        CameraRecords shallowRecords(shallow, kCam);
        const float inShallow[3] = {9.9f, 0, 0};
        require(queryCameraRecords(shallowRecords, inShallow).records.size() == 1, "overlap depth 0.2 drops the later record");

        assets::Level focus;
        focus.bounds.push_back(box("CameraA", 0, 0, 0, 10, 10, 10));
        focus.bounds.push_back(box("CameraFocus_B", 5, 0, 0, 10, 10, 10));
        CameraRecords focusRecords(focus, "");
        const CameraRecordSet focused = queryCameraRecords(focusRecords, inBoth);
        require(focused.records.size() == 1 && focused.records[0] == 1, "a Focus record wins alone");

        CameraLockIcon icon;
        require(icon.alpha() == 0.0f, "icon starts hidden");
        icon.trigger();
        icon.update(0.15f);
        require(near(icon.alpha(), 0.5f), "fade in over 0.3 s");
        icon.update(0.2f);
        require(icon.state() == CameraLockIcon::State::Shown && icon.alpha() == 1.0f, "shown after the fade");
        icon.update(0.4f);
        icon.trigger();
        icon.update(0.4f);
        require(icon.state() == CameraLockIcon::State::Shown, "a trigger renews the 0.5 s hold");
        icon.update(0.2f);
        require(icon.state() == CameraLockIcon::State::FadingOut, "fade out after the hold");
        icon.update(0.15f);
        require(near(icon.alpha(), 0.5f), "fade out over 0.3 s");
        icon.trigger();
        require(icon.state() == CameraLockIcon::State::FadingIn && near(icon.alpha(), 0.5f), "a trigger while fading out resumes from the same alpha");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

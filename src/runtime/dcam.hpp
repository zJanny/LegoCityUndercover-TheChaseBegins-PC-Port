#pragma once
// Director camera (DCam, docs/re/dcam.md): the camera command that the script functions StartDCam / StartDCamTrack push
// on the camera command stack (0x21cc80), its command routine 0x2665e4 / 0x271130 (pose from level paths, eased by the
// curve of 0x221b48), the phase machine 0x2028c8 / 0x202814 (start wait, run, end wait, done), the busy test 0x236104 that
// WaitForDCam polls and the end 0x196784 that hands back to the gameplay camera. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/camera_records.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr float kDirectorDefaultBlendSeconds = 1.6f;
constexpr float kDirectorFieldOfViewDegrees = 43.0f;
constexpr float kDirectorOffset3d = 8.0f;

enum DirectorSnap : int { kSnapNone = 0, kSnapStart = 1, kSnapEnd = 2, kSnapBoth = 3 };

// Catmull-Rom spline through the points of a type 0 path (0x1c17c0): `position` runs from 0 to point count - 1 (clamped
// for an open path, wrapped for a closed one).
void sampleDirectorPath(const assets::LevelPath& path, float position, float out[3]);
// First path whose name hash matches (0x22a3b0), or nullptr.
const assets::LevelPath* findLevelPath(const assets::Level& level, const std::string& name);

// The command the director camera asks of the camera stack (0x224080 for a start, the request 0x2c5918 for the end).
struct DirectorRequest {
    enum class Kind { Start, End } kind = Kind::Start;
    float fadeSeconds = 0;
    bool instant = false;
};

class DirectorCamera {
public:
    // World point a Track camera looks at: the node matrix of the object transforms its look offset (0x221a1c, 0x250da8).
    using ObjectPoint = std::function<bool(const std::string& object, float out[3])>;

    // StartDCam 0x194fd8 (camera path, look path) and StartDCamTrack 0x1da810 (camera path, look object). `snap` is the
    // script's snap argument; false when the combination is not one of the two recovered commands.
    bool startPaths(const assets::LevelPath* camera, const assets::LevelPath* look, float seconds, int snap);
    bool startTrack(const assets::LevelPath* camera, const std::string& lookObject, float seconds, int snap);
    // EndDCam 0x17cbd4: only while a director camera is busy; ends it at the next update without a blend.
    void requestEnd();
    // SetDCamStartWait 0x26fdf0 / SetDCamEndWait 0x1da690: the value applies to the next start only (0x189548 clears it).
    void setStartWait(float seconds) { startWait_ = seconds; }
    void setEndWait(float seconds) { endWait_ = seconds; }
    // 0x236104 without its cutscene part (0x2297b0, SCRIPT-29): a director camera has started and has not ended.
    bool busy() const { return active_; }
    // DCamActive 0x1a2e48: flag bit 1 of the object's component (what reads it is not recovered).
    void setObjectActiveDuringDirector(const std::string& object, bool active);
    bool objectActiveDuringDirector(const std::string& object) const;
    // ShakeScreen 0x1b2690 stores its arguments here; nothing draws the shake yet (stand-in).
    struct Shake {
        float amount = 0, seconds = 0;
        bool damp = false;
        bool pending = false;
    };
    void shakeScreen(float amount, float seconds, bool damp);
    const Shake& shake() const { return shake_; }
    // SnapCamera 0x1a30fc: asks the follow camera to take its start pose again.
    void snapFollowCamera() { snapRequested_ = true; }
    bool takeSnapRequest();

    // One game update: the command routine (the phase machine and the pose), then the end test 0x196784.
    void update(float dt, const ObjectPoint& objectPoint);
    const RecordPose& pose() const { return pose_; }
    // Requests since the last call, in order.
    std::vector<DirectorRequest> takeRequests();

    enum class Phase { StartWait, Run, EndWait, Done };
    Phase phase() const { return phase_; }
    float progress() const { return progress_; }

private:
    bool begin(float seconds, int snap);
    bool routine(const ObjectPoint& objectPoint);
    void startTimer(float seconds);
    bool timerDone();
    void evaluate(const ObjectPoint& objectPoint, float eased);

    bool active_ = false;
    bool endRequested_ = false;
    bool doneFlag_ = false;
    Phase phase_ = Phase::Done;
    float startWait_ = 0, endWait_ = 0;
    float blendIn_ = kDirectorDefaultBlendSeconds, blendOut_ = kDirectorDefaultBlendSeconds;
    float seconds_ = 0;
    int snap_ = 0;
    const assets::LevelPath* cameraPath_ = nullptr;
    const assets::LevelPath* lookPath_ = nullptr;
    std::string lookObject_;
    long long clockMs_ = 0, timerStartMs_ = 0, timerDurationMs_ = 0;
    bool timerRunning_ = false;
    float progress_ = 0;
    RecordPose pose_;
    std::vector<DirectorRequest> requests_;
    std::map<std::string, bool> activeDuringDirector_;
    Shake shake_;
    bool snapRequested_ = false;
};

// Script-function handlers: `args` are the strings of the call, `read` turns a number or variable name into a value.
using DirectorReadValue = std::function<float(const std::string&)>;
void startDCamNative(DirectorCamera& director, const assets::Level& level, const std::vector<std::string>& args, const DirectorReadValue& read);
void startDCamTrackNative(DirectorCamera& director, const assets::Level& level, const std::vector<std::string>& args, const DirectorReadValue& read);
void dcamActiveNative(DirectorCamera& director, const std::vector<std::string>& args, const DirectorReadValue& read);
void shakeScreenNative(DirectorCamera& director, const std::vector<std::string>& args, const DirectorReadValue& read);

}

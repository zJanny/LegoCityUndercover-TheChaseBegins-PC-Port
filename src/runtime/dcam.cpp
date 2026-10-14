#include "runtime/dcam.hpp"
#include "runtime/camera_rig.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace chase::runtime {

namespace {
constexpr float kTicksPerSecond = 1000.0f;

int wrapIndex(int index, int count) {
    const int remainder = index % count;
    return remainder < 0 ? remainder + count : remainder;
}
}

void sampleDirectorPath(const assets::LevelPath& path, float position, float out[3]) {
    const int count = int(path.points.size());
    if (count == 0) return;
    if (!path.closed) {
        if (!(position >= 0.0f)) position = 0.0f;
        if (!(position <= float(count - 1))) position = float(count - 1);
    }
    const int base = int(std::floor(position));
    int before = base - 1, after = base + 1, beyond = base + 2, at = base;
    if (!path.closed) {
        before = std::max(before, 0);
        after = std::min(after, count - 1);
        beyond = std::min(beyond, count - 1);
    } else {
        before = wrapIndex(before, count);
        at = wrapIndex(at, count);
        after = wrapIndex(after, count);
        beyond = wrapIndex(beyond, count);
    }
    const float t = position - float(base);
    const float square = t * t;
    const float cube = square * t;
    for (int a = 0; a < 3; ++a) {
        const float p1 = path.points[std::size_t(at)][std::size_t(a)];
        const float p2 = path.points[std::size_t(after)][std::size_t(a)];
        const float p0 = path.points[std::size_t(before)][std::size_t(a)];
        const float p3 = path.points[std::size_t(beyond)][std::size_t(a)];
        out[a] = p1 * ((cube * 2.0f - square * 3.0f) + 1.0f) + (cube * -2.0f + square * 3.0f) * p2 +
                 (p2 - p0) * ((cube - square * 2.0f) + t) * 0.5f + (p3 - p1) * (cube - square) * 0.5f;
    }
}

const assets::LevelPath* findLevelPath(const assets::Level& level, const std::string& name) {
    const std::uint32_t hash = assets::nameHash(name);
    for (const assets::LevelPath& path : level.paths)
        if (path.nameHash == hash) return &path;
    return nullptr;
}

void DirectorCamera::startTimer(float seconds) {
    timerStartMs_ = clockMs_;
    timerDurationMs_ = static_cast<long long>(static_cast<unsigned>(kTicksPerSecond * seconds));
    if (timerDurationMs_ == 0) timerDurationMs_ = 1;
    timerRunning_ = true;
}

bool DirectorCamera::timerDone() {
    if (!timerRunning_) return false;
    if (timerDurationMs_ > clockMs_ - timerStartMs_) return false;
    timerRunning_ = false;
    return true;
}

bool DirectorCamera::begin(float seconds, int snap) {
    active_ = true;
    endRequested_ = false;
    doneFlag_ = false;
    seconds_ = seconds;
    snap_ = snap;
    if (startWait_ <= 0.0f) {
        phase_ = Phase::Run;
        startTimer(seconds);
    } else {
        phase_ = Phase::StartWait;
        startTimer(startWait_);
    }
    startWait_ = 0;
    endWait_ = 0;
    blendOut_ = (snap & kSnapEnd) ? 0.0f : kDirectorDefaultBlendSeconds;
    DirectorRequest request;
    request.kind = DirectorRequest::Kind::Start;
    request.fadeSeconds = (snap & kSnapStart) ? 0.0f : blendIn_;
    request.instant = request.fadeSeconds <= 0.0f;
    requests_.push_back(request);
    return true;
}

bool DirectorCamera::startPaths(const assets::LevelPath* camera, const assets::LevelPath* look, float seconds, int snap) {
    if (!camera || !look) return false;
    cameraPath_ = camera;
    lookPath_ = look;
    lookObject_.clear();
    return begin(seconds, snap);
}

bool DirectorCamera::startTrack(const assets::LevelPath* camera, const std::string& lookObject, float seconds, int snap) {
    if (!camera || lookObject.empty()) return false;
    cameraPath_ = camera;
    lookPath_ = nullptr;
    lookObject_ = lookObject;
    return begin(seconds, snap);
}

void DirectorCamera::requestEnd() {
    if (active_) endRequested_ = true;
}

void DirectorCamera::setObjectActiveDuringDirector(const std::string& object, bool active) { activeDuringDirector_[object] = active; }

bool DirectorCamera::objectActiveDuringDirector(const std::string& object) const {
    const auto found = activeDuringDirector_.find(object);
    return found != activeDuringDirector_.end() && found->second;
}

void DirectorCamera::shakeScreen(float amount, float seconds, bool damp) {
    shake_.amount = amount;
    shake_.seconds = seconds;
    shake_.damp = damp;
    shake_.pending = true;
}

bool DirectorCamera::takeSnapRequest() {
    const bool requested = snapRequested_;
    snapRequested_ = false;
    return requested;
}

std::vector<DirectorRequest> DirectorCamera::takeRequests() {
    std::vector<DirectorRequest> taken;
    taken.swap(requests_);
    return taken;
}

void DirectorCamera::evaluate(const ObjectPoint& objectPoint, float eased) {
    const float cameraSpan = float(cameraPath_->points.size() - (cameraPath_->closed ? 0 : 1));
    sampleDirectorPath(*cameraPath_, eased * cameraSpan, pose_.eye);
    if (lookPath_) {
        const float lookSpan = float(lookPath_->points.size() - (lookPath_->closed ? 0 : 1));
        sampleDirectorPath(*lookPath_, eased * lookSpan, pose_.look);
    } else if (objectPoint) {
        float point[3];
        if (objectPoint(lookObject_, point))
            for (int a = 0; a < 3; ++a) pose_.look[a] = point[a];
    }
}

bool DirectorCamera::routine(const ObjectPoint& objectPoint) {
    if (!cameraPath_) return false;
    pose_.fovDegrees = kDirectorFieldOfViewDegrees;
    pose_.offset3d = kDirectorOffset3d;

    if (phase_ == Phase::StartWait && timerDone()) {
        phase_ = Phase::Run;
        startTimer(seconds_);
    } else if (phase_ == Phase::EndWait && timerDone()) {
        phase_ = Phase::Done;
    }
    const bool running = phase_ == Phase::StartWait || phase_ == Phase::Run;

    float eased = 1.0f;
    if (running) {
        float raw = 0.0f;
        if (phase_ == Phase::Run) {
            if (!timerRunning_) raw = 0.0f;
            else if (timerDurationMs_ > clockMs_ - timerStartMs_) raw = float(clockMs_ - timerStartMs_) / float(timerDurationMs_);
            else {
                timerRunning_ = false;
                raw = 1.0f;
            }
        }
        eased = fadeCurve(raw);
        progress_ = raw;
    }
    if (running || lookPath_) evaluate(objectPoint, eased);

    if (phase_ == Phase::Done) return true;
    if (phase_ == Phase::Run && !timerRunning_) {
        if (endWait_ == 0.0f) {
            phase_ = Phase::Done;
            return true;
        }
        phase_ = Phase::EndWait;
        startTimer(endWait_);
    }
    return false;
}

void DirectorCamera::update(float dt, const ObjectPoint& objectPoint) {
    clockMs_ += std::llround(double(dt) * 1000.0);
    if (active_ && (endRequested_ || doneFlag_)) {
        DirectorRequest request;
        request.kind = DirectorRequest::Kind::End;
        request.fadeSeconds = blendOut_;
        request.instant = endRequested_;
        requests_.push_back(request);
        active_ = false;
        endRequested_ = false;
        doneFlag_ = false;
    }
    if (routine(objectPoint) && active_) doneFlag_ = true;
}

namespace {
std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

void startDCamNative(DirectorCamera& director, const assets::Level& level, const std::vector<std::string>& args, const DirectorReadValue& read) {
    if (args.size() < 4) return;
    director.startPaths(findLevelPath(level, args[0]), findLevelPath(level, args[1]), read(args[3]), static_cast<int>(read(args[2])) & 0xff);
}

void startDCamTrackNative(DirectorCamera& director, const assets::Level& level, const std::vector<std::string>& args, const DirectorReadValue& read) {
    if (args.size() < 4) return;
    director.startTrack(findLevelPath(level, args[0]), lowerCase(args[1]), read(args[3]), static_cast<int>(read(args[2])) & 0xff);
}

void dcamActiveNative(DirectorCamera& director, const std::vector<std::string>& args, const DirectorReadValue& read) {
    if (args.size() >= 2) director.setObjectActiveDuringDirector(lowerCase(args[0]), read(args[1]) != 0.0f);
}

void shakeScreenNative(DirectorCamera& director, const std::vector<std::string>& args, const DirectorReadValue& read) {
    if (args.size() >= 3) director.shakeScreen(read(args[0]), read(args[1]), static_cast<int>(read(args[2])) != 0);
}

}

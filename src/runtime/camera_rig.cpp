#include "runtime/camera_rig.hpp"
#include <algorithm>

namespace chase::runtime {

namespace {
constexpr std::size_t kStackCapacity = 4;
constexpr float kFollowFadeSeconds = 0.5f;
constexpr float kUnfilteredFactor = 1.0f;
constexpr float kDefaultFilterFactor = 0.3f;

float lerp(float from, float to, float t) { return from + (to - from) * t; }
}

float poseFilterFactor(std::uint8_t filterByte) {
    if (filterByte == 0) return kDefaultFilterFactor;
    const float remaining = 1.0f - float(filterByte) * 0.02f;
    return 1.0f - (1.0f - remaining * remaining);
}

float fadeCurve(float progress) { return progress * progress * (3.0f - 2.0f * progress); }

CameraRig::CameraRig(const assets::Level& level, const std::string& camText) : records_(level, camText), selector_(records_) {}

RecordPose CameraRig::entryPose(const Entry& entry, const float target[3]) const {
    if (entry.kind == Entry::Kind::Record) return evaluateRecordPose(records_, entry.set, target);
    if (entry.kind == Entry::Kind::Director) return directorPose_;
    RecordPose pose;
    for (int a = 0; a < 3; ++a) {
        pose.look[a] = follow_.lookAt()[a];
        pose.eye[a] = follow_.eye()[a];
    }
    pose.fovDegrees = FollowCamera::kFieldOfViewDegrees;
    return pose;
}

RecordPose CameraRig::weightedPose(const std::vector<Entry>& entries) {
    RecordPose combined;
    float total = 0.0f;
    for (const Entry& entry : entries) {
        for (int a = 0; a < 3; ++a) {
            combined.look[a] += entry.pose.look[a] * entry.weight;
            combined.eye[a] += entry.pose.eye[a] * entry.weight;
        }
        combined.fovDegrees += entry.pose.fovDegrees * entry.weight;
        combined.offset3d += entry.pose.offset3d * entry.weight;
        total += entry.weight;
    }
    const float scale = 1.0f / total;
    for (int a = 0; a < 3; ++a) {
        combined.look[a] *= scale;
        combined.eye[a] *= scale;
    }
    combined.fovDegrees *= scale;
    combined.offset3d *= scale;
    return combined;
}

void CameraRig::mergeOldestTwo() {
    Entry& newer = stack_[stack_.size() - 2];
    const Entry older = stack_.back();
    const float sum = newer.weight + older.weight;
    std::vector<Entry> pair{newer, older};
    pair[0].weight = newer.weight / sum;
    pair[1].weight = older.weight / sum;
    newer.pose = weightedPose(pair);
    newer.weight = sum;
    newer.frozen = true;
    stack_.pop_back();
}

void CameraRig::push(Entry entry, bool clearStack) {
    if (clearStack) stack_.clear();
    if (stack_.size() == kStackCapacity) mergeOldestTwo();
    stack_.insert(stack_.begin(), std::move(entry));
}

void CameraRig::reset(const float target[3], float yaw, float facing) {
    follow_.reset(target, yaw, facing);
    selector_ = CameraRecordSelector(records_);
    selector_.update(target);
    stack_.clear();
    Entry entry;
    if (selector_.command() == CameraCommand::RecordPose) {
        entry.kind = Entry::Kind::Record;
        entry.set = selector_.blend();
        eyeFactor_ = poseFilterFactor(recordEyeFilter(records_, entry.set));
        lookFactor_ = poseFilterFactor(recordLookFilter(records_, entry.set));
    } else {
        eyeFactor_ = lookFactor_ = kUnfilteredFactor;
    }
    entry.pose = entryPose(entry, target);
    stack_.push_back(entry);
    output_ = entry.pose;
}

void CameraRig::setDirector(const RecordPose& pose, std::vector<DirectorRequest> requests) {
    directorPose_ = pose;
    directorRequests_ = std::move(requests);
}

void CameraRig::snapFollow(const float target[3], float facing) { follow_.reset(target, poseYaw(output_), facing); }

void CameraRig::update(const FollowCameraInput& input) {
    selector_.update(input.target);
    std::vector<DirectorRequest> applied = std::move(delayedDirectorEnds_);
    delayedDirectorEnds_.clear();
    for (DirectorRequest& request : directorRequests_)
        (request.kind == DirectorRequest::Kind::End ? delayedDirectorEnds_ : applied).push_back(std::move(request));
    for (const DirectorRequest& request : applied) {
        Entry entry;
        const float rate = request.instant || request.fadeSeconds <= 0.0f ? 0.0f : 1.0f / request.fadeSeconds;
        if (request.kind == DirectorRequest::Kind::Start) {
            entry.kind = Entry::Kind::Director;
        } else if (selector_.command() == CameraCommand::RecordPose) {
            entry.kind = Entry::Kind::Record;
            entry.set = selector_.blend();
        } else {
            entry.kind = Entry::Kind::Follow;
            follow_.reset(input.target, poseYaw(output_), input.targetFacing);
        }
        entry.progress = 0.0f;
        entry.progressPerSecond = rate;
        push(std::move(entry), request.instant || request.fadeSeconds <= 0.0f);
    }
    directorRequests_.clear();
    if (selector_.handedOver() && !(!stack_.empty() && stack_.front().kind == Entry::Kind::Director)) {
        Entry entry;
        if (selector_.command() == CameraCommand::RecordPose) {
            entry.kind = Entry::Kind::Record;
            entry.set = selector_.blend();
            const float fade = recordFadeSeconds(records_, entry.set);
            entry.progress = 0.0f;
            entry.progressPerSecond = fade > 0.0f ? 1.0f / fade : 0.0f;
            eyeFactor_ = poseFilterFactor(recordEyeFilter(records_, entry.set));
            lookFactor_ = poseFilterFactor(recordLookFilter(records_, entry.set));
            push(std::move(entry), fade <= 0.0f);
        } else if (!stack_.empty() && stack_.front().kind != Entry::Kind::Follow) {
            entry.kind = Entry::Kind::Follow;
            entry.progress = 0.0f;
            entry.progressPerSecond = 1.0f / kFollowFadeSeconds;
            eyeFactor_ = lookFactor_ = kUnfilteredFactor;
            follow_.reset(input.target, poseYaw(output_), input.targetFacing);
            push(std::move(entry), false);
        }
    } else if (selector_.command() == CameraCommand::RecordPose && locked()) {
        stack_.front().set = selector_.blend();
    }

    FollowCameraInput followInput = input;
    if (locked()) followInput.stickTurn = followInput.shoulderTurn = 0.0f;
    const bool followRuns = std::any_of(stack_.begin(), stack_.end(), [](const Entry& e) { return e.kind == Entry::Kind::Follow; });
    if (followRuns) follow_.update(followInput);

    float remaining = 1.0f;
    for (std::size_t i = 0; i < stack_.size(); ++i) {
        Entry& entry = stack_[i];
        if (i + 1 < stack_.size()) {
            entry.progress = entry.progressPerSecond > 0.0f ? std::min(entry.progress + input.dt * entry.progressPerSecond, 1.0f) : 1.0f;
            entry.weight = fadeCurve(entry.progress) * remaining;
            remaining -= entry.weight;
        } else {
            entry.weight = remaining;
        }
    }
    if (stack_.size() > 1 && stack_.back().weight == 0.0f) stack_.pop_back();

    for (Entry& entry : stack_)
        if (!entry.frozen) entry.pose = entryPose(entry, input.target);
    const RecordPose combined = weightedPose(stack_);

    for (int a = 0; a < 3; ++a) {
        output_.eye[a] = lerp(output_.eye[a], combined.eye[a], eyeFactor_);
        output_.look[a] = lerp(output_.look[a], combined.look[a], lookFactor_);
    }
    output_.fovDegrees = combined.fovDegrees;
    output_.offset3d = lerp(output_.offset3d, combined.offset3d, eyeFactor_);
}

float CameraRig::verticalFieldOfView() const { return FollowCamera::verticalFieldOfView(output_.fovDegrees); }

}

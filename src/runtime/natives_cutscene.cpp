#include "runtime/natives_cutscene.hpp"
#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {
struct Entry {
    const char* name;
    CutsceneNativeKind kind;
    unsigned arguments;
};

constexpr Entry kEntries[] = {
    {"dcamcutsceneload", CutsceneNativeKind::Load, 1},
    {"dcamcutsceneplay", CutsceneNativeKind::Play, 5},
    {"dcamcutscenewait", CutsceneNativeKind::Wait, 1},
    {"dcamcutscenefree", CutsceneNativeKind::Free, 1},
    {"dcamcutscenefadedisable", CutsceneNativeKind::FadeDisable, 0},
    {"playcutscene", CutsceneNativeKind::Movie, 1},
    {"playcutscenenofadebackin", CutsceneNativeKind::MovieNoFadeBackIn, 1},
};
}

std::optional<CutsceneNativeKind> cutsceneNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> cutsceneNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned cutsceneNativeArguments(CutsceneNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

int CutsceneNativeState::find(const std::string& name) const {
    for (std::size_t i = 0; i < slots_.size(); ++i)
        if (slots_[i].used && slots_[i].name == name) return int(i);
    return -1;
}

bool CutsceneNativeState::loaded(const std::string& name) const { return find(name) >= 0; }

void CutsceneNativeState::load(const std::string& name) {
    if (find(name) >= 0) return;
    const auto free = std::find_if(slots_.begin(), slots_.end(), [](const Slot& slot) { return !slot.used; });
    if (free == slots_.end() || !lengthSource_) return;
    const std::optional<float> length = lengthSource_(name);
    if (!length) return;
    *free = {name, *length, true};
    requests_.push_back({CutsceneRequest::Kind::Load, name});
}

bool CutsceneNativeState::play(const std::string& name, const std::array<bool, 4>& flags) {
    if (find(name) < 0) load(name);
    const int index = find(name);
    if (index < 0) return false;
    CutscenePlaying started;
    started.name = name;
    started.length = slots_[std::size_t(index)].length;
    started.flags = flags;
    started.startFadeLeft = flags[0] ? kCutsceneFadeSeconds : 0.0f;
    modeEntered_ = !flags[0];
    endPhase_ = EndPhase::None;
    fadeRequest_ = CutsceneFadeRequest::None;
    playing_ = started;
    requests_.push_back({CutsceneRequest::Kind::Play, name});
    return true;
}

void CutsceneNativeState::end() {
    playing_.reset();
    endPhase_ = EndPhase::None;
}

void CutsceneNativeState::free(const std::string& name) {
    const int index = find(name);
    if (index < 0) return;
    if (playing_ && playing_->name == name) end();
    slots_[std::size_t(index)] = Slot{};
    requests_.push_back({CutsceneRequest::Kind::Free, name});
}

bool CutsceneNativeState::busy(const std::string& name) const {
    if (!playing_) return false;
    const int index = find(playing_->name);
    if (index < 0) return false;
    return name.empty() || slots_[std::size_t(index)].name == name;
}

void CutsceneNativeState::update(float dt, bool screenFadeRunning) {
    if (!playing_) return;
    if (playing_->startFadeLeft > 0) {
        playing_->startFadeLeft -= dt;
        if (playing_->startFadeLeft <= 0) modeEntered_ = true;
        return;
    }
    switch (endPhase_) {
    case EndPhase::None:
        playing_->elapsed += dt;
        if (playing_->elapsed >= playing_->length) {
            playing_->elapsed = playing_->length;
            endPhase_ = EndPhase::Fading;
            fadeRequest_ = CutsceneFadeRequest::Out;
        }
        break;
    case EndPhase::Fading:
        if (fadeRequest_ == CutsceneFadeRequest::None && !screenFadeRunning) {
            endPhase_ = EndPhase::Waiting;
            exitWaitLeft_ = kCutsceneExitWaitSeconds;
        }
        break;
    case EndPhase::Waiting:
        exitWaitLeft_ -= dt;
        if (exitWaitLeft_ <= 0) {
            if (playing_->name != kCutsceneWithoutExitFade) fadeRequest_ = CutsceneFadeRequest::In;
            end();
        }
        break;
    }
}

CutsceneFadeRequest CutsceneNativeState::takeFadeRequest() {
    const CutsceneFadeRequest request = fadeRequest_;
    fadeRequest_ = CutsceneFadeRequest::None;
    return request;
}

bool CutsceneNativeState::takeModeEntered() {
    const bool entered = modeEntered_;
    modeEntered_ = false;
    return entered;
}

std::vector<CutsceneRequest> CutsceneNativeState::takeRequests() {
    std::vector<CutsceneRequest> taken;
    taken.swap(requests_);
    return taken;
}

void CutsceneNativeState::startMovie(const std::string& name, bool fadeBackIn) {
    movieFinished_ = false;
    if (!hostPlaysMovies_) {
        movieFinished_ = true;
        return;
    }
    movie_ = MovieRequest{name, fadeBackIn};
}

void CutsceneNativeState::finishMovie() {
    movie_.reset();
    movieFinished_ = true;
}

}

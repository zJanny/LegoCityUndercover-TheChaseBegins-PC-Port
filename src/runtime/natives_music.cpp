#include "runtime/natives_music.hpp"
#include <cmath>
#include <limits>

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    MusicNativeKind kind;
    unsigned arguments;
};

constexpr Entry kEntries[] = {
    {"overridemusic", MusicNativeKind::OverrideMusic, 1},
    {"automaticmusic", MusicNativeKind::AutomaticMusic, 0},
    {"stopmusic", MusicNativeKind::StopMusic, 1},
};

std::int32_t toInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}

}

std::optional<MusicNativeKind> musicNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> musicNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned musicNativeArguments(MusicNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry.arguments;
    return 0;
}

void MusicNativeState::run(MusicNativeKind kind, const std::vector<float>& values) {
    const float first = values.empty() ? 0.0f : values[0];
    switch (kind) {
    case MusicNativeKind::OverrideMusic: {
        const unsigned track = static_cast<std::uint16_t>(toInt(first));
        if (track >= trackCount_) {
            ++outOfRange_;
            break;
        }
        overrideActive_ = true;
        MusicRequest request;
        request.kind = MusicRequest::Kind::Start;
        request.track = track;
        request.loop = true;
        request.crossfade = true;
        request.fade = kOverrideMusicFade;
        requests_.push_back(request);
        break;
    }
    case MusicNativeKind::AutomaticMusic:
        overrideActive_ = false;
        break;
    case MusicNativeKind::StopMusic: {
        MusicRequest request;
        if (first == 0.0f) {
            request.kind = MusicRequest::Kind::StopNow;
        } else {
            request.kind = MusicRequest::Kind::FadeOut;
            const float step = 1.0f / (kStopMusicFramesPerSecond * first);
            request.step = step <= 0.0f ? kStopMusicMinimumStep : step;
        }
        requests_.push_back(request);
        break;
    }
    }
}

std::vector<MusicRequest> MusicNativeState::takeRequests() {
    std::vector<MusicRequest> taken;
    taken.swap(requests_);
    return taken;
}

}

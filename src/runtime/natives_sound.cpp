#include "runtime/natives_sound.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace chase::runtime {
namespace {

struct Entry {
    const char* name;
    SoundNativeKind kind;
    unsigned arguments;
    int objectArgument;
};

constexpr Entry kEntries[] = {
    {"playsound", SoundNativeKind::PlaySound, 2, -1},
    {"playsoundgo", SoundNativeKind::PlaySoundGO, 3, 1},
    {"playsoundatobject", SoundNativeKind::PlaySoundAtObject, 2, 0},
    {"stopsound", SoundNativeKind::StopSound, 3, -1},
    {"stopsoundgo", SoundNativeKind::StopSoundGO, 3, 1},
    {"registersound", SoundNativeKind::RegisterSound, 2, -1},
    {"scalesoundfrequency", SoundNativeKind::ScaleSoundFrequency, 3, -1},
    {"setsoundvolume", SoundNativeKind::SetSoundVolume, 3, -1},
    {"vehiclestopsounds", SoundNativeKind::VehicleStopSounds, 1, 0},
    {"vehicleenabledrivingmusic", SoundNativeKind::VehicleEnableDrivingMusic, 2, 0},
};

const Entry& entryOf(SoundNativeKind kind) {
    return *std::find_if(std::begin(kEntries), std::end(kEntries), [&](const Entry& e) { return e.kind == kind; });
}

std::int32_t toInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}

float valueAt(const std::vector<float>& values, std::size_t index) { return index < values.size() ? values[index] : 0.0f; }

}

std::optional<SoundNativeKind> soundNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> soundNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned soundNativeArguments(SoundNativeKind kind) { return entryOf(kind).arguments; }

int soundNativeObjectArgument(SoundNativeKind kind) { return entryOf(kind).objectArgument; }

std::uint16_t SoundNativeState::ownerOf(const std::string& object) {
    const auto found = owners_.find(object);
    if (found != owners_.end()) return found->second;
    const std::uint16_t owner = std::uint16_t(kObjectOwnerBase + owners_.size());
    owners_[object] = owner;
    return owner;
}

bool SoundNativeState::drivingMusicEnabled(const std::string& vehicle) const {
    const auto found = drivingMusic_.find(vehicle);
    return found != drivingMusic_.end() && found->second;
}

void SoundNativeState::run(SoundNativeKind kind, const std::vector<float>& values, const std::vector<std::string>& objects, const PositionLookup& position) {
    const auto object = [&](std::size_t index) { return index < objects.size() ? objects[index] : std::string(); };
    const auto atObject = [&](const std::string& name, SoundRequest& request) {
        float where[3];
        if (!position || !position(name, where)) {
            ++unresolved_;
            return false;
        }
        request.positional = true;
        std::copy(where, where + 3, request.position.begin());
        return true;
    };
    SoundRequest request;
    switch (kind) {
    case SoundNativeKind::PlaySound:
        request.kind = SoundRequest::Kind::Play;
        request.id = std::uint32_t(toInt(valueAt(values, 0)));
        request.owner = std::uint16_t(toInt(valueAt(values, 1)));
        requests_.push_back(request);
        break;
    case SoundNativeKind::PlaySoundGO: {
        request.kind = SoundRequest::Kind::Play;
        request.id = std::uint32_t(toInt(valueAt(values, 0)));
        const float uid = valueAt(values, 2);
        request.owner = (uid < 0.0f) ? ownerOf(object(1)) : std::uint16_t(toInt(uid));
        if (atObject(object(1), request)) requests_.push_back(request);
        break;
    }
    case SoundNativeKind::PlaySoundAtObject:
        request.kind = SoundRequest::Kind::Play;
        request.id = std::uint32_t(toInt(valueAt(values, 1)));
        request.owner = ownerOf(object(0));
        if (atObject(object(0), request)) requests_.push_back(request);
        break;
    case SoundNativeKind::StopSound:
        request.kind = SoundRequest::Kind::Stop;
        request.id = std::uint32_t(toInt(valueAt(values, 0)));
        request.owner = toInt(valueAt(values, 1));
        request.value = valueAt(values, 2);
        requests_.push_back(request);
        break;
    case SoundNativeKind::StopSoundGO:
        request.kind = SoundRequest::Kind::Stop;
        request.id = std::uint32_t(toInt(valueAt(values, 0)));
        request.owner = ownerOf(object(1));
        request.value = valueAt(values, 2);
        requests_.push_back(request);
        break;
    case SoundNativeKind::RegisterSound: {
        const std::pair<std::uint16_t, std::uint8_t> entry{std::uint16_t(toInt(valueAt(values, 0))), std::uint8_t(toInt(valueAt(values, 1)) & 0xff)};
        if (std::find(registered_.begin(), registered_.end(), entry) == registered_.end()) registered_.push_back(entry);
        break;
    }
    case SoundNativeKind::ScaleSoundFrequency:
    case SoundNativeKind::SetSoundVolume:
        request.kind = kind == SoundNativeKind::SetSoundVolume ? SoundRequest::Kind::SetVolume : SoundRequest::Kind::SetFrequency;
        request.id = std::uint32_t(toInt(valueAt(values, 0)));
        request.owner = toInt(valueAt(values, 1));
        request.value = valueAt(values, 2);
        requests_.push_back(request);
        break;
    case SoundNativeKind::VehicleStopSounds:
        vehicleStops_.push_back(object(0));
        break;
    case SoundNativeKind::VehicleEnableDrivingMusic:
        drivingMusic_[object(0)] = valueAt(values, 1) != 0.0f;
        break;
    }
}

std::vector<SoundRequest> SoundNativeState::takeRequests() {
    std::vector<SoundRequest> taken;
    taken.swap(requests_);
    return taken;
}

}

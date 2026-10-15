#pragma once
// Script sound natives PlaySound 0x1b4394, PlaySoundGO 0x295244, PlaySoundAtObject 0x27bd48, StopSound 0x1b43ec,
// StopSoundGO 0x1d0d28, registersound 0x1cdbdc, ScaleSoundFrequency 0x2a8afc, SetSoundVolume 0x272dcc,
// vehiclestopsounds 0x27c148 and VehicleEnableDrivingMusic 0x2bf2b4 (batch SCRIPT-33, docs/re/script-natives-audio.md).
// The state records requests for the owner of the audio device. No SDL dependency.
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace chase::runtime {

enum class SoundNativeKind {
    PlaySound, PlaySoundGO, PlaySoundAtObject, StopSound, StopSoundGO,
    RegisterSound, ScaleSoundFrequency, SetSoundVolume, VehicleStopSounds, VehicleEnableDrivingMusic,
};

std::optional<SoundNativeKind> soundNativeKind(const std::string& lowerCaseName);
std::vector<std::string> soundNativeNames();
unsigned soundNativeArguments(SoundNativeKind kind);
int soundNativeObjectArgument(SoundNativeKind kind);

// AUDIO-S30: the owner of a sound played for an object is the u16 at entity+0x14 in the original, whose source is
// unread; objects get consecutive owners from this base (the doors use 1 and up).
constexpr std::uint16_t kObjectOwnerBase = 0x4000;

struct SoundRequest {
    enum class Kind {
        Play,          // 0x23fa54(id, position or null, owner)
        Stop,          // 0x23f75c(bank, id, owner, fade); a negative fade is the column 16 of the sound's row
        SetVolume,     // 0x219178 -> 0x227ff4: voice field +0x50 = value
        SetFrequency,  // 0x22f770: voice rate = (int)(value * source rate)
    };
    Kind kind = Kind::Play;
    std::uint32_t id = 0;
    std::int32_t owner = 0;
    bool positional = false;
    std::array<float, 3> position{};
    float value = 0.0f;
};

class SoundNativeState {
public:
    using PositionLookup = std::function<bool(const std::string&, float[3])>;

    void run(SoundNativeKind kind, const std::vector<float>& values, const std::vector<std::string>& objects, const PositionLookup& position);

    std::vector<SoundRequest> takeRequests();
    std::uint16_t ownerOf(const std::string& object);

    const std::vector<std::pair<std::uint16_t, std::uint8_t>>& registered() const { return registered_; }
    bool drivingMusicEnabled(const std::string& vehicle) const;
    const std::vector<std::string>& vehicleSoundStops() const { return vehicleStops_; }
    unsigned unresolvedObjects() const { return unresolved_; }

private:
    std::vector<SoundRequest> requests_;
    std::map<std::string, std::uint16_t> owners_;
    std::vector<std::pair<std::uint16_t, std::uint8_t>> registered_;
    std::map<std::string, bool> drivingMusic_;
    std::vector<std::string> vehicleStops_;
    unsigned unresolved_ = 0;
};

}

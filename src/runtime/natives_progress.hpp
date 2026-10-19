#pragma once
// Script natives that read or write the progress of the loaded slot, studs and a few global switches (batch SCRIPT-27,
// docs/re/script-natives.md). The slot is kept as the byte image of the original (live copy at 0x51b6b0), so the bit
// positions are those of the handlers. No SDL dependency.
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class ProgressNativeKind {
    EnableStudsByGroup,
    GiveReward,
    UnlockAbility,
    UnlockCharacter,
    LockCharacter,
    SetRaceInProgress,
    AddStuds,
    QuickSave,
    AwardRacingCar,
    CountRacesWon,
    IsRacingCarAwarded,
    SetRaceWon,
    UnlockLevel,
    DisableSuperBuiltItSave,
    SetParachuteDeployHeight,
    LockAbility,
    EnableSuperBricks,
    GetCharacterUnlocked,
    GetLanguage,
    GetPlayerNumber,
    IsRedbrickBought,
    UnlockScanner,
    EnableTrafficAndPedestrians,
};

// Argument shape: `values` leading arguments are read as numbers (object first for DisableSuperBuiltItSave), a getter
// has one more argument, the variable it writes.
struct ProgressNativeShape {
    unsigned values = 0;
    bool object = false;
    bool output = false;
};

std::optional<ProgressNativeKind> progressNativeKind(const std::string& lowerCaseName);
std::vector<std::string> progressNativeNames();
ProgressNativeShape progressNativeShape(ProgressNativeKind kind);
bool progressNativeIsStandIn(ProgressNativeKind kind);

constexpr std::size_t kSlotImageSize = 0x5230;
// Slot offsets (live address - 0x51b6b0) of the sets the natives use.
constexpr std::size_t kRedBrickSetOffset = 0x51b8;     // 0x1ffb5c, IsRedbrickBought
constexpr std::size_t kCharacterGroupOffset = 0x51bb;  // 0x263ab8, UnlockCharacter group bits
constexpr std::size_t kCharacterBitsOffset = 0x51bc;   // 0x52086c, 2 bits per id
constexpr std::size_t kCharacterNewSetOffset = 0x5211; // 0x232028: bit id, first time a bit is set for an id other than 9
constexpr std::size_t kLevelUnlockOffset = 0x51e0;     // 0x1b20b8
constexpr std::size_t kAbilityOffset = 0x51ee;         // 0x1cdeac, 0x1b24d4
constexpr std::size_t kRaceWonOffset = 0x5200;         // 0x1cd964 + 0x5200, 0x1a30f8 + 0x5200
constexpr std::size_t kRacingCarOffset = 0x520c;       // 0x1d9cb0 + 0x20c, 0x285f88 + 0x20c, bit 0
constexpr unsigned kRaceBits = 7;                      // CountRacesWon 0x1cd910 counts bits 0..6 of the first byte
constexpr unsigned kRacingCarId = 41;                  // AwardRacingCar 0x1d9c84: 0x205d5c(41, 0)

// Language names of GetLanguage 0x1b241c: option value 1..12 selects the level parameter "Language_<name>" (0x29e2ec).
const char* languageParameterName(unsigned language);

struct StudGroupRequest {
    std::uint8_t group = 0;
    std::uint8_t value = 0;  // stored at +2 of the sequencer entry 0x58f710 by 0x29693c; the levels pass 6 or 8
};
struct ObjectSuperBuiltItSave {
    std::string instance;
    bool disabled = false;
};

struct ProgressNativeResult {
    std::optional<float> output;
    std::optional<unsigned> unlockedLevel;
    std::optional<std::string> languageParameter;
};

class ProgressNativeState {
public:
    ProgressNativeState();

    void setSlotImage(const std::vector<std::uint8_t>& image);
    const std::vector<std::uint8_t>& slotImage() const { return slot_; }
    bool bit(std::size_t offset, std::uint32_t index) const;

    // STAND-IN: the language option (*0x376adc) is not wired; 1 is English.
    void setLanguage(std::uint8_t language) { language_ = language; }

    // `values` holds the numbers of the arguments (the raw integer words of the original are the truncated numbers).
    ProgressNativeResult run(ProgressNativeKind kind, const std::vector<float>& values, const std::string& object);

    std::vector<StudGroupRequest> takeStudGroupRequests();
    std::uint64_t takeStudsAdded();
    unsigned saveRequests() const { return saveRequests_; }
    // The save request 0x21cf38(0) made by the destroy of a super build brick (0x1d737c): the summary 0x17caf8 first copies the u32 at 0x593430 (the
    // super brick count) into slot +0x5190.
    void requestSave(std::uint32_t superBrickCount);
    const std::vector<std::uint32_t>& rewards() const { return rewards_; }
    const std::vector<std::uint32_t>& charactersUnlocked() const { return charactersUnlocked_; }
    // LockCharacter requests; the removal from the party (0x23b388) is not modelled (SCRIPT-S31).
    const std::vector<std::uint32_t>& charactersLocked() const { return charactersLocked_; }
    std::optional<bool> raceInProgress() const { return raceInProgress_; }
    std::optional<bool> superBricksEnabled() const { return superBricks_; }
    std::optional<float> parachuteDeployHeight() const { return parachuteHeight_; }
    bool scannerUnlocked() const { return scannerUnlocked_; }
    // EnableTrafficAndPedestrians 0x2c5780 writes bit 1 of the traffic manager (0x51b3e0) and of the pedestrian manager
    // (0x5277a0); their update spawns while it is set and removes every entity while it is clear. No traffic exists yet
    // (SCRIPT-230), so the flag is only recorded.
    std::optional<bool> trafficAndPedestriansEnabled() const { return trafficAndPedestrians_; }
    const std::vector<ObjectSuperBuiltItSave>& superBuiltItSaves() const { return superBuiltItSaves_; }
    unsigned outOfRangeAccesses() const { return outOfRange_; }

private:
    void setBit(std::size_t offset, std::uint32_t index, bool on);
    bool characterUnlocked(std::int32_t id) const;
    void unlockCharacter(std::uint32_t id);

    std::vector<std::uint8_t> slot_;
    std::uint8_t language_ = 1;
    unsigned saveRequests_ = 0;
    unsigned outOfRange_ = 0;
    std::uint64_t studsAdded_ = 0;
    std::vector<StudGroupRequest> studGroupRequests_;
    std::vector<std::uint32_t> rewards_;
    std::vector<std::uint32_t> charactersUnlocked_;
    std::vector<std::uint32_t> charactersLocked_;
    std::optional<bool> raceInProgress_;
    std::optional<bool> superBricks_;
    std::optional<float> parachuteHeight_;
    bool scannerUnlocked_ = false;
    std::optional<bool> trafficAndPedestrians_;
    std::vector<ObjectSuperBuiltItSave> superBuiltItSaves_;
};

}

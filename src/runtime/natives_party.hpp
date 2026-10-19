#pragma once
// Script natives of batch SCRIPT-35 (docs/re/script-natives-party.md): allegiance, weapons, vehicle entry, the super freeplay swap, the boss
// hooks of the Walnuts / Rex / 1-8 missions, mech piloting, skydive mode and the downtown intro switch. The handlers set fields of the
// character components and global bytes; the systems that read them (AI core SCRIPT-42, weapons, mechs, party) are not ported, so every
// handler records its effect (STAND-IN SCRIPT-S277 to SCRIPT-S280), except UsePet (an empty handler) and HACK_PlayDowntownIntro. No SDL dependency.
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

class ProgressNativeState;

enum class PartyNativeKind {
    SwapAllegiance,
    SetWeaponEnabled,
    CharSetAllowVehicleEntry,
    SuperFreeplaySwap,
    RelockChasePlainClothes,
    WalnutsHit,
    RejoinFight,
    GetInMech,
    RunFromArrest,
    CharSetSkydiveMode,
    FireBeamWeapon,
    GetOutMech,
    HackPlayDowntownIntro,
    RexAxeSmashDone,
    RexGrapplePullDone,
    UsePet,
    WalnutsStartPhase2,
};

// Leading `objects` arguments are object names, the next `numbers` arguments are numbers; `output` is a variable name written by the handler.
struct PartyNativeShape {
    unsigned objects = 0;
    unsigned numbers = 0;
    bool output = false;
    unsigned ignored = 0;  // trailing arguments the handler never reads
};

std::optional<PartyNativeKind> partyNativeKind(const std::string& lowerCaseName);
std::vector<std::string> partyNativeNames();
PartyNativeShape partyNativeShape(PartyNativeKind kind);
bool partyNativeIsStandIn(PartyNativeKind kind);

constexpr std::uint16_t kCharacterPlainClothes = 16;
constexpr std::uint16_t kCharacterChase = 9;
constexpr std::size_t kStoryProgressOffset = 0x30e4;  // level block of level 8 (0x203464) + 0x1c, u16
constexpr std::uint32_t kDowntownLevel = 8;

// SetWeaponEnabled 0x26fea4: the weapon value of a character (0 off, 2 hand A = 0x22ae28, any other non-zero value hand B = 0x22af80).
enum class WeaponHand { None, A, B };
WeaponHand weaponHandOf(float value);

struct WeaponRequest {
    std::string object;
    float value = 0;
    WeaponHand hand = WeaponHand::None;
};

// The three bytes 0x375f04 / 0x375f05 / 0x375f06 that SetWeaponEnabled sets for the player when the value is not 0 (names unknown).
struct PlayerWeaponBytes {
    bool byte4 = false;
    bool byte5 = false;
    bool byte6 = false;
};

// The record at 0x5163e8 + 0xc8 (entity) and + 0xcc (u16 character id) that SuperFreeplaySwap and RelockChasePlainClothes fill; the reader is not found.
struct FreeplaySwapRecord {
    std::string object;
    std::uint16_t character = 0;
};

// Effects 7 and 0xf1 that SuperFreeplaySwap spawns at the node of the player (0x225248).
constexpr std::array<std::uint16_t, 2> kFreeplaySwapEffects = {7, 241};

struct BeamShot {
    std::string object;
    std::uint8_t beam = 0;
    float first = 0;
    float second = 0;
};

// Requested state halfword at +4 of the boss component of an entity (entity + 0x7c).
struct BossRequest {
    std::string object;
    std::uint16_t state = 0;
};

// CharSetSkydiveMode: 0x2414b4(entity, ctrl + 0x50, state, 0) with state 129 (skydive) or 1, and the float at 0x376910 (kFloorNormalMin of
// collision.hpp in the port) set to 0.2016 or 0.7071. The port keeps its constant.
constexpr std::uint8_t kSkydiveState = 129;
constexpr std::uint8_t kNoSkydiveState = 1;
constexpr float kSkydiveFloorNormalMin = 0x1.9db22ep-3f;  // 0x3e4ed917
constexpr float kNormalFloorNormalMin = 0x1.6a0210p-1f;   // 0x3f350108

struct SkydiveRequest {
    std::string object;
    bool on = false;
    std::uint8_t state = 0;
};

struct PartyNativeResult {
    std::optional<float> output;
};

class PartyNativeState {
public:
    // Character id of the current party leader (byte 4 of the party manager 0x51b248); the port always plays Chase (9).
    void setCurrentCharacter(std::uint16_t id) { currentCharacter_ = id; }

    PartyNativeResult run(PartyNativeKind kind, const std::vector<std::string>& objects, const std::vector<float>& numbers, const ProgressNativeState& progress);

    // SwapAllegiance toggles bit 2 of the object's flag word (+0xc); the initial bit comes from the level data and is not read (SCRIPT-S277).
    std::uint32_t allegianceSwaps(const std::string& object) const;
    const std::vector<WeaponRequest>& weaponRequests() const { return weaponRequests_; }
    const std::optional<PlayerWeaponBytes>& playerWeaponBytes() const { return playerWeapon_; }
    // CharSetAllowVehicleEntry: bit 2 of ctrl + 0xe0 + 0x131 per object; unset until a script wrote it.
    std::optional<bool> vehicleEntryAllowed(const std::string& object) const;
    const std::optional<FreeplaySwapRecord>& freeplaySwap() const { return freeplaySwap_; }
    unsigned freeplaySwapEffects() const { return freeplaySwapEffects_; }
    // 0x23b388(16) of RelockChasePlainClothes: character 16 leaves the party.
    unsigned plainClothesRemovals() const { return plainClothesRemovals_; }
    const std::vector<BossRequest>& bossRequests() const { return bossRequests_; }
    // 1-8_runfromarrest: bits 0..2 of byte +0xda of the component of the entity's target set to 1.
    const std::vector<std::string>& runFromArrest() const { return runFromArrest_; }
    // getinmech: pilot stored at +0x348 of the mech's component; GetOutMech clears it.
    std::optional<std::string> pilotOf(const std::string& mech) const;
    const std::vector<SkydiveRequest>& skydiveRequests() const { return skydiveRequests_; }
    const std::optional<float>& floorNormalMin() const { return floorNormalMin_; }
    const std::vector<BeamShot>& beamShots() const { return beamShots_; }
    bool downtownIntroPlayed() const { return downtownIntroPlayed_; }

private:
    std::uint16_t currentCharacter_ = kCharacterChase;
    std::map<std::string, std::uint32_t> allegianceSwaps_;
    std::vector<WeaponRequest> weaponRequests_;
    std::optional<PlayerWeaponBytes> playerWeapon_;
    std::map<std::string, bool> vehicleEntry_;
    std::optional<FreeplaySwapRecord> freeplaySwap_;
    unsigned freeplaySwapEffects_ = 0;
    unsigned plainClothesRemovals_ = 0;
    std::vector<BossRequest> bossRequests_;
    std::vector<std::string> runFromArrest_;
    std::map<std::string, std::string> pilots_;
    std::vector<SkydiveRequest> skydiveRequests_;
    std::optional<float> floorNormalMin_;
    std::vector<BeamShot> beamShots_;
    bool downtownIntroPlayed_ = false;
};

}

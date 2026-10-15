#pragma once
// Vehicle objects (class descriptor of type 183 at 0x155b5c): the vehicle record the class creates at entity+0x7c, the start state its
// parameter reader 0x23ed0c writes, the wheel children the post-link 0x15624c finds by name, and the five flag natives of mission 1-1.
// No driving: the physics body and the input providers are not ported; entering and leaving are in vehicle_boarding.hpp. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

// Record states the parameter reader and the natives write (+2 current, +4 requested). Meaning of the others is not recovered.
constexpr std::uint16_t kVehicleStateParked = 4;
constexpr std::uint16_t kVehicleStateUnshovable = 10;
constexpr std::uint16_t kVehicleStateBlocksUnshovableA = 6;
constexpr std::uint16_t kVehicleStateBlocksUnshovableB = 7;

// Bits of record byte +0x311 and +0x312 as the parameter reader and the natives use them.
constexpr std::uint8_t kVehicleCameraOverride = 0x01, kVehicleLocked = 0x02, kVehicleInvulnerable = 0x04, kVehicleRemoteControl = 0x10,
                       kVehicleBigHitOnContact = 0x20, kVehicleJumpOutDenied = 0x40;
constexpr std::uint8_t kVehicleMusicEnabled = 0x04;

// Row of tables/tablescharacters.bxls sheet 4 (runtime array *0x372e14, stride 0x18); the row index is VehicleType.
struct VehicleTableRow {
    std::string name;            // +0
    std::int32_t price = 0;      // +4
    std::int32_t health = 0;     // +8: record health (+0x30e)
    std::uint32_t hash = 0;      // +0xc
    std::uint8_t column5 = 0;    // +0x10
    std::uint8_t column6 = 0;    // +0x11
    std::uint16_t horn = 0;      // +0x12: horn sound id, read by the player input 0x29cd9c
    std::uint8_t column8 = 0;    // +0x14
};
std::vector<VehicleTableRow> parseVehicleTable(const assets::Table& characters);

// A wheel child of the vehicle class file ("Wheel%02u", class vehicles/wheel): model and pose relative to the vehicle node.
struct VehicleWheel {
    std::string instance;
    assets::Placement local;
};

struct VehicleRecord {
    std::string instance;        // as written in the level, compared without case by the natives
    std::string className;
    std::string room;
    std::uint16_t state = 0;           // +2
    std::uint16_t requestedState = 0;  // +4
    bool statePending = false;         // +4 was written by the player side and +2 follows in the next update
    std::uint16_t vehicleType = 0;     // +0x2ec
    std::uint16_t musicTrack = 0;      // +0x2ee
    std::uint8_t flags311 = 0;
    std::uint8_t flags312 = 0;
    std::uint8_t forcedStop = 0;       // +0x310
    std::uint8_t health = 0;           // +0x30e
    std::uint8_t healthStart = 0;      // +0x30d
    std::array<float, 3> driverPosition = {0, 0, 0};  // +0x27c
    bool hasDriverPosition = false;
    std::array<float, 3> centreOfMass = {0, 0, 0};  // class value CentreOfMass: the origin of the physics body relative to the node
    float cameraPosY = 0, cameraPosZ = 0, cameraLookY = 0, cameraLookZ = 0, cameraPanScale = 1.0f;  // +0x2f0..+0x300
    std::uint8_t wheelCount = 0;       // +0x314 (class value numWheels)
    std::vector<VehicleWheel> wheels;  // +0x318: one per found "Wheel%02u" up to wheelCount

    bool locked() const { return (flags311 & kVehicleLocked) != 0; }
    bool jumpOutAllowed() const { return (flags311 & kVehicleJumpOutDenied) == 0; }
    bool musicEnabled() const { return (flags312 & kVehicleMusicEnabled) != 0; }
    bool invulnerable() const { return (flags311 & kVehicleInvulnerable) != 0; }
};

enum class VehicleNativeKind {
    SetLocked,               // VehicleSetLocked 0x26fe78
    SetForcedStop,           // VehicleSetForcedStop 0x29ea7c
    SetForcedStopOnMyVehicle,  // SetForcedStopOnMyVehicle 0x2b9df0
    AllowJumpOut,            // VehicleAllowJumpOut 0x27c014
    SetUnshovable,           // VehicleSetUnshovable 0x29eaa8
};

std::optional<VehicleNativeKind> vehicleNativeKind(const std::string& lowerCaseName);
std::vector<std::string> vehicleNativeNames();
bool vehicleNativeTakesValue(VehicleNativeKind kind);

class VehicleSet {
public:
    // Creates a record for every level object of the vehicle object type and runs the parameter reader 0x23ed0c on it.
    void load(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<VehicleTableRow>& table);
    const std::vector<VehicleRecord>& records() const { return records_; }
    const VehicleRecord* find(const std::string& lowerCaseInstance) const;

    // The first update promotes the requested state to the current one (stand-in VEH-S5: the original transition function was not found).
    void settle();

    // The player side writes +4 (5 while he drives, 3 after he jumps out); +2 takes it one update later (oracle, docs/re/vehicles.md, entry 2026-10-15T08:15:49).
    void requestState(std::size_t index, std::uint16_t state);
    void promotePending();

    // Instance name of the vehicle a character sits in (0x24255c); empty on foot. Without a lookup everybody is on foot.
    using OccupantLookup = std::function<std::string(const std::string&)>;
    void setOccupantLookup(OccupantLookup lookup) { occupant_ = std::move(lookup); }

    // The handlers read the vehicle record and write one flag. Returns false when the handler returns 0 (the script thread repeats the
    // call): only VehicleSetUnshovable does, while the vehicle state is 6 or 7. Unknown names and non-vehicles are ignored like a
    // script object of the wrong type would be by the original after the lookup.
    bool apply(VehicleNativeKind kind, const std::string& lowerCaseInstance, float value);

private:
    VehicleRecord* mutableFind(const std::string& lowerCaseInstance);

    std::vector<VehicleRecord> records_;
    OccupantLookup occupant_;
};

}

#include "runtime/vehicle.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::size_t kVehicleTableSheet = 4;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float floatOf(const assets::LevelParam* param, float fallback) {
    if (!param) return fallback;
    if (param->type == 0) {
        float value;
        std::memcpy(&value, &param->value, 4);
        return value;
    }
    if (param->type == 5 || param->type == 7) return float(std::int32_t(param->value));
    return fallback;
}

std::int32_t intOf(const assets::LevelParam* param) {
    if (!param) return 0;
    if (param->type == 5 || param->type == 7) return std::int32_t(param->value);
    if (param->type == 0) return std::int32_t(floatOf(param, 0));
    return 0;
}

struct ParamReader {
    const assets::LevelObject& object;
    const assets::Level& cls;

    const assets::LevelParam* get(const char* name) const { return effectiveParam(object, cls, assets::nameHash(name)); }
    bool flag(const char* name) const { return intOf(get(name)) != 0; }
};

void setBit(std::uint8_t& byte, std::uint8_t bit, bool on) { byte = on ? std::uint8_t(byte | bit) : std::uint8_t(byte & ~bit); }

void readParameters(const ParamReader& reader, const std::vector<VehicleTableRow>& table, VehicleRecord& record) {
    record.vehicleType = std::uint16_t(intOf(reader.get("VehicleType")));
    record.musicTrack = std::uint16_t(intOf(reader.get("MusicTrack")));
    setBit(record.flags312, kVehicleMusicEnabled, !reader.flag("NoMusic"));
    if (const assets::LevelParam* position = reader.get("DriverPosition"))
        if (position->type == 2 && !position->blocks.empty() && position->blocks[0].size() >= 12) {
            std::memcpy(record.driverPosition.data(), position->blocks[0].data(), 12);
            record.hasDriverPosition = true;
        }
    const std::int32_t tableHealth = record.vehicleType < table.size() ? table[record.vehicleType].health : 0;
    record.health = std::uint8_t(tableHealth);
    record.healthStart = record.health;
    const float override = floatOf(reader.get("HealthOverride"), -1.0f);
    if (override > 0.0f) record.health = record.healthStart = std::uint8_t(std::int32_t(override));
    setBit(record.flags311, kVehicleInvulnerable, reader.flag("Invulnerable"));
    setBit(record.flags311, kVehicleCameraOverride, reader.flag("ATTR_CameraOverride"));
    record.cameraPosY = floatOf(reader.get("ATTR_CameraPosY"), 0.0f);
    record.cameraPosZ = floatOf(reader.get("ATTR_CameraPosZ"), 0.0f);
    record.cameraLookY = floatOf(reader.get("ATTR_CameraLookY"), 0.0f);
    record.cameraLookZ = floatOf(reader.get("ATTR_CameraLookZ"), 0.0f);
    record.cameraPanScale = floatOf(reader.get("ATTR_PanScale"), 1.0f);
    setBit(record.flags311, kVehicleRemoteControl, reader.flag("RC_Vehicle"));
    setBit(record.flags311, kVehicleBigHitOnContact, reader.flag("BigHitOnContact"));
    if (reader.flag("PlayerDisable")) record.flags311 |= kVehicleLocked;
    if (reader.flag("SpawnUnshovable")) record.requestedState = kVehicleStateUnshovable;
    else if (reader.flag("SpawnParked")) record.requestedState = kVehicleStateParked;
}

void findWheels(const assets::Level& cls, const assets::ClassLookup& classOf, VehicleRecord& record) {
    record.wheelCount = std::uint8_t(floatOf(effectiveParam(assets::LevelObject{}, cls, assets::nameHash("numWheels")), 0.0f));
    const std::vector<assets::Placement> children = assets::objectPlacements(cls, classOf);
    for (unsigned number = 1; number <= record.wheelCount; ++number) {
        char name[16];
        std::snprintf(name, sizeof name, "Wheel%02u", number);
        for (const assets::Placement& child : children)
            if (lower(child.instance) == lower(name)) {
                record.wheels.push_back({child.instance, child});
                break;
            }
    }
}

struct NativeEntry {
    const char* name;
    VehicleNativeKind kind;
    bool takesValue;
};

constexpr NativeEntry kNatives[] = {
    {"vehiclesetlocked", VehicleNativeKind::SetLocked, true},
    {"vehiclesetforcedstop", VehicleNativeKind::SetForcedStop, true},
    {"setforcedstoponmyvehicle", VehicleNativeKind::SetForcedStopOnMyVehicle, true},
    {"vehicleallowjumpout", VehicleNativeKind::AllowJumpOut, true},
    {"vehiclesetunshovable", VehicleNativeKind::SetUnshovable, false},
};

}

std::vector<VehicleTableRow> parseVehicleTable(const assets::Table& characters) {
    std::vector<VehicleTableRow> rows;
    if (characters.sheets.size() <= kVehicleTableSheet) return rows;
    const assets::TableSheet& sheet = characters.sheets[kVehicleTableSheet];
    for (unsigned row = 0; row < sheet.rows; ++row) {
        VehicleTableRow entry;
        entry.name = assets::tableValue(sheet, row, 1).text;
        entry.price = std::int32_t(assets::tableValue(sheet, row, 2).bits);
        entry.health = std::int32_t(assets::tableValue(sheet, row, 3).bits);
        entry.hash = assets::tableValue(sheet, row, 4).bits;
        entry.column5 = std::uint8_t(assets::tableValue(sheet, row, 5).bits);
        entry.column6 = std::uint8_t(assets::tableValue(sheet, row, 6).bits);
        entry.horn = std::uint16_t(assets::tableValue(sheet, row, 7).bits);
        entry.column8 = std::uint8_t(assets::tableValue(sheet, row, 8).bits);
        rows.push_back(std::move(entry));
    }
    return rows;
}

std::optional<VehicleNativeKind> vehicleNativeKind(const std::string& lowerCaseName) {
    for (const NativeEntry& entry : kNatives)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> vehicleNativeNames() {
    std::vector<std::string> names;
    for (const NativeEntry& entry : kNatives) names.push_back(entry.name);
    return names;
}

bool vehicleNativeTakesValue(VehicleNativeKind kind) {
    for (const NativeEntry& entry : kNatives)
        if (entry.kind == kind) return entry.takesValue;
    return false;
}

void VehicleSet::load(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<VehicleTableRow>& table) {
    records_.clear();
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || !assets::isVehicleClassFile(*cls)) continue;
        VehicleRecord record;
        record.instance = object.instance;
        record.className = object.className;
        record.room = object.room;
        readParameters({object, *cls}, table, record);
        findWheels(*cls, classOf, record);
        records_.push_back(std::move(record));
    }
}

const VehicleRecord* VehicleSet::find(const std::string& lowerCaseInstance) const {
    for (const VehicleRecord& record : records_)
        if (lower(record.instance) == lowerCaseInstance) return &record;
    return nullptr;
}

VehicleRecord* VehicleSet::mutableFind(const std::string& lowerCaseInstance) {
    return const_cast<VehicleRecord*>(static_cast<const VehicleSet*>(this)->find(lowerCaseInstance));
}

void VehicleSet::settle() {
    for (VehicleRecord& record : records_) record.state = record.requestedState;
}

bool VehicleSet::apply(VehicleNativeKind kind, const std::string& lowerCaseInstance, float value) {
    const bool on = value != 0.0f;
    if (kind == VehicleNativeKind::SetForcedStopOnMyVehicle) {
        const std::string vehicle = occupant_ ? occupant_(lowerCaseInstance) : std::string();
        if (VehicleRecord* record = vehicle.empty() ? nullptr : mutableFind(lower(vehicle))) record->forcedStop = on ? 1 : 0;
        return true;
    }
    VehicleRecord* record = mutableFind(lowerCaseInstance);
    if (!record) return true;
    switch (kind) {
    case VehicleNativeKind::SetLocked: setBit(record->flags311, kVehicleLocked, on); break;
    case VehicleNativeKind::SetForcedStop: record->forcedStop = on ? 1 : 0; break;
    case VehicleNativeKind::AllowJumpOut: setBit(record->flags311, kVehicleJumpOutDenied, !on); break;
    case VehicleNativeKind::SetUnshovable:
        if (record->state == kVehicleStateBlocksUnshovableA || record->state == kVehicleStateBlocksUnshovableB) return false;
        record->requestedState = kVehicleStateUnshovable;
        break;
    case VehicleNativeKind::SetForcedStopOnMyVehicle: break;
    }
    return true;
}

}

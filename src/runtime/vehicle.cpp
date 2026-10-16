#include "runtime/vehicle.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/vehicle_drive.hpp"

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
    if (const assets::LevelParam* centre = reader.get("CentreOfMass"))
        if (centre->type == 2 && !centre->blocks.empty() && centre->blocks[0].size() >= 12) std::memcpy(record.centreOfMass.data(), centre->blocks[0].data(), 12);
    const std::int32_t tableHealth = record.vehicleType < table.size() ? table[record.vehicleType].health : 0;
    record.health = std::uint8_t(tableHealth);
    record.healthStart = record.health;
    const float override = floatOf(reader.get("HealthOverride"), -1.0f);
    if (override > 0.0f) record.health = record.healthStart = std::uint8_t(std::int32_t(override));
    setBit(record.flags311, kVehicleInvulnerable, reader.flag("Invulnerable"));
    setBit(record.flags311, kVehicleCameraOverride, reader.flag("ATTR_CameraOverride"));
    record.topSpeed = floatOf(reader.get("TopSpeed"), 0.0f);
    record.dampingLinear = floatOf(reader.get("DampingLinear"), 0.0f);
    record.dampingAngular = floatOf(reader.get("DampingAngular"), 0.0f);
    record.suspensionScale = floatOf(reader.get("WheelSuspensionScale"), 0.0f);
    record.mass = floatOf(reader.get("Mass"), 1400.0f);
    if (const assets::LevelParam* scale = reader.get("InertiaScale"))
        if (scale->type == 2 && !scale->blocks.empty() && scale->blocks[0].size() >= 12) {
            std::array<float, 3> value{};
            std::memcpy(value.data(), scale->blocks[0].data(), 12);
            record.inertiaScale = value;
        }
    if (const assets::LevelParam* spheres = reader.get("CollSpheres"))
        if (spheres->type == 2 && !spheres->blocks.empty() && spheres->blocks[0].size() >= 12) {
            std::array<float, 3> value{};
            std::memcpy(value.data(), spheres->blocks[0].data(), 12);
            record.collSpheres = value;
        }
    record.collRadius = floatOf(reader.get("CollRadius"), 0.49f);
    record.extraColSpheres = (std::uint32_t(floatOf(reader.get("ExtraColSpheres1"), 0.0f)) & 1u) != 0;
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
                record.wheels.push_back({child.instance, child, {}, {}});
                break;
            }
    }
}

void readSuspension(const ParamReader& reader, VehicleRecord& record) {
    const auto value = [&](const char* side, const char* name, float fallback) {
        char key[64];
        std::snprintf(key, sizeof key, "%s%s", side, name);
        return floatOf(reader.get(key), fallback);
    };
    const float suspensionRatio = floatOf(reader.get("WheelSuspensionRatio"), 0.0f);
    const float driveRatio = floatOf(reader.get("WheelDriveRatio"), 0.0f);
    for (std::size_t index = 0; index < record.wheels.size(); ++index) {
        const bool rear = (index & 2) != 0;
        const char* side = rear ? "Rear" : "Front";
        const float ratio = rear ? 2.0f - suspensionRatio * 2.0f : suspensionRatio * 2.0f;
        WheelSuspension& suspension = record.wheels[index].suspension;
        const assets::Placement& local = record.wheels[index].local;
        suspension.mount = {local.position[0] - record.centreOfMass[0], value(side, "ShockYoffset", 0.0f) - record.centreOfMass[1],
                            local.position[2] - record.centreOfMass[2]};
        const float width = value(side, "WheelWidth", 0.0f) * 0.25f;
        suspension.sideOffset = local.position[0] < 0.0f ? -width : width;
        suspension.radius = value(side, "WheelRadius", 0.0f);
        suspension.travel = value(side, "ShockTravelMax", 0.0f);
        suspension.stiffness = value(side, "ShockStiffness", 0.0f) * ratio * kVehicleBodyMassFactor;
        suspension.dampingExtend = value(side, "ShockDampingPos", 0.0f) * ratio * kVehicleBodyMassFactor;
        suspension.dampingCompress = value(side, "ShockDampingNeg", 0.0f) * ratio * kVehicleBodyMassFactor;
        WheelGripSpec& grip = record.wheels[index].grip;
        grip.driveRatio = rear ? 2.0f - driveRatio * 2.0f : driveRatio * 2.0f;
        grip.steered = !rear;
        grip.frictionStatic = value(side, "WheelFrictionStatic", 0.0f) * 2.0f;
        grip.frictionDynamic = value(side, "WheelFrictionDynamic", 0.0f) * 2.0f;
        grip.inertia = value(side, "WheelInertia", 0.0f);
        grip.driftScale = value(side, "DriftScale", 1.0f);
        grip.handbrakeScale = value(side, "HandbrakeScale", 0.0f);
        grip.brakeForce = value(side, "BrakeForce", 0.0f);
        grip.brakeFriction = value(side, "BrakeFriction", 0.0f);
    }
}

void readEngine(const ParamReader& reader, VehicleRecord& record) {
    const auto value = [&](const char* name, float fallback) { return floatOf(reader.get(name), fallback); };
    SteeringSpec steering;
    steering.speed = value("SteeringSpeed", 2.0f);
    steering.speedNeg = value("SteeringSpeedNeg", 0.4f);
    steering.speedCentre = value("SteeringSpeedCentre", 0.5f);
    steering.lockMax = value("SteeringLockMax", 0.9f);
    steering.lockMin = value("SteeringLockMin", 0.4f);
    steering.lockPower = value("SteeringLockPower", 3.0f);
    const float wheelRadius = record.wheels.empty() ? 0.0f : record.wheels[0].suspension.radius;
    record.engine = makeEngineSpec(value("Friction", 0.0f), value("IdleSpeed", 0.0f), value("Inertia", 0.0f), value("MaxRPM", 0.0f),
                                   value("Redline", 0.0f), value("dtPower", 0.0f), record.topSpeed, wheelRadius,
                                   kVehicleBodyMassFactor * record.mass, value("Slowdown", 0.2f), steering);
}

struct NativeEntry {
    const char* name;
    VehicleNativeKind kind;
    VehicleNativeArgs args;
};

constexpr NativeEntry kNatives[] = {
    {"vehiclesetlocked", VehicleNativeKind::SetLocked, VehicleNativeArgs::NameValue},
    {"vehiclesetforcedstop", VehicleNativeKind::SetForcedStop, VehicleNativeArgs::NameValue},
    {"setforcedstoponmyvehicle", VehicleNativeKind::SetForcedStopOnMyVehicle, VehicleNativeArgs::NameValue},
    {"vehicleallowjumpout", VehicleNativeKind::AllowJumpOut, VehicleNativeArgs::NameValue},
    {"vehiclesetunshovable", VehicleNativeKind::SetUnshovable, VehicleNativeArgs::Name},
    {"vehiclejumpout", VehicleNativeKind::JumpOut, VehicleNativeArgs::Name},
    {"vehiclejumpin", VehicleNativeKind::JumpIn, VehicleNativeArgs::NameName},
    {"stopplayervehicle", VehicleNativeKind::StopPlayerVehicle, VehicleNativeArgs::Value},
    {"vehiclesettopspeed", VehicleNativeKind::SetTopSpeed, VehicleNativeArgs::NameValue},
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

VehicleNativeArgs vehicleNativeArgs(VehicleNativeKind kind) {
    for (const NativeEntry& entry : kNatives)
        if (entry.kind == kind) return entry.args;
    return VehicleNativeArgs::Name;
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
        readSuspension({object, *cls}, record);
        readEngine({object, *cls}, record);
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

void VehicleSet::requestState(std::size_t index, std::uint16_t state) {
    if (index >= records_.size()) return;
    records_[index].requestedState = state;
    records_[index].statePending = true;
}

void VehicleSet::promotePending() {
    for (VehicleRecord& record : records_)
        if (record.statePending) {
            record.state = record.requestedState;
            record.statePending = false;
        }
}

void VehicleSet::playerEntered(std::size_t index) {
    if (index >= records_.size()) return;
    if (const std::optional<std::uint16_t> state = requestedStateOnPlayerEntered(records_[index].state)) requestState(index, *state);
}

void VehicleSet::playerExited(std::size_t index) {
    if (index >= records_.size()) return;
    if (const std::optional<std::uint16_t> state = requestedStateOnPlayerExited(records_[index].state)) requestState(index, *state);
}

void VehicleSet::limitSpeed(const std::string& lowerCaseInstance, float maxSpeed) {
    if (VehicleRecord* record = mutableFind(lowerCaseInstance)) record->pendingSpeedLimit = maxSpeed;
}

bool VehicleSet::apply(VehicleNativeKind kind, const std::string& lowerCaseInstance, float value, const std::string& lowerCaseOther) {
    const bool on = value != 0.0f;
    const std::string seated = occupant_ ? lower(occupant_("player1")) : std::string();
    if (kind == VehicleNativeKind::JumpOut) {
        if (lowerCaseInstance == "player1" && !seated.empty() && boarding_) boarding_(BoardingRequestKind::JumpOut, seated);
        return true;
    }
    if (kind == VehicleNativeKind::JumpIn) {
        if (lowerCaseInstance == "player1" && mutableFind(lowerCaseOther) && boarding_) boarding_(BoardingRequestKind::JumpIn, lowerCaseOther);
        return true;
    }
    if (kind == VehicleNativeKind::StopPlayerVehicle) {
        if (VehicleRecord* record = seated.empty() ? nullptr : mutableFind(seated)) {
            record->pendingSpeedLimit = 0.0f;
            record->forcedStop = on ? 1 : 0;
        }
        return true;
    }
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
    case VehicleNativeKind::SetTopSpeed: record->topSpeed = value; break;
    case VehicleNativeKind::SetForcedStopOnMyVehicle:
    case VehicleNativeKind::JumpOut:
    case VehicleNativeKind::JumpIn:
    case VehicleNativeKind::StopPlayerVehicle: break;
    }
    return true;
}

VehicleRecord vehicleRecordAtMassScale(const VehicleRecord& record, float massScale) {
    VehicleRecord scaled = record;
    for (VehicleWheel& wheel : scaled.wheels) {
        wheel.suspension.stiffness = wheel.suspension.stiffness / kVehicleBodyMassFactor * massScale;
        wheel.suspension.dampingExtend = wheel.suspension.dampingExtend / kVehicleBodyMassFactor * massScale;
        wheel.suspension.dampingCompress = wheel.suspension.dampingCompress / kVehicleBodyMassFactor * massScale;
    }
    scaled.engine.mass = record.engine.mass / kVehicleBodyMassFactor * massScale;
    return scaled;
}

}

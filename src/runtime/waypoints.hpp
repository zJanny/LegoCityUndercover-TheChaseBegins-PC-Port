#pragma once
// The waypoint class of the level objects props/map_waypoint, map_vehiclewaypoint and map_getintovehiclewaypoint
// (docs/re/scripts-missions.md, SCRIPT-110): init 0x17aa34, setup 0x170fd8, update 0x17acbc. An active waypoint tests the
// player against the class bound "TriggerBound" every update; a hit starts a fade (states 2 and 3 at component +0x78) and,
// when the fade-out timer ends, a thread of the level script named by the instance parameter TriggerScript (0x22ce9c at
// 0x17b03c). The waypoint then switches itself off. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/node_tween.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class WaypointKind : std::uint8_t { OnFoot = 0, Vehicle = 1 };

struct WaypointBound {
    float centre[3] = {0, 0, 0};
    float half[3] = {0, 0, 0};
    std::uint8_t shape = 0;
    bool active = false;
};

struct Waypoint {
    std::string instance, room, triggerScript;
    float position[3] = {0, 0, 0};
    float yawDegrees = 0;
    WaypointKind kind = WaypointKind::OnFoot;
    bool autoTrigger = true;
    bool takePlayerControl = true;
    float fadeScaleIn = 0, fadeScaleOut = 0;
    bool hasBound = false;
    WaypointBound bound;
    std::string namedVehicle;
    std::int8_t vehicleType = 0;
    int waypointType = 0;
    bool getIntoVehicle = false;
    std::string firedVehicle;

    bool active = false;
    bool entityEnabled = false;
    int mapSlot = -1;
    bool fired = false;
    std::uint8_t state = 0;
    float timer = 0;

    float badgeFadeIn = 0;  // BadgeFadeInTime (+0x68)
    NodeTween columnTween = NodeTween::start(0.0f, 0.0f, 0.0f);  // alpha of the column node: 0 to 1 in state 0, 1 to 0 in state 2
    NodeTween badgeTween = NodeTween::start(0.0f, 0.0f, 0.0f);   // alpha of the Badge child: 0 until state 1 ends, then 0 to 1
    float columnFade() const { return columnTween.value(); }
    float badgeFade() const { return badgeTween.value(); }
    // 0x229c98 with enable 0 outside state 3: the column fades out over FadeScaleInTime while the waypoint is already off.
    bool disableFade = false;
    bool columnShown() const { return entityEnabled && (active || (disableFade && columnTween.value() > 0.0f)); }
};

constexpr float kWaypointVehicleSpeedLimit = 12.0f;
constexpr std::uint32_t kWaypointVehicleHintKey = 0xa0fc8748;
constexpr float kWaypointVehicleHintSeconds = 0.5f;
constexpr float kWaypointEnableTimer = 0x1p-23f;
constexpr float kWaypointHitTimer = 0x1.0624dep-10f;
constexpr int kWaypointMapSlots = 16;      // 0x1a2114: the first free of 16 map slots
constexpr int kWaypointTypeGetInto = 11;   // 0x229d3c: a vehicle waypoint of WaypointType 11 does not become the vehicle waypoint
constexpr float kWaypointBadgeHideSeconds = 0x1p-23f;  // 0x229e48, the duration of the badge tween 1 to 0 of a disable

struct WaypointVehicle {
    std::string instance;
    std::int8_t type = 0;
    assets::Matrix matrix{};
    float boxCentre[3] = {0, 0, 0};
    float boxHalf[3] = {0, 0, 0};
};

struct WaypointVehicleAction {
    std::string vehicle;
    bool stop = false;
};

struct WaypointUpdate {
    std::vector<std::string> startScripts;
    std::vector<std::string> deactivated;
    std::vector<WaypointVehicleAction> vehicleActions;
    bool vehicleHint = false;
};

class WaypointSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    bool known(const std::string& lowerName) const;
    bool active(const std::string& lowerName) const;
    // 0x26e168 / 0x229c98: enabling clears the entity's disabled flag (0x242cf0), resets state and timer, takes the first free of the
    // 16 map slots when it holds none (-1 when all are used) and, for a vehicle waypoint with a named vehicle, WaypointType other than 11 and
    // GetIntoVehicle clear, becomes the vehicle waypoint *(0x529c18 + 0x28). A disable is ignored unless AutoTrigger is 0 (bit 4 of
    // +0x8e). An accepted disable clears the vehicle waypoint, frees the map slot and, outside state 3, starts the column tween 1 to 0
    // over FadeScaleInTime (+0x6c) and the badge tween 1 to 0 over 2^-23 s, whether or not the waypoint was active; the entity disable
    // 0x2428c0 happens only when *(0x529c18 + 0x24) (0x230ef4) is this waypoint, which the port never models (SCRIPT-S269: the global's
    // writer was not found; its readers use it as a character). Returns whether the waypoint is active afterwards.
    bool setActive(const std::string& lowerName, bool enable);
    // 0x17acbc for every active waypoint of a loaded room, in object order; the disable fades of switched-off waypoints advance too. The player position is the player node.
    // `vehicle` is the vehicle of 0x22ec10 / 0x24255c (jump in, seated and jump out count), null on foot.
    WaypointUpdate update(float dt, const float player[3], const WaypointVehicle* vehicle, const std::function<bool(const std::string&)>& roomLoaded);
    // Bit 3 of +0x8e (TakePlayerControl) sets bit 4 of the player's byte +0xd8 in state 2 and clears it when the script starts.
    bool playerControlTaken() const { return controlTaken_; }
    const std::vector<Waypoint>& waypoints() const { return waypoints_; }
    const Waypoint* find(const std::string& lowerName) const;
    // *(0x529c18 + 0x28) (0x2b4dac): the instance of the last enabled vehicle waypoint, empty after its disable or none. No reader found.
    const std::string& vehicleWaypoint() const { return vehicleWaypoint_; }

    static bool inBound(const Waypoint& waypoint, const float player[3]);
    // 0x17b21c: the closest point of the vehicle's box to the bound centre, seen from the waypoint, tested against the bound.
    static bool vehicleInBound(const Waypoint& waypoint, const WaypointVehicle& vehicle);
    static bool vehicleMatches(const Waypoint& waypoint, const WaypointVehicle* vehicle);

private:
    std::vector<Waypoint> waypoints_;
    int takeMapSlot();
    void freeMapSlot(Waypoint& waypoint);

    bool controlTaken_ = false;
    std::array<bool, kWaypointMapSlots> slotUsed_{};
    std::string vehicleWaypoint_;
};

}

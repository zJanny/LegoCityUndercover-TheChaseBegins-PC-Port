#pragma once
// WaitForPlatform 0x263b84 (docs/re/props-rocking.md): the script waits until the platform record of the object (ObjectType 64, record at
// entity+0x7c) reports its travel value R+0x7c at 1.0 or above, or its state R+2 at 5. The records move their nodes with the travel of
// platform_node_move.hpp and carry the objects attached below them. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/platform_node_move.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kPlatformKind = 64;

// 0x263b84: the bit pattern of the value equal to 1.0, or not below it as a signed integer (a float at or above 1.0), or the state 5.
bool platformReached(const PlatformRecord& record);

// A travel that ended in the last update (the state left 2, 3 or 4), or a lap of a `Looped` path platform (its value went round past 1.0 or 0.0).
struct PlatformArrival {
    std::string instance;
    PlatformVector node{};
    std::uint16_t state = 0;
    std::uint32_t movedUpdates = 0;
};

class PlatformSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    PlatformRecord* find(const std::string& lowerInstance);
    const PlatformRecord* find(const std::string& lowerInstance) const;
    // Messages 0xff and 0xfe of the handler 0x1a8d74 (platform_node_move.hpp).
    void message(const std::string& lowerInstance, std::uint8_t message);
    // The entity updates after the script pass, in the order of the level's objects (0x2807c8): travel, node move and the carried nodes.
    void update(float dt);
    // Message 8 (ResetObject, platform_riding.hpp): the record's reset, and its node and the nodes below it go back to the origin.
    void reset(const std::string& lowerInstance);
    // The translation of a platform's node: its placement plus what moved it; nothing for an object that is no platform.
    std::optional<PlatformVector> nodeTranslation(const std::string& lowerInstance) const;
    // The world rotation rows (the placement's rows times the carried turn) and translation of a platform's node; nothing for an object that is no platform.
    std::optional<std::pair<PathRotation, PlatformVector>> nodeFrame(const std::string& lowerInstance) const;

    // AttachObject 0x1bf68c: the child's node goes under the parent's (0x245008), and a platform parent gets message 43 (its list R+0x60).
    void attachObject(const std::string& lowerParent, const std::string& lowerChild);
    // AttachToBone / AttachToBoneRot keep the parent node; AttachToWorldRoot / AttachToRoom take the child out of the hierarchy.
    void setNodeParent(const std::string& lowerChild, const std::string& lowerParent) { carry_.setParent(lowerChild, lowerParent); }
    // How far the moving platform nodes above an object (or the object's own node, for a platform) have moved it from its placement.
    PlatformVector carriedOffset(const std::string& lowerInstance) const { return carry_.offset(lowerInstance); }
    // The turn of the moving platform nodes above an object (or of a platform's own node): the rows R of its carry p -> p R + T.
    PathRotation carriedRotation(const std::string& lowerInstance) const { return carry_.rotation(lowerInstance); }
    // The list R+0x60 of a platform: the objects AttachObject attached to it, in order (empty for other objects).
    const std::vector<std::string>& attachedTo(const std::string& lowerPlatform) const { return carry_.attached(lowerPlatform); }
    const std::string* nodeParent(const std::string& lowerChild) const { return carry_.parent(lowerChild); }
    // The objects whose carried offset changed since the last call, and the travels that ended.
    std::vector<std::string> takeMoved() { return carry_.takeMoved(); }
    std::vector<PlatformArrival> takeArrivals();
    std::vector<PlatformArrival> takeLaps();

private:
    // A node move that also turns the node: the carry that takes the platform from its placement to `node` with the rows the update wrote.
    void turnNode(const std::string& lowerInstance, const PlatformRecord& record, const PlatformVector& node);
    PlatformArrival report(const std::string& lowerInstance, const PlatformRecord& record);
    std::vector<std::string> order_;
    std::map<std::string, PlatformRecord> records_;
    std::map<std::string, std::uint32_t> movedUpdates_;
    std::vector<PlatformArrival> arrivals_;
    std::vector<PlatformArrival> laps_;
    NodeCarry carry_;
};

}

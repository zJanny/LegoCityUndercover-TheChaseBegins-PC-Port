#pragma once
// Level switches between world levels (docs/re/streaming.md): the `scenechange` objects of the master level
// lvl_the_city, their destination points and the request order of the manager at 0x5937e4 (slot +0x2c 0x3037e8 stores
// the request and opens the loading page, mode 6; the execution 0x303480 follows the A press). No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kMasterLevelFile = "levels/levels/lvl_the_city.blvl";

// Updates between the A press on the loading page and the execution 0x303480 (trap run of the oracle, state
// station-exit-loadingpage: the manager's execution follows 4 updates after the press).
constexpr unsigned kSceneChangeSwitchDelayUpdates = 4;

struct WorldLevelEntry {
    std::string instance;   // "WorldLevel_01_CherryTreeHills"
    std::string levelFile;  // "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl"
};

// Where a request leads: the world level and the `point` object named by the SceneChangeTo parameter (hash d2fb9421,
// data +0x14 of the object; a 20-byte reference {world level hash, object hash, ...}).
struct SceneChangeDestination {
    WorldLevelEntry worldLevel;
    std::uint32_t pointHash = 0;
};

// One `scenechange` object of the master level. SceneChangeBound (hash f5b7ba6c) is "<WorldLevel instance>.<bound>".
struct SceneChangeObject {
    std::string instance;
    std::string boundWorldLevel;
    std::string boundName;
    std::optional<SceneChangeDestination> destination;
};

std::vector<WorldLevelEntry> worldLevels(const assets::Level& master);
std::vector<SceneChangeObject> sceneChangeObjects(const assets::Level& master);
// The world level whose level file is `levelFile`, empty instance when the master level lists none.
WorldLevelEntry worldLevelOfFile(const assets::Level& master, const std::string& levelFile);
// Lookup of `SceneChange <bound>` (0x26344c): the object whose bound is `<current world level>.<bound>`. STAND-IN
// TRANS-S2: how the original qualifies the argument was not read; the data fit is that every shipped call matches the
// calling level's own instance name.
const SceneChangeObject* findSceneChange(const std::vector<SceneChangeObject>& objects, const std::string& currentWorldLevel,
                                         const std::string& bound);
// The placement of the destination point inside the destination level (position and yaw degrees of the object).
std::optional<assets::Placement> destinationPoint(const assets::Level& destinationLevel, const SceneChangeDestination& destination);

// Request order of the manager: request (slot +0x2c: stores the destination and opens the loading page), A on the page,
// the execution after kSceneChangeSwitchDelayUpdates. A second request while one is pending is ignored (the manager's
// flag byte [0x37758c]+0x30 is cleared by the first request, 0x3037e8).
class SceneChangeFlow {
public:
    bool request(const SceneChangeDestination& destination);
    bool pending() const { return destination_.has_value(); }
    bool pageOpen() const { return destination_.has_value() && !confirmed_; }
    // One update while the request is pending; `confirmPress` is the A press of this update.
    void update(bool confirmPress);
    // The destination once the execution is due; clears the request.
    std::optional<SceneChangeDestination> takeSwitch();

private:
    std::optional<SceneChangeDestination> destination_;
    bool confirmed_ = false;
    unsigned waited_ = 0;
};

}

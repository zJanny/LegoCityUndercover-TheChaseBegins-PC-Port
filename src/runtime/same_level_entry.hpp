#pragma once
#include "runtime/scene_change.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

// STAND-IN TRANS-S6: the update of the execution 0x303480 after slot +0x28 and the fade of the pushed mode node
// (durations at manager +0x3c/+0x44, byte +0x64) were not measured; the move is applied in the next update and the
// level is not held while it waits.
constexpr unsigned kSameLevelEntryDelayUpdates = 1;

struct SameLevelEntry {
    std::string instance;
    std::string boundName;
    std::uint32_t pointHash = 0;
};

std::vector<SameLevelEntry> levelSceneChangeEntries(const assets::Level& level);
std::vector<SameLevelEntry> masterSameLevelEntries(const std::vector<SceneChangeObject>& objects, const std::string& currentWorldLevel);
std::vector<SameLevelEntry> sameLevelEntries(const assets::Level& level, const std::vector<SceneChangeObject>& masterObjects,
                                             const std::string& currentWorldLevel);
bool destinationInLoadedLevel(const SceneChangeDestination& destination, const std::string& currentWorldLevel);
const SameLevelEntry* findSameLevelEntry(const std::vector<SameLevelEntry>& entries, const std::string& bound);

class SameLevelEntryFlow {
public:
    bool request(const SameLevelEntry& entry);
    bool pending() const { return entry_.has_value(); }
    void update();
    std::optional<SameLevelEntry> takeEntry();

private:
    std::optional<SameLevelEntry> entry_;
    unsigned waited_ = 0;
};

}

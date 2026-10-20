#include "runtime/same_level_entry.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {
constexpr std::uint32_t kSceneChangeBoundHash = 0xf5b7ba6c;
constexpr std::uint32_t kSceneChangeToHash = 0xd2fb9421;
constexpr const char* kSceneChangeClass = "scenechange";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* param(const assets::LevelObject& object, std::uint32_t hash) {
    for (const assets::LevelParam& candidate : object.params)
        if (candidate.hash == hash) return &candidate;
    return nullptr;
}
}

std::vector<SameLevelEntry> levelSceneChangeEntries(const assets::Level& level) {
    std::vector<SameLevelEntry> entries;
    for (const assets::LevelObject& object : level.objects) {
        if (object.className != kSceneChangeClass) continue;
        const assets::LevelParam* bound = param(object, kSceneChangeBoundHash);
        const assets::LevelParam* to = param(object, kSceneChangeToHash);
        if (!bound || bound->type != 1 || bound->blocks.empty() || !to || to->type != 4 || to->blocks.size() != 1 || to->blocks[0].size() < 8) continue;
        std::uint32_t worldLevelHash, pointHash;
        std::memcpy(&worldLevelHash, to->blocks[0].data(), 4);
        std::memcpy(&pointHash, to->blocks[0].data() + 4, 4);
        if (worldLevelHash != 0) continue;
        const std::string text(reinterpret_cast<const char*>(bound->blocks[0].data()), bound->blocks[0].size());
        entries.push_back({object.instance, text.substr(0, text.find('\0')), pointHash});
    }
    return entries;
}

bool destinationInLoadedLevel(const SceneChangeDestination& destination, const std::string& currentWorldLevel) {
    return !currentWorldLevel.empty() && lower(destination.worldLevel.instance) == lower(currentWorldLevel);
}

std::vector<SameLevelEntry> masterSameLevelEntries(const std::vector<SceneChangeObject>& objects, const std::string& currentWorldLevel) {
    std::vector<SameLevelEntry> entries;
    for (const SceneChangeObject& object : objects)
        if (object.destination && !object.boundName.empty() && lower(object.boundWorldLevel) == lower(currentWorldLevel) &&
            destinationInLoadedLevel(*object.destination, currentWorldLevel))
            entries.push_back({object.instance, object.boundName, object.destination->pointHash});
    return entries;
}

std::vector<SameLevelEntry> sameLevelEntries(const assets::Level& level, const std::vector<SceneChangeObject>& masterObjects,
                                             const std::string& currentWorldLevel) {
    std::vector<SameLevelEntry> entries = levelSceneChangeEntries(level);
    const std::vector<SameLevelEntry> fromMaster = masterSameLevelEntries(masterObjects, currentWorldLevel);
    entries.insert(entries.end(), fromMaster.begin(), fromMaster.end());
    return entries;
}

const SameLevelEntry* findSameLevelEntry(const std::vector<SameLevelEntry>& entries, const std::string& bound) {
    for (const SameLevelEntry& entry : entries)
        if (lower(entry.boundName) == lower(bound)) return &entry;
    return nullptr;
}

bool SameLevelEntryFlow::request(const SameLevelEntry& entry) {
    if (entry_) return false;
    entry_ = entry;
    waited_ = 0;
    return true;
}

void SameLevelEntryFlow::update() {
    if (entry_) ++waited_;
}

std::optional<SameLevelEntry> SameLevelEntryFlow::takeEntry() {
    if (!entry_ || waited_ < kSameLevelEntryDelayUpdates) return std::nullopt;
    std::optional<SameLevelEntry> due = entry_;
    entry_.reset();
    return due;
}

}

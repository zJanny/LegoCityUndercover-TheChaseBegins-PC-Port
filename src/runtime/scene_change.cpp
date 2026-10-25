#include "runtime/scene_change.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {
constexpr std::uint32_t kSceneChangeBoundHash = 0xf5b7ba6c;
constexpr std::uint32_t kSceneChangeToHash = 0xd2fb9421;
constexpr std::uint32_t kSceneChangeFastTravelHash = 0xd25a2180;
constexpr const char* kSceneChangeClass = "scenechange";
constexpr const char* kWorldLevelPrefix = "WorldLevel_";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* param(const assets::LevelObject& object, std::uint32_t hash) {
    for (const assets::LevelParam& candidate : object.params)
        if (candidate.hash == hash) return &candidate;
    return nullptr;
}

std::string levelFileOfClass(const std::string& className) { return "levels/" + className + ".blvl"; }
}

std::vector<WorldLevelEntry> worldLevels(const assets::Level& master) {
    std::vector<WorldLevelEntry> entries;
    for (const assets::LevelObject& object : master.objects)
        if (object.instance.rfind(kWorldLevelPrefix, 0) == 0 && object.className.rfind("levels/", 0) == 0)
            entries.push_back({object.instance, levelFileOfClass(object.className)});
    return entries;
}

WorldLevelEntry worldLevelOfFile(const assets::Level& master, const std::string& levelFile) {
    for (const WorldLevelEntry& entry : worldLevels(master))
        if (lower(entry.levelFile) == lower(levelFile)) return entry;
    return {};
}

std::vector<SceneChangeObject> sceneChangeObjects(const assets::Level& master) {
    const std::vector<WorldLevelEntry> levels = worldLevels(master);
    std::vector<SceneChangeObject> objects;
    for (const assets::LevelObject& object : master.objects) {
        if (object.className != kSceneChangeClass) continue;
        SceneChangeObject entry;
        entry.instance = object.instance;
        if (const assets::LevelParam* bound = param(object, kSceneChangeBoundHash); bound && bound->type == 1 && !bound->blocks.empty()) {
            const std::string text(reinterpret_cast<const char*>(bound->blocks[0].data()), bound->blocks[0].size());
            const std::string qualified = text.substr(0, text.find('\0'));
            const std::size_t dot = qualified.find('.');
            if (dot != std::string::npos) {
                entry.boundWorldLevel = qualified.substr(0, dot);
                entry.boundName = qualified.substr(dot + 1);
            }
        }
        if (const assets::LevelParam* to = param(object, kSceneChangeToHash); to && to->type == 4 && to->blocks.size() == 1 && to->blocks[0].size() >= 8) {
            std::uint32_t worldLevelHash, pointHash;
            std::memcpy(&worldLevelHash, to->blocks[0].data(), 4);
            std::memcpy(&pointHash, to->blocks[0].data() + 4, 4);
            for (const WorldLevelEntry& level : levels)
                if (assets::nameHash(level.instance) == worldLevelHash) entry.destination = SceneChangeDestination{level, pointHash, 0};
        }
        if (const assets::LevelParam* travel = param(object, kSceneChangeFastTravelHash);
            travel && entry.destination && travel->type == 4 && travel->blocks.size() == 1 && travel->blocks[0].size() >= 8)
            std::memcpy(&entry.destination->fastTravelHolderHash, travel->blocks[0].data() + 4, 4);
        objects.push_back(entry);
    }
    return objects;
}

const SceneChangeObject* findSceneChange(const std::vector<SceneChangeObject>& objects, const std::string& currentWorldLevel,
                                         const std::string& bound) {
    for (const SceneChangeObject& object : objects)
        if (lower(object.boundWorldLevel) == lower(currentWorldLevel) && lower(object.boundName) == lower(bound)) return &object;
    return nullptr;
}

std::optional<assets::Placement> destinationPoint(const assets::Level& destinationLevel, const SceneChangeDestination& destination) {
    for (const assets::LevelObject& object : destinationLevel.objects)
        if (assets::nameHash(object.instance) == destination.pointHash) return assets::objectStart(destinationLevel, object.instance);
    return std::nullopt;
}

bool SceneChangeFlow::request(const SceneChangeDestination& destination) {
    if (destination_) return false;
    destination_ = destination;
    confirmed_ = false;
    waited_ = 0;
    return true;
}

void SceneChangeFlow::update(bool confirmPress) {
    if (!destination_) return;
    if (!confirmed_) {
        confirmed_ = confirmPress;
        waited_ = 0;
        return;
    }
    ++waited_;
}

std::optional<SceneChangeDestination> SceneChangeFlow::takeSwitch() {
    if (!destination_ || !confirmed_ || waited_ < kSceneChangeSwitchDelayUpdates) return std::nullopt;
    std::optional<SceneChangeDestination> due = destination_;
    destination_.reset();
    confirmed_ = false;
    return due;
}

}

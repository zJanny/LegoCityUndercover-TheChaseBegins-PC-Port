#include "runtime/entity_ids.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

namespace chase::runtime {
namespace {

constexpr std::uint8_t kNeverCreated = 1;
constexpr std::uint8_t kDefaultSceneOnly = 2;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

SceneEntityIds::SceneEntityIds(const assets::Level& level, const assets::ClassLookup& classOf, bool defaultScene) {
    count_ = 1;
    std::function<void(const assets::LevelObject&, const std::string&)> create = [&](const assets::LevelObject& object, const std::string& name) {
        if (object.createFlags & kNeverCreated) return;
        if ((object.createFlags & kDefaultSceneOnly) && !defaultScene) return;
        ids_.emplace(lower(name), std::uint16_t(count_));
        ++count_;
        if (const assets::Level* classFile = classOf ? classOf(object.className) : nullptr)
            for (const assets::LevelObject& child : classFile->objects) create(child, name + "." + child.instance);
    };
    for (const assets::LevelObject& object : level.objects) create(object, object.instance);
}

std::optional<std::uint16_t> SceneEntityIds::find(const std::string& lowerCaseName) const {
    const auto found = ids_.find(lowerCaseName);
    if (found == ids_.end()) return std::nullopt;
    return found->second;
}

}

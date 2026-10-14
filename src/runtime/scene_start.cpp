#include "runtime/scene_start.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <set>

namespace chase::runtime {
namespace {
using assets::Level;
using assets::LevelObject;
using assets::LevelParam;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::string stringOf(const LevelParam& param) {
    if (param.type != 1 || param.blocks.empty()) return {};
    const assets::Bytes& block = param.blocks[0];
    const auto end = std::find(block.begin(), block.end(), 0);
    return std::string(block.begin(), end);
}

std::size_t boundIndex(const Level& level, const std::string& name) {
    const std::string wanted = lower(name);
    for (std::size_t i = 0; i < level.bounds.size(); ++i)
        if (lower(level.bounds[i].name) == wanted) return i;
    return level.bounds.size();
}

bool hasScript(const Level& cls, const std::string& name) {
    const std::uint32_t hash = assets::nameHash(name);
    return std::any_of(cls.scripts.begin(), cls.scripts.end(), [hash](const assets::LevelScript& script) { return script.nameHash == hash; });
}

void switchOffUnresolvedTriggers(const Level& level, ScriptRunner& runner) {
    for (const std::size_t index : triggersWithoutObject(level)) runner.setTriggerEnabled(index, false);
}
}

std::vector<std::size_t> cameraBoundsSwitchedOffAtStart(const Level& level, const assets::ClassLookup& classOf) {
    std::vector<std::size_t> switched;
    std::set<std::size_t> seen;
    for (const char* paramName : kCameraBoundParams) {
        const std::uint32_t hash = assets::nameHash(paramName);
        for (const LevelObject& object : level.objects) {
            const Level* cls = classOf(object.className);
            if (!cls) continue;
            const LevelParam* param = effectiveParam(object, *cls, hash);
            if (!param) continue;
            const std::string name = stringOf(*param);
            if (name.empty()) continue;
            const std::size_t bound = boundIndex(level, name);
            if (bound < level.bounds.size() && seen.insert(bound).second) switched.push_back(bound);
        }
    }
    std::sort(switched.begin(), switched.end());
    return switched;
}

std::vector<std::size_t> triggersWithoutObject(const Level& level) {
    std::set<std::string> instances;
    for (const LevelObject& object : level.objects) instances.insert(lower(object.instance));
    std::vector<std::size_t> missing;
    for (std::size_t i = 0; i < level.triggers.size(); ++i) {
        const std::string& name = level.triggers[i].objectName;
        if (!name.empty() && !instances.count(lower(name))) missing.push_back(i);
    }
    return missing;
}

int roomNumber(const std::string& room) {
    const std::string name = lower(room);
    if (name.size() < 5 || name.compare(0, 4, "room") != 0) return -1;
    int value = 0;
    for (std::size_t i = 4; i < name.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return -1;
        value = value * 10 + (name[i] - '0');
    }
    return value;
}

std::string sceneEnterScriptName(int room) { return "OnSceneEnter" + std::to_string(room); }

SceneStart::SceneStart(const Level& level, const assets::ClassLookup& classOf, ScriptRunner& levelRunner)
    : levelRunner_(levelRunner) {
    switchOffUnresolvedTriggers(level, levelRunner);
    cameraBounds_ = cameraBoundsSwitchedOffAtStart(level, classOf);
    for (std::size_t i = level.objects.size(); i-- > 0;) {
        const LevelObject& object = level.objects[i];
        const Level* cls = classOf(object.className);
        if (!cls || !hasScript(*cls, "startup")) continue;
        auto owner = std::make_unique<ClassOwner>();
        owner->objectIndex = i;
        owner->instance = object.instance;
        owner->className = object.className;
        owner->runner = std::make_unique<ScriptRunner>(*cls, owner->state);
        switchOffUnresolvedTriggers(*cls, *owner->runner);
        owners_.push_back(std::move(owner));
    }
}

std::uint32_t SceneStart::run(const SceneStartOptions& options) {
    for (const std::unique_ptr<ClassOwner>& owner : owners_) owner->runner->start("startup");
    const bool levelStarted = levelRunner_.start("startup");
    for (const std::size_t bound : cameraBounds_) levelRunner_.setBoundEnabled(bound, false);
    if (options.startRoom >= 0) levelRunner_.start(sceneEnterScriptName(options.startRoom));
    if (!levelStarted && !options.untilAllEnded) return 0;
    std::uint32_t updates = 0;
    while (updates < options.maxUpdates) {
        const bool pending = options.untilAllEnded ? levelRunner_.threadCount() > 0 || classThreadCount() > 0
                                                   : levelRunner_.running("startup", false);
        if (!pending) break;
        levelRunner_.update(options.dt);
        update(options.dt);
        ++updates;
        if (options.yieldWhen && options.yieldWhen()) break;
    }
    return updates;
}

void SceneStart::update(float dt) {
    for (const std::unique_ptr<ClassOwner>& owner : owners_) owner->runner->update(dt);
}

std::size_t SceneStart::classThreadCount() const {
    std::size_t count = 0;
    for (const std::unique_ptr<ClassOwner>& owner : owners_) count += owner->runner->threadCount();
    return count;
}

bool SceneStart::startRequested() {
    if (!extrasRequested_) return false;
    extrasRequested_ = false;
    return levelRunner_.start(kExtrasCheckScript);
}

}

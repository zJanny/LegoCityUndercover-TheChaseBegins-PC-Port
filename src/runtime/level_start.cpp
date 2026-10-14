#include "runtime/level_start.hpp"
#include <algorithm>
#include <cctype>
#include <map>

namespace chase::runtime {
namespace {
using assets::Level;
using assets::LevelObject;
using assets::LevelParam;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const LevelParam* findParam(const std::vector<LevelParam>& params, std::uint32_t hash) {
    for (const LevelParam& param : params)
        if (param.hash == hash) return &param;
    return nullptr;
}

const LevelParam* effectiveParam(const LevelObject& object, const Level& cls, std::uint32_t hash) {
    const LevelParam* classParam = findParam(cls.defaults, hash);
    if (!classParam) classParam = findParam(cls.classParams, hash);
    if (!classParam) return nullptr;
    if (classParam->flags & 2)
        if (const LevelParam* instance = findParam(object.params, hash)) return instance;
    return classParam;
}

void hideRevealedObjects(const Level& level, const assets::ClassLookup& classOf, assets::PlacedObjectState& state) {
    const std::uint32_t links[] = {assets::nameHash("BuilditFinalProp"), assets::nameHash("PropToReveal"), assets::nameHash("RevealObject")};
    std::map<std::uint32_t, std::string> instanceByHash;
    for (const LevelObject& object : level.objects) instanceByHash[assets::nameHash(object.instance)] = lower(object.instance);
    for (const LevelObject& object : level.objects) {
        const Level* cls = classOf(object.className);
        if (!cls) continue;
        for (const std::uint32_t link : links) {
            const LevelParam* param = effectiveParam(object, *cls, link);
            if (!param) continue;
            const auto target = instanceByHash.find(assets::objectReferenceHash(*param));
            if (target != instanceByHash.end()) state.disabled.insert(target->second);
        }
    }
}
}

assets::PlacedObjectState levelInitialState(const Level& level, const assets::ClassLookup& classOf) {
    assets::PlacedObjectState state;
    hideRevealedObjects(level, classOf, state);
    return state;
}

std::uint32_t runLevelStartup(ScriptRunner& runner, float dt, std::uint32_t maxUpdates) {
    if (!runner.start("startup")) return 0;
    std::uint32_t updates = 0;
    while (runner.threadCount() > 0 && updates < maxUpdates) {
        runner.update(dt);
        ++updates;
    }
    return updates;
}

assets::PlacedObjectState levelStartState(const Level& level, const assets::ClassLookup& classOf) {
    assets::PlacedObjectState state = levelInitialState(level, classOf);
    ScriptRunner runner(level, state);
    runLevelStartup(runner, kStartupStepDt, kStartupMaxUpdates);
    return state;
}

}

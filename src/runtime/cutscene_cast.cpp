#include "runtime/cutscene_cast.hpp"
#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {
std::string lowered(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

bool contains(const std::string& text, const char* part) {
    return lowered(text).find(part) != std::string::npos;
}
}

CastTarget resolveCastTarget(const std::string& folder, const std::string& modelFile, bool playerExists, const std::vector<CastEntity>& entities) {
    if (contains(modelFile, "cape")) return {};
    const std::string name = lowered(folder);
    if (name.find("player") != std::string::npos) {
        if (playerExists && contains(modelFile, "minifig") && name == "player1") return {CastTargetKind::Player, 0};
        return {};
    }
    for (const CastEntity& entity : entities)
        if (!name.empty() && lowered(entity.cutsceneName) == name)
            return {entity.prop ? CastTargetKind::LevelProp : CastTargetKind::LevelCharacter, entity.index};
    return {};
}

CastTarget resolveCastTarget(const std::string& folder, const std::string& modelFile, bool playerExists,
                             const std::vector<std::string>& characterCutsceneNames) {
    std::vector<CastEntity> entities;
    for (std::size_t i = 0; i < characterCutsceneNames.size(); ++i) entities.push_back({characterCutsceneNames[i], false, i});
    return resolveCastTarget(folder, modelFile, playerExists, entities);
}

}

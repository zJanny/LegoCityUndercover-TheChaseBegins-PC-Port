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

CastTarget resolveCastTarget(const std::string& folder, const std::string& modelFile, bool playerExists,
                             const std::vector<std::string>& characterCutsceneNames) {
    if (contains(modelFile, "cape")) return {};
    const std::string name = lowered(folder);
    if (name.find("player") != std::string::npos) {
        if (playerExists && contains(modelFile, "minifig") && name == "player1") return {CastTargetKind::Player, 0};
        return {};
    }
    for (std::size_t i = 0; i < characterCutsceneNames.size(); ++i)
        if (!name.empty() && lowered(characterCutsceneNames[i]) == name) return {CastTargetKind::LevelCharacter, i};
    return {};
}

}

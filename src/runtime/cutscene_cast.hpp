#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace chase::runtime {

enum class CastTargetKind { Standalone, Player, LevelCharacter };

struct CastTarget {
    CastTargetKind kind = CastTargetKind::Standalone;
    std::size_t characterIndex = 0;
};

// Which existing entity a cutscene actor is (cutscene object builder 0x2d1ddc, docs/re/cutscene-actors.md). `folder` is the actor
// folder name and `modelFile` the model file name from the cutscene manifest. `characterCutsceneNames` holds the CSCharacter
// parameter of each level character in entity order. Anything without an entity gets a model of its own (Standalone).
CastTarget resolveCastTarget(const std::string& folder, const std::string& modelFile, bool playerExists,
                             const std::vector<std::string>& characterCutsceneNames);

}

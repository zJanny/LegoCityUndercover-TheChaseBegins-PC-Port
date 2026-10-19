#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace chase::runtime {

enum class CastTargetKind { Standalone, Player, LevelCharacter, LevelProp };

struct CastTarget {
    CastTargetKind kind = CastTargetKind::Standalone;
    std::size_t index = 0;
};

struct CastEntity {
    std::string cutsceneName;
    bool prop = false;
    std::size_t index = 0;
};

// Which existing entity a cutscene actor is (cutscene object builder 0x2d1ddc, docs/re/cutscene-actors.md). `folder` is the actor
// folder name and `modelFile` the model file name from the cutscene manifest. `characterCutsceneNames` holds the CSCharacter
// parameter of each level character in entity order. Anything without an entity gets a model of its own (Standalone).
CastTarget resolveCastTarget(const std::string& folder, const std::string& modelFile, bool playerExists,
                             const std::vector<std::string>& characterCutsceneNames);

// The same rule over every entity of the level in entity order, characters and props alike: the builder compares the folder with the
// CSCharacter of each entity and takes the first match whatever its kind (0x2d1fc8..0x2d1fec). `index` of the result is the `index` of
// the entity in its own list.
CastTarget resolveCastTarget(const std::string& folder, const std::string& modelFile, bool playerExists, const std::vector<CastEntity>& entities);

}

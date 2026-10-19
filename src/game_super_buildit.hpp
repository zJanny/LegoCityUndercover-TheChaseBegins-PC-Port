#pragma once
// LevelSession glue of the super build-its (runtime/super_buildit.hpp, frontend/super_build_prompt.hpp): the prompt page, the confirmation
// dialog, the movie and the models of the plate and of its revealed prop.
#include <cstdint>
#include <string>
#include <vector>

namespace game_super_buildit {

struct Model {
    std::string instance;  // lower case "parent.child"
    std::string parent;
    std::size_t sceneIndex = 0;
};

struct State {
    bool moviePlaying = false;
    bool promptOn = false;
    float promptTime = 1.0e9f;
    std::vector<std::string> pictures;  // tables/tablessuperbuildit sheet 0 column 1
    std::vector<Model> models;
};

}

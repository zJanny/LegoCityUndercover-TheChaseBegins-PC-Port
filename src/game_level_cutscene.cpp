#include "game_level.hpp"
#include <algorithm>
#include <cctype>

namespace game_level {

bool LevelSession::holdForMovie(float dt) {
    chase::runtime::CutsceneNativeState& state = levelScripts.cutscenes();
    const chase::runtime::MovieRequest* request = state.movie();
    if (!request) return false;
    if (!movieStarted) {
        movieStarted = true;
        std::string name = request->name;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        const bool skip = introMovieSkip && name == "01_gameintro";
        if (skip) introMovieSkip = false;
        if (skip || !movies->open(name)) {
            state.finishMovie();
            movieStarted = false;
            return false;
        }
    }
    if (movies->advance(dt)) return true;
    state.finishMovie();
    movieStarted = false;
    return false;
}

}

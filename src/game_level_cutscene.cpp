#include "game_level.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace game_level {

void LevelSession::logCutsceneChange() {
    const chase::runtime::CutsceneNativeState& state = levelScripts.cutscenes();
    std::string current;
    if (const auto* request = state.movie()) current = "movie " + request->name;
    else if (const auto* playing = state.playing()) current = "cutscene " + playing->name;
    if (current == loggedCutscene) return;
    if (!loggedCutscene.empty()) std::cout << loggedCutscene << " ended at update " << updates << '\n';
    if (!current.empty()) std::cout << current << " started at update " << updates << '\n';
    loggedCutscene = current;
}

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

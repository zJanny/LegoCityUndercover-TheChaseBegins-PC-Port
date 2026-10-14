#pragma once
// Host of the cutscene natives (runtime/natives_cutscene.hpp) in chase-game: loads the in-engine cutscenes the scripts name,
// poses the cast in the level's scene, plays the soundtrack, gives the cutscene camera, and plays the movies of PlayCutscene.
#include "assets/assets.hpp"
#include "frontend/canvas.hpp"
#include "frontend/movie_host.hpp"
#include "frontend/resources.hpp"
#include "game_audio.hpp"
#include "game_movie_sound.hpp"
#include "render/scene.hpp"
#include "runtime/natives_cutscene.hpp"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace game_cutscene {

// Clip planes of the viewer's cutscene playback; the original's are not recovered (MOVIE-S10).
constexpr float kNearPlane = 0.1f, kFarPlane = 4000.0f;

struct CameraView {
    chase::render::Mat4 view;
    float fieldOfView = 0;
};

class InEngine {
public:
    InEngine(chase::assets::FuseArchive& archive, chase::render::Scene& scene, std::optional<GameAudio>& audio);
    ~InEngine();
    InEngine(const InEngine&) = delete;
    InEngine& operator=(const InEngine&) = delete;

    // Length of the cutscene in seconds, loading its cast into the scene (hidden); nullopt when its data is missing.
    std::optional<float> load(const std::string& lowerCaseName);
    // Applies the requests the natives made and poses the cast for the cutscene that plays.
    void update(chase::runtime::CutsceneNativeState& state);
    std::optional<CameraView> camera(const chase::runtime::CutsceneNativeState& state) const;

private:
    struct Loaded {
        chase::assets::CutsceneContent content;
        std::vector<std::size_t> instances;
        std::shared_ptr<const chase::assets::Sound> sound;
        double rate = 30;
    };
    void show(const std::string& name, bool visible);
    void pose(const Loaded& cutscene, double seconds);

    chase::assets::FuseArchive& archive_;
    chase::render::Scene& scene_;
    std::optional<GameAudio>& audio_;
    std::map<std::string, Loaded> loaded_;
    std::string shown_;
    int voice_ = 0;
};

class Movies {
public:
    Movies(chase::assets::FuseArchive& archive, chase::frontend::Resources& resources, const std::string& directory, int language);
    ~Movies();
    // Opens the movie `name` (a file of the cutscenes folder, localised when a localised one exists); false without the file.
    bool open(const std::string& name);
    bool playing() const { return playing_; }
    // False when the movie has ended.
    bool advance(float dt);
    void draw(chase::frontend::Canvas& top, chase::frontend::Canvas& bottom);

private:
    chase::assets::FuseArchive& archive_;
    chase::frontend::Resources& resources_;
    chase::frontend::MoviePictureHost host_;
    std::unique_ptr<MovieSound> sound_;
    std::string suffix_;
    std::vector<chase::assets::SubtitleCue> cues_;
    bool playing_ = false;
};

}

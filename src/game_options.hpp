#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace game_options {

struct ParticleRequest {
    unsigned definition = 0;
    std::array<float, 3> position{};
};

struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib";
    std::string names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string config = "config/controls.cfg";
    std::string level = "levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl";
    bool levelGiven = false;
    std::string screenshot;
    unsigned frames = 2;
    std::string script;
    std::vector<std::string> callScripts;
    std::vector<std::pair<std::string, float>> variables;
    std::string saveFile;
    std::string saveOut;
    std::string frontSave;
    unsigned saveSlot = 0;
    bool cameraCollision = true;
    bool startCutscenes = true;
    bool debugOverlay = false;
    std::optional<std::array<float, 3>> start;
    std::optional<float> facing;
    std::vector<ParticleRequest> particles;
    std::optional<int> hideRoom;
    std::string startRoom;
    // Options volumes 0..10 (the game divides them by 10). Read in the oracle at the gameplay start of a new game:
    // sound effects 1.0 (0x37636c + 4), music 0.8 (0x376b3c); the saved options are not read yet.
    unsigned soundVolume = 10, musicVolume = 8;
    // Front-end mode is the default: boot, language, prompts, slot, intro movie, then the level. --gameplay, --script,
    // --level and --start describe a level run and start in the level directly.
    bool gameplay = false;
    std::string frontScript;
    unsigned language = 1;
    bool returningPlayer = false;
    bool introMovieShown = false;
    int windowWidth = 960, windowHeight = 600;
};

// A new game starts in Cherry Tree Hills at the position of the player entity in the oracle at gameplay-start
// (docs/re/oracle.md); the missions that place him there are not recovered. STAND-IN.
inline constexpr const char* kNewGameLevel = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
inline constexpr std::array<float, 3> kNewGameStart = {886.764f, 16.589f, 1111.166f};

struct ScriptStep {
    int stickX = 0, stickY = 0;
    unsigned updates = 0;
    std::uint32_t buttons = 0;
    bool teleport = false;
    std::array<float, 3> position{};
    std::optional<float> facingDegrees;
};

std::vector<ScriptStep> parseScript(const std::string& text);
Options parse(int argc, char** argv);

}

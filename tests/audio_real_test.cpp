// Real-data checks of the sound layer: the sound tables of the extracted RomFS against values read in the original
// game (Azahar oracle, docs/re/audio.md), the listener matrix of the oracle, and a replay of recorded sound-call
// traces (tools/export_sound_trace.py, scenarios in tools/oracle/audio-*.txt) through chase-game's `--script` mode.
// Arguments: fib, names, trace directory, chase-game executable, controls file. Exits with 77 (skipped) when the RomFS, the trace
// directory or the executable is missing.
#include "assets/assets.hpp"
#include "audio/spatial.hpp"
#include "runtime/music.hpp"
#include "runtime/sound_events.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase;
namespace fs = std::filesystem;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    int update = 0;
    unsigned id = 0;
    float x = 0, y = 0, z = 0;
    bool hasPosition = false;
};

std::vector<Call> readTrace(const fs::path& path) {
    std::ifstream in(path);
    require(bool(in), "cannot open " + path.string());
    std::vector<Call> calls;
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        std::stringstream fields(line);
        std::string field;
        std::vector<std::string> columns;
        while (std::getline(fields, field, ',')) columns.push_back(field);
        require(columns.size() >= 4, "bad trace row in " + path.string());
        Call call;
        call.update = std::stoi(columns[0]);
        call.id = unsigned(std::stoul(columns[1]));
        if (columns.size() >= 7) {
            call.x = std::stof(columns[4]);
            call.y = std::stof(columns[5]);
            call.z = std::stof(columns[6]);
            call.hasPosition = true;
        }
        calls.push_back(call);
    }
    return calls;
}

std::vector<Call> runGame(const std::string& game, const std::string& config, const std::string& fib, const std::string& names, const std::string& arguments) {
    const std::string command = "SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \"" + game + "\" --fib \"" + fib + "\" --names \"" + names +
                                "\" --config \"" + config + "\" --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes " +
                                arguments + " 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    require(pipe != nullptr, "cannot start chase-game");
    std::string output;
    char buffer[4096];
    while (std::fgets(buffer, sizeof buffer, pipe)) output += buffer;
    pclose(pipe);
    const std::size_t at = output.find("\nsounds:");
    require(at != std::string::npos, "chase-game printed no sounds line:\n" + output);
    std::stringstream fields(output.substr(at + 8, output.find('\n', at + 1) - at - 8));
    std::vector<Call> calls;
    std::string token;
    while (fields >> token) {
        const std::size_t colon = token.find(':');
        require(colon != std::string::npos, "bad sound token " + token);
        Call call;
        call.update = std::stoi(token.substr(0, colon));
        call.id = unsigned(std::stoul(token.substr(colon + 1)));
        calls.push_back(call);
    }
    return calls;
}

std::string describe(const std::vector<Call>& calls) {
    std::string text;
    for (const Call& call : calls) text += " " + std::to_string(call.update) + ":" + std::to_string(call.id);
    return text;
}

// Updates before which the replica equals the recording exactly; later calls may differ by one update.
void compare(const std::string& name, const std::vector<Call>& recorded, const std::vector<Call>& replayed, std::size_t exactCalls) {
    require(recorded.size() == replayed.size(),
            name + ": recorded" + describe(recorded) + ", replayed" + describe(replayed));
    for (std::size_t i = 0; i < recorded.size(); ++i) {
        const int tolerance = i < exactCalls ? 0 : 1;
        require(recorded[i].id == replayed[i].id && std::abs(recorded[i].update - replayed[i].update) <= tolerance,
                name + ": call " + std::to_string(i) + " differs; recorded" + describe(recorded) + ", replayed" + describe(replayed));
    }
}
}

int main(int argc, char** argv) {
    if (argc < 6) {
        std::cerr << "usage: audio-real-test FIB NAMES TRACE_DIR CHASE_GAME CONTROLS_CFG\n";
        return 2;
    }
    const std::string fibPath = argv[1], namesPath = argv[2], traceDir = argv[3], game = argv[4], config = argv[5];
    if (!fs::exists(fibPath) || !fs::exists(namesPath) || !fs::exists(game)) {
        std::cout << "skipped: RomFS or chase-game missing\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(fibPath);
        archive.loadNames(namesPath);
        const assets::Table soundfx = assets::parseTable(archive.read(*archive.find("tables/tablessoundfx.bxls")));
        const assets::SoundTables tables = assets::parseSoundTables(soundfx);
        const std::vector<assets::SoundEffect> effects = assets::parseSoundEffects(soundfx);
        {
            // Provenance: tablessoundfx dump (build/chase-tabledump) cross-checked with the oracle: row 2 slot 7 = 410 and slot 5
            // = 178 were played by the player (docs/re/audio.md, events part 3), music sets and level bytes read from the heap.
            const assets::Table characters = assets::parseTable(archive.read(*archive.find("tables/tablescharacters.bxls")));
            const std::uint8_t category = assets::characterSoundCategory(characters, 9);
            require(category == 2, "police sound category");
            const auto& row = tables.characterSounds.at(category);
            require(row[runtime::kSlotFootstep] == 178 && row[runtime::kSlotJump] == 288 && row[runtime::kSlotLandFromHeight] == 410 &&
                        row[runtime::kSlotLandWater] == 182 && row[runtime::kSlotLandOnObject11] == 187,
                    "character sound row of the player");
            require(tables.characterSounds.size() == 9 && tables.musicNames.size() == 39 && tables.musicSets.size() == 14, "sound table sizes");
            // Memory at 0x083c3108 (the music set table) and sheet 2.
            const std::uint8_t expectedSets[14][2] = {{2, 3}, {0, 0}, {255, 5}, {1, 1}, {1, 1}, {1, 1}, {7, 4}, {11, 4},
                                                      {8, 5}, {11, 5}, {7, 4}, {11, 4}, {7, 5}, {11, 5}};
            for (std::size_t i = 0; i < 14; ++i)
                require(tables.musicSets[i].ambient == expectedSets[i][0] && tables.musicSets[i].action == expectedSets[i][1], "music set " + std::to_string(i));
            require(effects[178].volumePercent == 20 && effects[178].paramA == 10 && effects[178].paramB == 30, "fs_default1 row");
            require(effects[288].volumePercent == 60 && effects[64].volumePercent == 100 && effects[64].paramB == 100, "jump and coin rows");
        }
        {
            // Level records read from the heap at 0x083682cc (84 bytes each, byte +0x39): lvl_00_policestation (row 1) 1,
            // lvl_01_cherrytreehills (row 2) 2; the level in memory at the gameplay start was row 2 with the set pointer
            // at set 2 and no music stream (0x376b4c = 0, 0x586220 = 0).
            const assets::Table levels = assets::parseTable(archive.read(*archive.find("tables/tableslevels.bxls")));
            require(assets::levelMusicSet(levels, 1) == 1 && assets::levelMusicSet(levels, 2) == 2, "level music sets");
            const runtime::LevelMusic lobby = runtime::levelStartMusic(tables, assets::levelMusicSet(levels, 1), 0);
            require(!lobby.choice.stop && !lobby.choice.track && lobby.trackName.empty(), "lvl_00 starts no music");
            const runtime::LevelMusic street = runtime::levelStartMusic(tables, assets::levelMusicSet(levels, 2), 0);
            require(street.choice.stop && !street.choice.track, "lvl_01 starts no music (ambient entry 255)");
            const std::optional<assets::MusicSet> streetSet = runtime::musicSetOfLevel(tables, assets::levelMusicSet(levels, 2));
            require(streetSet && streetSet->ambient == 255 && streetSet->action == 5 && !runtime::musicSetOfLevel(tables, 14), "level start hands set 2 to the block; set 14 is none (0x2f7160)");
            const runtime::LevelMusic actionStreet = runtime::levelStartMusic(tables, 2, 0);
            const runtime::MoodChoice fight = runtime::chooseMoodTrack(tables.musicSets[2], runtime::kMoodAction, 0);
            require(fight.track && tables.musicNames.at(*fight.track) == "act_fight_music_1" && actionStreet.choice.stop, "action mood of set 2");
        }
        {
            // The listener matrix read at 0x52ac10 (camera at the gameplay start): forward row 2 and position row 3 give rows 0 and 1.
            const audio::Listener listener = audio::listenerFromForward({861.8127f, 21.86995f, 1100.334f}, {0.5344449f, -0.2955031f, -0.7918627f});
            require(std::fabs(listener.right[0] + 0.828879f) < 1e-4f && std::fabs(listener.right[1]) < 1e-6f && std::fabs(listener.right[2] + 0.559428f) < 1e-4f,
                    "listener row 0");
            require(std::fabs(listener.up[0] - 0.1653127f) < 1e-4f && std::fabs(listener.up[1] - 0.9553418f) < 1e-4f &&
                        std::fabs(listener.up[2] + 0.2449363f) < 1e-4f,
                    "listener row 1");
        }
        if (!fs::exists(traceDir)) {
            std::cout << "skipped: no sound traces in " << traceDir << " (tables and listener checked)\n";
            return 77;
        }
        const std::string spawn = "--start \"886.764 16.589 1111.166\" ";
        struct Scenario {
            const char* trace;
            std::string arguments;
            std::size_t exactCalls;
        };
        // Scripts mirror the recorded input (tools/oracle/audio-events*.txt): the stick 0 90 / 0 150 from update 0, B pressed
        // in update 3, falls from a teleport started by a one-update nudge (stick 0 70).
        const std::vector<Scenario> scenarios = {
            {"walk.csv", spawn + "--script \"0 90 70\"", 99},
            {"run.csv", spawn + "--script \"0 150 110\"", 3},
            {"jump.csv", spawn + "--script \"0 0 3;0 0 1 B;0 0 50\"", 99},
            {"fall_0p5.csv", "--start \"886.7 17.1 1111.2\" --script \"0 70 1;0 0 45\"", 99},
            {"fall_2.csv", "--start \"886.7 18.6 1111.2\" --script \"0 70 1;0 0 45\"", 99},
            {"fall_12p5.csv", "--start \"886.7 29.1 1111.2\" --script \"0 70 1;0 0 80\"", 99},
            {"fall_5.csv", "--start \"886.7 21.6 1111.2\" --script \"0 70 1;0 0 55\"", 99},
        };
        std::size_t replayed = 0, compared = 0;
        for (const Scenario& scenario : scenarios) {
            const fs::path path = fs::path(traceDir) / scenario.trace;
            if (!fs::exists(path)) continue;
            const std::vector<Call> recorded = readTrace(path);
            const std::vector<Call> mine = runGame(game, config, fibPath, namesPath, scenario.arguments);
            compare(scenario.trace, recorded, mine, scenario.exactCalls);
            ++replayed;
            compared += recorded.size();
        }
        {
            // The stud pickup: the same sound id at the stud's centre. The update of the pickup is not compared (the skip counter of
            // the recorded far stud is a stand-in, docs/re/studs.md).
            const fs::path path = fs::path(traceDir) / "stud.csv";
            if (fs::exists(path)) {
                const std::vector<Call> recorded = readTrace(path);
                require(recorded.size() == 1 && recorded[0].hasPosition, "stud trace");
                const std::vector<Call> mine = runGame(game, config, fibPath, namesPath, "--start \"862.5931 16.1988 1060.5875\" --script \"0 0 30\"");
                bool found = false;
                for (const Call& call : mine) found = found || call.id == recorded[0].id;
                require(found, "stud pickup plays sound " + std::to_string(recorded[0].id) + ", replayed" + describe(mine));
                ++replayed;
                ++compared;
            }
        }
        require(replayed > 0, "no sound trace was replayed");
        std::cout << "audio-real-test passed: " << replayed << " scenarios, " << compared << " recorded calls\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "audio-real-test failed: " << error.what() << '\n';
        return 1;
    }
}

// Strict command-line parsing of chase-game (src/game_options.cpp). Pure parsing: no game data, no SDL.
#include "game_options.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void fail(const std::string& what) {
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

game_options::Options parseArgs(const std::vector<std::string>& args) {
    std::vector<std::string> argv = {"chase-game"};
    argv.insert(argv.end(), args.begin(), args.end());
    std::vector<char*> pointers;
    for (std::string& arg : argv) pointers.push_back(arg.data());
    return game_options::parse(int(pointers.size()), pointers.data());
}

void expectRejected(const std::vector<std::string>& args, const std::string& message) {
    std::string joined;
    for (const std::string& arg : args) joined += (joined.empty() ? "" : " ") + arg;
    try {
        parseArgs(args);
        fail("accepted, expected rejection '" + message + "': " + joined);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()) != message)
            fail("message for " + joined + " is '" + std::string(error.what()) + "', expected '" + message + "'");
    }
}

void expectUsageRejected(const std::vector<std::string>& args, const std::string& prefix) {
    try {
        parseArgs(args);
        fail("accepted unknown option " + args[0]);
    } catch (const std::runtime_error& error) {
        if (std::string(error.what()).rfind(prefix, 0) != 0)
            fail("message is '" + std::string(error.what()) + "', expected prefix '" + prefix + "'");
    }
}

void requireEqual(const std::string& actual, const std::string& expected, const std::string& what) {
    if (actual != expected) fail(what + ": got '" + actual + "', expected '" + expected + "'");
}

void testDefaults() {
    const game_options::Options options = parseArgs({});
    if (options.language != 1 || options.frames != 2 || options.saveSlot != 0 || options.soundVolume != 10 || options.musicVolume != 8)
        fail("defaults changed");
    if (options.gameplay || options.start || options.facing) fail("defaults start a level");
}

void testLevelGiven() {
    if (parseArgs({"--save", "savegame.dat"}).levelGiven) fail("--save alone must not count as a given level");
    const game_options::Options given = parseArgs({"--save", "savegame.dat", "--level", "levels/x.blvl"});
    if (!given.levelGiven || given.level != "levels/x.blvl") fail("--level must override the level of the save");
}

void testValidValues() {
    const game_options::Options slot = parseArgs({"--slot", "1", "--language", "9", "--sound-volume", "0", "--music-volume", "10"});
    if (slot.saveSlot != 1 || slot.language != 9 || slot.soundVolume != 0 || slot.musicVolume != 10) fail("valid numbers changed");

    const game_options::Options window = parseArgs({"--window", "400x480"});
    if (window.windowWidth != 400 || window.windowHeight != 480) fail("valid --window changed");

    const game_options::Options start = parseArgs({"--start", "886.764 16.589 1111.166", "--facing", "-90.5"});
    if (!start.start || std::fabs((*start.start)[0] - 886.764f) > 1e-3f || std::fabs((*start.start)[2] - 1111.166f) > 1e-3f || !start.gameplay) fail("valid --start changed");
    if (!start.facing || *start.facing != -90.5f) fail("valid --facing changed");
    const game_options::Options particles = parseArgs({"--particle", "286 1 2.5 -3", "--particle", "0 0 0 0"});
    if (particles.particles.size() != 2 || particles.particles[0].definition != 286 || particles.particles[0].position[1] != 2.5f ||
        particles.particles[0].position[2] != -3.0f || !particles.gameplay)
        fail("valid --particle changed");
    if (parseArgs({"--hide-room", "4"}).hideRoom != 4 || parseArgs({}).hideRoom) fail("valid --hide-room changed");

    const game_options::Options script = parseArgs({"--script", "0 150 40;0 0 1 AB;-156 0 0"});
    requireEqual(script.script, "0 150 40;0 0 1 AB;-156 0 0", "valid --script text");
    requireEqual(parseArgs({"--script", "0 0 5;touch 159 65 3"}).script, "0 0 5;touch 159 65 3", "valid touch step");
    const std::vector<game_options::ScriptStep> touchSteps = game_options::parseScript("touch 159 65 3");
    requireEqual(std::to_string(touchSteps.size()), "1", "one touch step");
    requireEqual(std::to_string(touchSteps[0].touch) + " " + std::to_string(touchSteps[0].touchX) + " " + std::to_string(touchSteps[0].touchY) + " " + std::to_string(touchSteps[0].updates), "1 159 65 3", "touch fields");
    const std::vector<game_options::ScriptStep> steps = game_options::parseScript("0 150 40;0 0 1 AB;-156 0 0");
    if (steps.size() != 3 || steps[0].stickY != 150 || steps[0].updates != 40 || steps[1].buttons == 0 || steps[2].stickX != -156)
        fail("parseScript steps changed");
    const std::vector<game_options::ScriptStep> teleports = game_options::parseScript("0 0 3;at 817.5 24 1071.25 -30;at 1 2 3");
    if (teleports.size() != 3 || !teleports[1].teleport || teleports[1].updates != 0 || teleports[1].position[0] != 817.5f ||
        teleports[1].position[2] != 1071.25f || !teleports[1].facingDegrees || *teleports[1].facingDegrees != -30.0f ||
        teleports[2].facingDegrees || teleports[0].teleport)
        fail("parseScript teleport steps changed");
    const std::vector<game_options::ScriptStep> exits = game_options::parseScript("0 0 3;left-vehicle 1-4_Chaser 1-4_Car;0 0 1 A");
    if (exits.size() != 3 || exits[1].updates != 0 || exits[1].leftVehicleCharacter != "1-4_Chaser" || exits[1].leftVehicleVehicle != "1-4_Car" ||
        !exits[0].leftVehicleCharacter.empty())
        fail("parseScript left-vehicle step changed");

    const game_options::Options variables = parseArgs({"--set-variable", "Lobby_Dcam_Intro=1", "--set-variable", "Other=-2.5"});
    if (variables.variables.size() != 2 || variables.variables[0].first != "Lobby_Dcam_Intro" || variables.variables[0].second != 1.0f || variables.variables[1].second != -2.5f)
        fail("valid --set-variable changed");

    if (parseArgs({}).superBricks) fail("--super-bricks set by default");
    requireEqual(std::to_string(*parseArgs({"--super-bricks", "250"}).superBricks), "250", "valid --super-bricks");
    requireEqual(std::to_string(*parseArgs({"--super-bricks", "0"}).superBricks), "0", "--super-bricks 0");
    requireEqual(std::to_string(*parseArgs({"--super-bricks", "4294967295"}).superBricks), "4294967295", "--super-bricks maximum");

    if (parseArgs({}).extras) fail("--extras set by default");
    requireEqual(std::to_string(*parseArgs({"--extras", "524288"}).extras), "524288", "valid --extras");
    requireEqual(std::to_string(*parseArgs({"--extras", "4294967295"}).extras), "4294967295", "--extras maximum");

    const game_options::Options front = parseArgs({"--front-script", "until language 60; run 0.5; press A; shot x"});
    requireEqual(front.frontScript, "until language 60; run 0.5; press A; shot x", "valid --front-script text");
}

void testRejections() {
    expectRejected({"--slot", "1x"}, "--slot: not a whole number: '1x'");
    if (parseArgs({"--save-out", "out.dat"}).saveOut != "out.dat") fail("--save-out without --save: a new game writes there");
    expectRejected({"--save", "a.dat", "--save-out", "./a.dat"}, "--save-out: must not be the file given with --save: './a.dat'");
    expectRejected({"--save-out"}, "--save-out: missing value: ''");
    if (parseArgs({"--save", "a.dat", "--save-out", "b.dat"}).saveOut != "b.dat") fail("--save-out value lost");
    expectRejected({"--front-save"}, "--front-save: missing value: ''");
    expectRejected({"--front-save", "a.dat", "--gameplay"}, "--front-save: only the front end reads it, not a level run (--gameplay, --script, --level, --start, --save): 'a.dat'");
    expectRejected({"--front-save", "a.dat", "--save", "b.dat"}, "--front-save: only the front end reads it, not a level run (--gameplay, --script, --level, --start, --save): 'a.dat'");
    if (parseArgs({"--front-save", "a.dat"}).frontSave != "a.dat") fail("--front-save value lost");
    expectRejected({"--slot", "2"}, "--slot: out of range, allowed 0 to 1: '2'");
    expectRejected({"--slot", " 1"}, "--slot: not a whole number: ' 1'");
    expectRejected({"--language", "0"}, "--language: out of range, allowed 1 to 9: '0'");
    expectRejected({"--sound-volume", "11"}, "--sound-volume: out of range, allowed 0 to 10: '11'");
    expectRejected({"--music-volume", "-1"}, "--music-volume: out of range, allowed 0 to 10: '-1'");
    expectRejected({"--frames", "-1"}, "--frames: out of range, allowed 0 to 1000000: '-1'");
    expectRejected({"--facing", "nan"}, "--facing: not a number: 'nan'");
    expectRejected({"--hide-room", "0"}, "--hide-room: out of range, allowed 1 to 99: '0'");
    expectRejected({"--hide-room", "Room4"}, "--hide-room: not a whole number: 'Room4'");
    expectRejected({"--facing", "90deg"}, "--facing: not a number: '90deg'");
    expectRejected({"--set-variable", "Lobby_Dcam_Intro"}, "--set-variable: expected NAME=VALUE: 'Lobby_Dcam_Intro'");
    expectRejected({"--set-variable", "=1"}, "--set-variable: expected a variable name before '=': '=1'");
    expectRejected({"--set-variable", "a b=1"}, "--set-variable: expected a variable name before '=': 'a b=1'");
    expectRejected({"--set-variable", "Lobby_Dcam_Intro=on"}, "--set-variable: not a number: 'on'");
    expectRejected({"--set-variable", "Lobby_Dcam_Intro="}, "--set-variable: not a number: ''");
    expectRejected({"--set-variable"}, "--set-variable: missing value: ''");
    expectRejected({"--super-bricks"}, "--super-bricks: missing value: ''");
    expectRejected({"--super-bricks", "many"}, "--super-bricks: not a whole number: 'many'");
    expectRejected({"--super-bricks", "-1"}, "--super-bricks: out of range, allowed 0 to 4294967295: '-1'");
    expectRejected({"--extras"}, "--extras: missing value: ''");
    expectRejected({"--extras", "all"}, "--extras: not a whole number: 'all'");
    expectRejected({"--extras", "-1"}, "--extras: out of range, allowed 0 to 4294967295: '-1'");
    expectRejected({"--extras", "4294967296"}, "--extras: out of range, allowed 0 to 4294967295: '4294967296'");
    expectRejected({"--super-bricks", "4294967296"}, "--super-bricks: out of range, allowed 0 to 4294967295: '4294967296'");
    expectRejected({"--super-bricks", "2.5"}, "--super-bricks: not a whole number: '2.5'");

    expectRejected({"--window", "400"}, "--window: expected WIDTHxHEIGHT: '400'");
    expectRejected({"--window", "400x480 "}, "--window: not a whole number: '480 '");
    expectRejected({"--window", "100x480"}, "--window: out of range, allowed 200 to 16384: '100'");

    expectRejected({"--start", "1 2"}, "--start: expected X Y Z: '1 2'");
    expectRejected({"--start", "1 2 3x"}, "--start: not a number: '3x'");
    expectRejected({"--start", "1 2 3 4"}, "--start: expected X Y Z: '1 2 3 4'");
    expectRejected({"--particle", "1 2 3"}, "--particle: expected DEFINITION X Y Z: '1 2 3'");
    expectRejected({"--particle", "1 2 3 4 5"}, "--particle: expected DEFINITION X Y Z: '1 2 3 4 5'");
    expectRejected({"--particle", "x 2 3 4"}, "--particle: not a whole number: 'x'");
    expectRejected({"--particle", "-1 2 3 4"}, "--particle: out of range, allowed 0 to 65535: '-1'");
    expectRejected({"--particle", "65536 2 3 4"}, "--particle: out of range, allowed 0 to 65535: '65536'");
    expectRejected({"--particle", "1 2 3 4x"}, "--particle: not a number: '4x'");
    expectRejected({"--particle"}, "--particle: missing value: ''");

    expectRejected({"--script", "0 150"}, "--script: expected X Y COUNT [BUTTONS]: '0 150'");
    expectRejected({"--script", "0 150 40;"}, "--script: expected X Y COUNT [BUTTONS]: ''");
    expectRejected({"--script", "0 150 40 AQ"}, "--script: unknown button: 'Q'");
    expectRejected({"--script", "0 200 40"}, "--script: out of range, allowed -156 to 156: '200'");
    expectRejected({"--script", "0 150 4.5"}, "--script: not a whole number: '4.5'");
    expectRejected({"--script", "at 1 2"}, "--script: expected at X Y Z [FACING]: 'at 1 2'");
    expectRejected({"--script", "at 1 2 3 4 5"}, "--script: expected at X Y Z [FACING]: 'at 1 2 3 4 5'");
    expectRejected({"--script", "at 1 two 3"}, "--script: not a number: 'two'");
    expectRejected({"--script", "0 150 40 AB C"}, "--script: expected X Y COUNT [BUTTONS]: '0 150 40 AB C'");
    expectRejected({"--script", "touch 159 65"}, "--script: expected touch X Y COUNT: 'touch 159 65'");
    expectRejected({"--script", "left-vehicle chaser"}, "--script: expected left-vehicle CHARACTER VEHICLE: 'left-vehicle chaser'");
    expectRejected({"--script", "left-vehicle a b c"}, "--script: expected left-vehicle CHARACTER VEHICLE: 'left-vehicle a b c'");
    expectRejected({"--script", "touch 159 65 3 A"}, "--script: expected touch X Y COUNT: 'touch 159 65 3 A'");
    expectRejected({"--script", "touch 320 65 3"}, "--script: out of range, allowed 0 to 319: '320'");
    expectRejected({"--script", "touch 159 -1 3"}, "--script: out of range, allowed 0 to 239: '-1'");
    expectRejected({"--script", "touch 159 65 x"}, "--script: not a whole number: 'x'");

    expectRejected({"--front-script", "run 1;;press A"}, "--front-script: empty command: ''");
    expectRejected({"--front-script", "press Q"}, "--front-script: unknown button: 'Q'");
    expectRejected({"--front-script", "until nowhere"}, "--front-script: unknown screen: 'nowhere'");
    expectRejected({"--front-script", "run abc"}, "--front-script: not a number: 'abc'");
    expectRejected({"--front-script", "run 1 2"}, "--front-script: wrong number of fields: 'run 1 2'");
    expectRejected({"--front-script", "jump"}, "--front-script: unknown command: 'jump'");

    expectRejected({"--fib"}, "--fib: missing value: ''");
    expectRejected({"--fib", ""}, "--fib: empty value: ''");
    expectUsageRejected({"--bogus"}, "--bogus: unknown option: '--bogus'\n");
    expectUsageRejected({"gameplay"}, "gameplay: unknown option: 'gameplay'\n");
}

}

int main() {
    testDefaults();
    testValidValues();
    testLevelGiven();
    testRejections();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "game options: strict parsing ok\n";
    return EXIT_SUCCESS;
}

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

    const game_options::Options script = parseArgs({"--script", "0 150 40;0 0 1 AB;-156 0 0"});
    requireEqual(script.script, "0 150 40;0 0 1 AB;-156 0 0", "valid --script text");
    const std::vector<game_options::ScriptStep> steps = game_options::parseScript("0 150 40;0 0 1 AB;-156 0 0");
    if (steps.size() != 3 || steps[0].stickY != 150 || steps[0].updates != 40 || steps[1].buttons == 0 || steps[2].stickX != -156)
        fail("parseScript steps changed");

    const game_options::Options front = parseArgs({"--front-script", "until language 60; run 0.5; press A; shot x"});
    requireEqual(front.frontScript, "until language 60; run 0.5; press A; shot x", "valid --front-script text");
}

void testRejections() {
    expectRejected({"--slot", "1x"}, "--slot: not a whole number: '1x'");
    expectRejected({"--slot", "2"}, "--slot: out of range, allowed 0 to 1: '2'");
    expectRejected({"--slot", " 1"}, "--slot: not a whole number: ' 1'");
    expectRejected({"--language", "0"}, "--language: out of range, allowed 1 to 9: '0'");
    expectRejected({"--sound-volume", "11"}, "--sound-volume: out of range, allowed 0 to 10: '11'");
    expectRejected({"--music-volume", "-1"}, "--music-volume: out of range, allowed 0 to 10: '-1'");
    expectRejected({"--frames", "-1"}, "--frames: out of range, allowed 0 to 1000000: '-1'");
    expectRejected({"--facing", "nan"}, "--facing: not a number: 'nan'");
    expectRejected({"--facing", "90deg"}, "--facing: not a number: '90deg'");

    expectRejected({"--window", "400"}, "--window: expected WIDTHxHEIGHT: '400'");
    expectRejected({"--window", "400x480 "}, "--window: not a whole number: '480 '");
    expectRejected({"--window", "100x480"}, "--window: out of range, allowed 200 to 16384: '100'");

    expectRejected({"--start", "1 2"}, "--start: expected X Y Z: '1 2'");
    expectRejected({"--start", "1 2 3x"}, "--start: not a number: '3x'");
    expectRejected({"--start", "1 2 3 4"}, "--start: expected X Y Z: '1 2 3 4'");

    expectRejected({"--script", "0 150"}, "--script: expected X Y COUNT [BUTTONS]: '0 150'");
    expectRejected({"--script", "0 150 40;"}, "--script: expected X Y COUNT [BUTTONS]: ''");
    expectRejected({"--script", "0 150 40 AQ"}, "--script: unknown button: 'Q'");
    expectRejected({"--script", "0 200 40"}, "--script: out of range, allowed -156 to 156: '200'");
    expectRejected({"--script", "0 150 4.5"}, "--script: not a whole number: '4.5'");
    expectRejected({"--script", "0 150 40 AB C"}, "--script: expected X Y COUNT [BUTTONS]: '0 150 40 AB C'");

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

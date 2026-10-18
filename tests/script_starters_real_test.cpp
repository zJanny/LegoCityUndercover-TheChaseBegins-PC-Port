// Mission 1-2 scripts without a starter in lvl_01_cherrytreehills (docs/re/scripts-missions.md, SCRIPT-113 / SCRIPT-285). Needs the extracted RomFS;
// exits with 77 without it. Provenance: every original thread start goes through a script record found by the lowercased CRC-32 of a name string (REA on
// game.arm.elf: 0x22ce9c, 0x2238d0, 0x197c3c and their callers); the level data names neither script 0x921a9a63 nor 0x94b63474 (assets::parseLevel), and the
// Azahar traps of 0x22ce9c / 0x2238d0 / 0x2388e4 from the user's states m12-2, m12-3, m12-7, m12-13 and m12-table-before started neither. The port starts
// scripts only by those names (triggers, waypoint TriggerScript, CallScript / SpawnScript), so neither may be reachable from the level data, and the dumpster
// peek must run as part of 1-2_HouseArrival, which carries its calls.
#include "assets/assets.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";
constexpr std::uint32_t kDumpsterPeek = 0x921a9a63u, kUseHint = 0x94b63474u;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string unquoted(const std::string& text) {
    return text.size() >= 2 && text.front() == '"' && text.back() == '"' ? text.substr(1, text.size() - 2) : text;
}

bool unnamed(std::uint32_t hash) {
    return hash == kDumpsterPeek || hash == kUseHint;
}

const assets::LevelScript& script(const assets::Level& level, std::uint32_t hash) {
    for (const assets::LevelScript& s : level.scripts)
        if (s.nameHash == hash) return s;
    throw std::runtime_error("script not in the level: " + std::to_string(hash));
}

void noNameReachesTheUnnamedScripts(const assets::Level& level) {
    require(script(level, kDumpsterPeek).calls.size() == 12, "the dumpster peek has 12 calls");
    require(script(level, kUseHint).calls.size() == 1 && script(level, kUseHint).calls[0].function == "PrintDebugString", "the hint is one PrintDebugString");
    for (const assets::LevelTrigger& trigger : level.triggers)
        require(!unnamed(assets::nameHash(trigger.scriptName)), "trigger names an unnamed script: " + trigger.scriptName);
    for (const assets::LevelObject& object : level.objects)
        for (const assets::LevelParam& param : object.params) {
            require(!unnamed(assets::objectReferenceHash(param)), "object reference to an unnamed script in " + object.instance);
            for (const assets::Bytes& block : param.blocks) {
                const std::string text(block.begin(), block.end());
                require(!unnamed(assets::nameHash(text.c_str())), "string parameter names an unnamed script in " + object.instance);
            }
        }
    for (const assets::LevelScript& s : level.scripts)
        for (const assets::ScriptCall& call : s.calls)
            for (const std::string& argument : call.arguments)
                require(!unnamed(assets::nameHash(unquoted(argument))), "argument of " + call.function + " names an unnamed script");
}

void houseArrivalCarriesThePeek(const assets::Level& level) {
    const std::vector<std::string> peek = {"TriggerObject", "Wait",       "StartDCamTrack", "PlayCharAnim", "LoadAnimation", "EnableObject",
                                           "EnableObject",  "PlayAnimation", "WaitForDCam",  "SnapCamera",   "Wait",          "EnableObject"};
    const assets::LevelScript& dumpster = script(level, kDumpsterPeek);
    for (std::size_t i = 0; i < peek.size(); ++i) require(dumpster.calls[i].function == peek[i], "dumpster peek call " + std::to_string(i));
    std::vector<const assets::ScriptCall*> wanted;
    for (const assets::ScriptCall& call : dumpster.calls)
        if (call.function != "Wait") wanted.push_back(&call);
    const assets::LevelScript& arrival = script(level, assets::nameHash("1-2_HouseArrival"));
    std::size_t next = 0;
    for (const assets::ScriptCall& call : arrival.calls)
        if (next < wanted.size() && call.function == wanted[next]->function && call.arguments == wanted[next]->arguments) ++next;
    require(wanted.size() == 10 && next == wanted.size(), "1-2_HouseArrival carries the peek's calls (waits aside) in order, matched " + std::to_string(next));
}

void doorLockHintRunsFromItsBound(const assets::Level& level) {
    const std::uint32_t hint = assets::nameHash("1-2_Door_Lock_Hint_Script");
    bool bound = false, object = false;
    for (const assets::LevelTrigger& trigger : level.triggers) {
        if (assets::nameHash(trigger.scriptName) != hint) continue;
        if (trigger.event == "BoundEntered" && trigger.firstArgument == "1-2_Door_Lock_Hint") bound = true;
        if (trigger.event == "ObjectTriggered" && trigger.objectName == "1-2_Door_Lock_Hint_Script") object = true;
    }
    require(bound && object, "Door_Lock_Hint_Script has a BoundEntered and an ObjectTriggered trigger");
    for (const assets::LevelObject& o : level.objects)
        for (const assets::LevelParam& param : o.params) require(assets::objectReferenceHash(param) != hint, "object reference to the hint script in " + o.instance);
    for (const assets::LevelScript& s : level.scripts)
        for (const assets::ScriptCall& call : s.calls)
            for (const std::string& argument : call.arguments)
                require(assets::nameHash(unquoted(argument)) != hint, "argument of " + call.function + " names the hint script object");
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: script-starters-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "script-starters-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find(kLevelFile);
        require(entry != nullptr, std::string("not in archive: ") + kLevelFile);
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        noNameReachesTheUnnamedScripts(level);
        houseArrivalCarriesThePeek(level);
        doorLockHintRunsFromItsBound(level);
    } catch (const std::exception& error) {
        std::cerr << "script-starters-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-starters-real-test: ok\n";
    return 0;
}

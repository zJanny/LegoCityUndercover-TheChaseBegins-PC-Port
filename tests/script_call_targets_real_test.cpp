// Every CallScript / SpawnScript in the eight main levels names a script of its own level, so the original's null-script
// fault of 0x1c3730 is never reached and ScriptRunner may stop on it. Counts recorded 2026-10-19 with the C++ level loader
// (110 + 104 + 89 + 110 + 102 + 151 + 223 + 5 = 894 calls). Needs the extracted RomFS; exits with 77 (skipped) when missing.
#include "assets/assets.hpp"
#include <cctype>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

struct Expected {
    const char* level;
    int calls;
};

std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: script-call-targets-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "script-call-targets-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const Expected levels[] = {{"lvl_00_policestation", 5},  {"lvl_01_cherrytreehills", 110}, {"lvl_02_albatrossisland", 104},
                                   {"lvl_03_auburn", 89},        {"lvl_04_fortmeadows", 110},     {"lvl_05_bluebellwood", 102},
                                   {"lvl_06_apolloisland", 151}, {"lvl_07_downtown", 223}};
        int total = 0;
        for (const Expected& expected : levels) {
            const std::string file = std::string("levels/levels/") + expected.level + "/" + expected.level + "_main.blvl";
            const assets::FuseEntry* entry = archive.find(file);
            require(entry != nullptr, "not in archive: " + file);
            const assets::Level level = assets::parseLevel(archive.read(*entry));
            std::set<std::uint32_t> known;
            for (const assets::LevelScript& script : level.scripts) known.insert(script.nameHash);
            int calls = 0;
            for (const assets::LevelScript& script : level.scripts)
                for (const assets::ScriptCall& call : script.calls) {
                    const std::string function = lowerCase(call.function);
                    if (function != "callscript" && function != "spawnscript") continue;
                    ++calls;
                    require(!call.arguments.empty() && known.count(assets::nameHash(call.arguments[0])) != 0,
                            std::string(expected.level) + ": unresolved script " + (call.arguments.empty() ? "" : call.arguments[0]));
                }
            require(calls == expected.calls, std::string(expected.level) + ": call count " + std::to_string(calls));
            total += calls;
        }
        require(total == 894, "total 894 calls");
    } catch (const std::exception& error) {
        std::cerr << "script-call-targets-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "script-call-targets-real-test: ok\n";
    return 0;
}

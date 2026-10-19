// Every call of the batch SCRIPT-35 natives in the shipped level files (parseLevel of all *.blvl, dumped on 2026-10-19) has the argument count
// the handler reads (census table docs/re/script-natives-census.md; handler reads: docs/re/script-natives-party.md), and the natives are
// handled. Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/natives_party.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: natives-party-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "natives-party-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        std::map<std::string, unsigned> calls;
        for (const std::string& name : archive.names()) {
            if (name.size() < 5 || name.compare(name.size() - 5, 5, ".blvl") != 0) continue;
            const assets::FuseEntry* entry = archive.find(name);
            require(entry != nullptr, name);
            const assets::Level level = assets::parseLevel(archive.read(*entry));
            for (const assets::LevelScript& script : level.scripts)
                for (const assets::ScriptCall& call : script.calls) {
                    const auto kind = runtime::partyNativeKind(lowerCase(call.function));
                    if (!kind) continue;
                    const runtime::PartyNativeShape shape = runtime::partyNativeShape(*kind);
                    const std::size_t needed = shape.objects + shape.numbers + (shape.output ? 1 : 0);
                    require(call.arguments.size() >= needed, call.function + " has fewer arguments than the handler reads in " + name);
                    ++calls[lowerCase(call.function)];
                }
        }
        for (const std::string& name : runtime::partyNativeNames()) std::cout << name << ": " << calls[name] << " calls\n";
        require(calls["swapallegiance"] > 0 && calls["setweaponenabled"] > 0, "the high-call natives occur in the levels");
    } catch (const std::exception& error) {
        std::cerr << "natives-party-real-test: FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "natives-party-real-test: ok\n";
    return 0;
}

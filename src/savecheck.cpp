// chase-savecheck: prints the completion of each slot of a savegame.dat, using the shipped tables.
#include "save/completion_tables.hpp"
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::save;

int run(int argc, char** argv) {
    std::string romfs = "artifacts/romfs/romfs", savePath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--romfs") {
            if (i + 1 >= argc) throw std::runtime_error("missing value for --romfs");
            romfs = argv[++i];
        } else if (savePath.empty()) {
            savePath = arg;
        } else {
            throw std::runtime_error("unexpected argument: " + arg);
        }
    }
    if (savePath.empty()) throw std::runtime_error("usage: chase-savecheck [--romfs DIR] savegame.dat");

    chase::assets::FuseArchive archive(romfs + "/lego_city3ds.fib");
    archive.loadNames(romfs + "/lego_city3ds.csv");
    CompletionTotals totals = completionTotals(archive);

    LoadResult loaded = load(savePath);
    if (!loaded.found) throw std::runtime_error("no such file: " + savePath);
    if (loaded.status != Status::Ok) throw std::runtime_error("not a valid save image (status " + std::to_string(int(loaded.status)) + ")");
    for (std::size_t slot = 0; slot < kSlotCount; ++slot) {
        std::optional<CompletionCounts> counts = completionCounts(loaded.image->slotView(slot), totals);
        if (!counts) throw std::runtime_error("slot " + std::to_string(slot) + " is too short");
        if (counts->collected > counts->total) {
            std::printf("slot %zu: empty (the game's formula wraps below zero here)\n", slot);
            continue;
        }
        unsigned permille = completionPermille(counts->collected, counts->total);
        std::printf("slot %zu: %u/%u collected, %u per mille, %u.%u%%\n", slot, counts->collected, counts->total,
                    permille, permille / 10, permille % 10);
    }
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "chase-savecheck: " << error.what() << '\n';
        return 1;
    }
}

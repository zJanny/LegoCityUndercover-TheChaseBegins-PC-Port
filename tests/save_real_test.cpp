// Opt-in check of the completion totals against the locally extracted tables (never committed).
// Usage: save-real-test <lego_city3ds.fib> <lego_city3ds.csv>; exits 77 (skip) when absent.
#include "save/completion_tables.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace chase::save;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    if (argc != 3 || !std::ifstream(argv[1]) || !std::ifstream(argv[2])) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        CompletionTotals totals = completionTotals(archive);
        // Provenance: docs/re/save-format.md 09:25 (levels, excluded id, total 711) and 09:19 (supercounter sum 271).
        const std::array<std::uint8_t, kCounterLevels> levels{7, 8, 5, 8, 9, 8, 8};
        require(totals.levelTotals == levels, "level totals");
        unsigned counterSum = 0;
        for (const auto& row : totals.categoryTotals)
            for (std::uint8_t cell : row) counterSum += cell;
        require(counterSum == 271, "supercounter sum");
        require(totals.excludedCharacters == std::vector<std::uint8_t>{9}, "excluded characters");
        SaveImage image;
        std::optional<CompletionCounts> counts = completionCounts(image.slotView(0), totals);
        require(counts.has_value(), "completion counts");
        require(counts->total == 711, "completion total");
        std::cout << "completion total " << counts->total << '\n';
    } catch (const std::exception& error) {
        std::cerr << "save-real-test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

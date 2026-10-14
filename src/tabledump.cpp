// chase-tabledump: prints the sheets of tables/*.bxls as tab-separated text.
#include "assets/assets.hpp"
#include "strict_parse.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;

std::string cell(const TableValue& value) {
    if (!value.present) return "-";
    if (value.type == 5 || value.type == 6) return '"' + value.text + '"';
    if (value.type == 4) return std::to_string(value.asFloat());
    if (value.type == 1) return std::to_string(std::int32_t(value.bits));
    return std::to_string(value.bits);
}

int run(int argc, char** argv) {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::vector<std::string> paths;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        strict::forOption(arg, [&] {
            if (arg == "--fib") fib = strict::nextValue(argc, argv, i);
            else if (arg == "--names") names = strict::nextValue(argc, argv, i);
            else if (!arg.empty() && arg[0] == '-') strict::reject("unknown option", arg);
            else paths.push_back(arg);
        });
    }
    if (paths.empty()) throw std::runtime_error("usage: chase-tabledump [--fib F] [--names CSV] tables/<name>.bxls...");
    FuseArchive archive(fib);
    archive.loadNames(names);
    for (const auto& path : paths) {
        const FuseEntry* entry = archive.find(path);
        if (!entry) throw std::runtime_error("not in archive: " + path);
        Table table = parseTable(archive.read(*entry));
        for (std::size_t s = 0; s < table.sheets.size(); ++s) {
            const TableSheet& sheet = table.sheets[s];
            std::cout << "# " << path << " sheet " << s << ": " << sheet.rows << " rows, types";
            for (const auto& column : sheet.columns) std::cout << ' ' << unsigned(column.type);
            std::cout << '\n';
            for (unsigned row = 0; row < sheet.rows; ++row) {
                std::cout << row;
                for (std::size_t column = 0; column < sheet.columns.size(); ++column)
                    std::cout << '\t' << cell(tableValue(sheet, row, column));
                std::cout << '\n';
            }
        }
    }
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "chase-tabledump: " << error.what() << '\n';
        return 1;
    }
}

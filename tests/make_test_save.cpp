// Writes the save of tests/save_fixture.hpp to argv[3] for the chase-game save tests.
// Usage: make-test-save <lego_city3ds.fib> <lego_city3ds.csv> <savegame.dat>
#include "save_fixture.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: make-test-save FIB NAMES SAVEGAME\n";
        return 2;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        chase::save::store(argv[3], chase::testing::userSaveImage(archive));
    } catch (const std::exception& error) {
        std::cerr << "make-test-save: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

// Synthetic save images: header, CRC, validation statuses and atomic storage. No real save exists.
#include "save/save.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
using namespace chase::save;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void writeFile(const std::filesystem::path& path, const Bytes& data) {
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
}
}

int main() {
    try {
        // Reference: zlib crc32("123456789") = 0xcbf43926, and this CRC is its complement (0x248308).
        const std::string check = "123456789";
        require(checksum(reinterpret_cast<const std::uint8_t*>(check.data()), check.size()) == 0x340bc6d9u,
                "CRC variant");
        // Descriptor 0x272948: 0x18 + 0x21c + 2 * 0x5230 = 0xa694.
        require(kFileSize == 0xa694, "file size");

        SaveImage image;
        require(image.validate() == Status::Ok, "fresh image is valid");
        require(image.storedSize() == 0xa694 && image.storedVersion() == 30, "header fields");
        const Bytes& raw = image.bytes();
        require(std::string(raw.begin() + 8, raw.begin() + 24) == "LEGO_ENGINE_SAVE", "magic");
        require(image.slot(0) - raw.data() == 0x234 && image.slot(1) - raw.data() == 0x5464, "slot offsets");
        bool threw = false;
        try { image.slot(2); } catch (const std::out_of_range&) { threw = true; }
        require(threw, "slot index checked");

        image.slot(1)[0x5190] = 0x42;
        require(image.validate() == Status::Corrupt, "edit without seal breaks CRC");
        image.seal();
        require(image.validate() == Status::Ok && image.slot(1)[0x5190] == 0x42, "seal keeps data");

        // Reference: 0x2081cc + 0x1a25a0 (REA ev_4e9c96fe…), globals 0x379d64..66 = 8, 8, 10.
        SaveImage defaults;
        defaults.options()[0x100] = 0x55;
        defaults.resetOptions(languageForSystem(3));
        const std::uint8_t expected[] = {0x8a, 4, 0xff, 0x21, 10, 10, 8, 8, 0};
        require(std::equal(std::begin(expected), std::end(expected), defaults.options()) &&
                    defaults.options()[0x100] == 0,
                "option defaults");
        require(languageForSystem(0) == 1 && languageForSystem(7) == 12 && languageForSystem(11) == 1,
                "language mapping 0x21b650");

        Bytes data = image.bytes();
        data[6] = 29;
        require(SaveImage::fromBytes(data)->validate() == Status::Corrupt, "version mismatch");
        data = image.bytes();
        data[4] ^= 1;
        require(SaveImage::fromBytes(data)->validate() == Status::Corrupt, "size mismatch");
        data = image.bytes();
        data[8] = 'l';
        require(SaveImage::fromBytes(data)->validate() == Status::Unreadable, "bad magic");
        data = image.bytes();
        data.pop_back();
        require(!SaveImage::fromBytes(data), "short image");
        data = image.bytes();
        data.push_back(0xee);
        require(SaveImage::fromBytes(data)->validate() == Status::Ok, "trailing bytes ignored");

        auto dir = std::filesystem::temp_directory_path() / ("chase-save-test-" + std::to_string(::getpid()));
        std::filesystem::remove_all(dir);
        std::string path = (dir / "nested" / "savegame.dat").string();
        LoadResult missing = load(path);
        require(!missing.found && !missing.image, "missing file");

        store(path, image);
        require(std::filesystem::file_size(path) == kFileSize, "stored size");
        LoadResult loaded = load(path);
        require(loaded.found && loaded.status == Status::Ok && loaded.image->bytes() == image.bytes(), "round trip");
        int entries = 0;
        for (auto& entry : std::filesystem::directory_iterator(dir / "nested")) entries += entry.is_regular_file();
        require(entries == 1, "temporary file left behind");

        SaveImage unsealed = image;
        unsealed.options()[2] = 1;
        store(path, unsealed);
        require(load(path).status == Status::Ok, "store seals");

        writeFile(path, Bytes(100, 0));
        LoadResult truncated = load(path);
        require(truncated.found && truncated.status == Status::Unreadable && !truncated.image, "short file");

        ::setenv("XDG_DATA_HOME", "/xdg", 1);
        require(defaultSavePath() == "/xdg/chase/savegame.dat", "XDG path");
        ::setenv("XDG_DATA_HOME", "relative", 1);
        ::setenv("HOME", "/home/u", 1);
        require(defaultSavePath() == "/home/u/.local/share/chase/savegame.dat", "relative XDG ignored");
        ::unsetenv("XDG_DATA_HOME");
        require(defaultSavePath() == "/home/u/.local/share/chase/savegame.dat", "HOME fallback");

        std::filesystem::remove_all(dir);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "save tests passed\n";
    return 0;
}

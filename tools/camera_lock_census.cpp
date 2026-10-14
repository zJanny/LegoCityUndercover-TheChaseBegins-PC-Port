// Per level and room: how many walkable navigation cells select the record pose camera (the camera is locked there).
// A cell is a point at the fitted floor height + 0.1 of every cell of the room's .bfnnav grids whose flag bit 0 is clear.
// The selection is the query 0x1d34ac of runtime::queryCameraRecords (no hysteresis of the per-frame decision).
// usage: camera-lock-census FIB NAMES LEVEL.blvl...
#include "assets/assets.hpp"
#include "runtime/camera_records.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>

using namespace chase;

namespace {
std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: camera-lock-census FIB NAMES LEVEL.blvl...\n";
        return 2;
    }
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    for (int arg = 3; arg < argc; ++arg) {
        const std::string levelFile = argv[arg];
        const assets::FuseEntry* entry = archive.find(levelFile);
        if (!entry) {
            std::cerr << "not in archive: " << levelFile << '\n';
            return 1;
        }
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        std::string camText;
        const std::string base = levelFile.substr(0, levelFile.size() - 5);
        if (const assets::FuseEntry* cam = archive.find(base + ".cam")) {
            const assets::Bytes bytes = archive.read(*cam);
            camText.assign(bytes.begin(), bytes.end());
        }
        const runtime::CameraRecords records(level, camText);
        std::map<std::string, int> recordsInRoom;
        for (const assets::LevelBound& bound : level.bounds)
            if (bound.name.rfind("Camera", 0) == 0 && bound.name.find("LookAt") == std::string::npos) ++recordsInRoom[lower(bound.room)];
        std::size_t total = 0, locked = 0;
        std::cout << levelFile << ": " << records.records().size() << " camera records, " << level.navRooms.size() << " nav rooms\n";
        for (const assets::NavRoom& room : level.navRooms) {
            const assets::FuseEntry* navEntry = archive.find(base + "_" + room.room + ".bfnnav");
            if (!navEntry) continue;
            const assets::NavMesh nav = assets::parseNav(archive.read(*navEntry));
            std::size_t cells = 0, roomLocked = 0;
            for (const assets::NavGrid& grid : nav.grids) {
                const float y = room.origin[1] + float(grid.height) * room.heightUnit / 256.0f + 0.1f;
                for (std::size_t gz = 0; gz < grid.depth; ++gz)
                    for (std::size_t gx = 0; gx < grid.width; ++gx) {
                        if (grid.cells[gz * grid.width + gx] & 1) continue;
                        const float point[3] = {room.origin[0] + room.cellSize * (float(grid.x + gx) + 0.5f), y,
                                                room.origin[2] + room.cellSize * (float(grid.z + gz) + 0.5f)};
                        const runtime::CameraRecordSet set = runtime::queryCameraRecords(records, point);
                        ++cells;
                        if (!(set.records.size() == 1 && set.records[0] == runtime::kRoomRecord)) ++roomLocked;
                    }
            }
            total += cells;
            locked += roomLocked;
            std::cout << "  " << room.room << ": " << cells << " walkable cells, " << roomLocked << " locked, " << recordsInRoom[lower(room.room)]
                      << " camera bounds in the room\n";
        }
        std::cout << "  total " << total << " cells, " << locked << " locked\n";
    }
    return 0;
}

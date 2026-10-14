#include "runtime/nav_load.hpp"

#include <cctype>
#include <stdexcept>

namespace chase::runtime {

std::string navFilePath(const std::string& levelPath, const std::string& room) {
    std::string name = room;
    for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const std::string suffix = ".blvl";
    const bool hasSuffix = levelPath.size() >= suffix.size() && levelPath.compare(levelPath.size() - suffix.size(), suffix.size(), suffix) == 0;
    return (hasSuffix ? levelPath.substr(0, levelPath.size() - suffix.size()) : levelPath) + "_" + name + ".bfnnav";
}

void navNormalizeLoaded(assets::NavMesh& mesh) {
    for (assets::NavGrid& grid : mesh.grids) {
        for (std::uint8_t& cell : grid.cells)
            if ((cell & 3) == 3) cell = static_cast<std::uint8_t>(cell & ~3);
        grid.flags = static_cast<std::uint16_t>(grid.flags & ~0x4000);
    }
}

std::vector<NavWorld> loadNavWorlds(assets::FuseArchive& archive, const std::string& levelPath, const assets::Level& level) {
    std::vector<assets::NavMesh> meshes;
    for (const assets::NavRoom& room : level.navRooms) {
        const std::string path = navFilePath(levelPath, room.room);
        const assets::FuseEntry* entry = archive.find(path);
        if (!entry) throw std::runtime_error("nav file not in archive: " + path);
        meshes.push_back(assets::parseNav(archive.read(*entry)));
        navNormalizeLoaded(meshes.back());
    }
    return navWorldsFromLevel(level.navRooms, meshes);
}

}

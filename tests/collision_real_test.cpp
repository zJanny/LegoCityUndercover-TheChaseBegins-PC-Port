// Differential test of the player collision (ground height and wall sliding) against traces recorded in
// the original game (Azahar oracle, tools/oracle/coll-*.txt and loco-*.txt; docs/re/collision.md).
// Needs the extracted RomFS and the local trace directories; exits with 77 (skipped) when one is missing.
#include "assets/assets.hpp"
#include "runtime/collision.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::runtime;
namespace fs = std::filesystem;

namespace {
// gameplay-start: the player stands in the street of lvl_01_cherrytreehills.
constexpr const char* kLevelModel = "models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl";
constexpr std::size_t kRecord = 4 + 0x100 + 0x340 + 0xb0;

struct Row {
    float dt = 0, x = 0, y = 0, z = 0, forwardX = 0, forwardZ = 0;
};

template <typename T> T read(const std::vector<char>& data, std::size_t offset) {
    T value;
    std::memcpy(&value, data.data() + offset, sizeof value);
    return value;
}

std::vector<Row> loadCsv(const fs::path& file) {
    std::ifstream csv(file);
    if (!csv) throw std::runtime_error("cannot open " + file.string());
    std::string line;
    std::getline(csv, line);
    std::vector<Row> rows;
    while (std::getline(csv, line)) {
        std::vector<std::string> fields;
        std::stringstream stream(line);
        std::string field;
        while (std::getline(stream, field, ',')) fields.push_back(field);
        Row row;
        row.dt = std::stof(fields[1]);
        row.x = std::stof(fields[5]);
        row.y = std::stof(fields[6]);
        row.z = std::stof(fields[7]);
        row.forwardX = std::stof(fields[8]);
        row.forwardZ = std::stof(fields[9]);
        rows.push_back(row);
    }
    return rows;
}

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct GroundTotals {
    std::size_t rows = 0, wrong = 0, changed = 0, edges = 0;
    float worst = 0;
};

// The ground height of the update that produced row i + 1: the probe sits at the new position with the
// old height, and the recorded height of row i + 1 is the answer (every row starts from the recorded state).
void checkGround(const CollisionWorld& world, const std::vector<Row>& rows, const std::string& name, GroundTotals& totals) {
    for (std::size_t i = 0; i + 1 < rows.size(); ++i) {
        const Row& before = rows[i];
        const Row& after = rows[i + 1];
        GroundProbe probe;
        probe.x = after.x;
        probe.y = before.y;
        probe.z = after.z;
        probe.forwardX = after.forwardX;
        probe.forwardZ = after.forwardZ;
        probe.rootDelta = std::hypot(after.x - before.x, after.z - before.z);
        probe.dt = after.dt;
        const bool moving = probe.rootDelta > 0;  // 0x20cd30 skips the probe for a character standing on the ground
        const float predicted = moving ? groundHeight(world, probe).value_or(before.y) : before.y;
        const float error = std::fabs(predicted - after.y);
        ++totals.rows;
        if (std::fabs(after.y - before.y) > 1e-6f) ++totals.changed;
        totals.worst = std::max(totals.worst, error);
        if (error <= 2e-4f) continue;
        // The movement is the difference of two float32 positions near 900 (ulp 6e-5..1.2e-4); the box height
        // changes in steps, so a movement that close to a step edge may fall either way. Accept a row when a
        // movement within 2e-4 reproduces the recorded height, and count it.
        bool edge = false;
        for (const float shift : {-2e-4f, 2e-4f}) {
            GroundProbe moved = probe;
            moved.rootDelta += shift;
            edge = edge || std::fabs(groundHeight(world, moved).value_or(before.y) - after.y) <= 2e-4f;
        }
        if (edge) {
            ++totals.edges;
            continue;
        }
        ++totals.wrong;
        std::cerr << name << " row " << i + 1 << ": predicted " << predicted << ", recorded " << after.y << '\n';
    }
}

struct WallTotals {
    std::size_t rows = 0, wrong = 0, slides = 0, edges = 0;
    float worst = 0;
};

// Memory traces: u32 frame, entity (0x100), controller (0x340), node (0xb0) per row. Controller +0x224 is the
// movement of the update before collision, +0x21c / +0x220 what the slide added to x and z; the node
// position is after the update. The test replays the slide from the position before the update.
void checkWalls(const CollisionWorld& world, const fs::path& mem, std::size_t rowCount, WallTotals& totals) {
    std::ifstream file(mem, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    require(data.size() == rowCount * kRecord, "unexpected trace record size: " + mem.string());
    for (std::size_t i = 1; i < rowCount; ++i) {
        const std::size_t controller = i * kRecord + 4 + 0x100, node = controller + 0x340;
        const float inX = read<float>(data, controller + 0x224), inZ = read<float>(data, controller + 0x22c);
        const float correctionX = read<float>(data, controller + 0x21c), correctionZ = read<float>(data, controller + 0x220);
        const float x = read<float>(data, node + 0x8c), y = read<float>(data, node + 0x90), z = read<float>(data, node + 0x94);
        const float forwardX = read<float>(data, node + 0x7c), forwardZ = read<float>(data, node + 0x84);
        const float startX = x - (inX + correctionX), startZ = z - (inZ + correctionZ);
        auto correctionError = [&](float shiftX, float shiftZ, float grow, int& slides) {
            float dx = inX, dz = inZ;
            PlayerBox box;
            box.half[0] += grow;
            box.half[2] += grow;
            slides = slideAlongWalls(world, startX + shiftX, y, startZ + shiftZ, forwardX, forwardZ, dx, dz, box);
            return std::max(std::fabs(dx - inX - correctionX), std::fabs(dz - inZ - correctionZ));
        };
        int slides = 0;
        const float error = correctionError(0, 0, 0, slides);
        ++totals.rows;
        if (std::fabs(correctionX) + std::fabs(correctionZ) > 1e-6f) ++totals.slides;
        totals.worst = std::max(totals.worst, error);
        if (error <= 1e-3f) continue;
        // The start position is recovered from two float32 positions near 900 (ulp up to 1.2e-4), and the original
        // transforms the triangles by a matrix product whose rounding is not replicated, so a box that just
        // touches a triangle may be decided either way. Accept a row that a start position within 3e-4 or a
        // box larger or smaller by 5e-3 reproduces, or whose result is within 5 cm of the recorded one, and count it.
        bool edge = false;
        for (const float shiftX : {-3e-4f, 0.0f, 3e-4f})
            for (const float shiftZ : {-3e-4f, 0.0f, 3e-4f})
                for (const float grow : {-5e-3f, 0.0f, 5e-3f}) {
                    int unused = 0;
                    edge = edge || correctionError(shiftX, shiftZ, grow, unused) <= 1e-3f;
                }
        // A contact decided differently at the last millimetres moves the result by up to a few centimetres.
        edge = edge || error <= 0.05f;
        if (edge) {
            ++totals.edges;
            continue;
        }
        ++totals.wrong;
        std::cerr << mem.filename().string() << " row " << i << ": predicted error " << error << " (" << slides
                  << " slides), recorded correction " << correctionX << ' ' << correctionZ << '\n';
    }
}
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: collision-real-test FIB NAMES LOCO_TRACE_DIR COLLISION_TRACE_DIR\n";
        return 2;
    }
    const fs::path fib = argv[1], names = argv[2], locoDirectory = argv[3], collisionDirectory = argv[4];
    if (!fs::exists(fib) || !fs::exists(names) || !fs::exists(locoDirectory) || !fs::exists(collisionDirectory)) {
        std::cout << "collision-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(fib.string());
        archive.loadNames(names.string());
        const chase::assets::FuseEntry* entry = archive.find(kLevelModel);
        require(entry != nullptr, std::string("not in archive: ") + kLevelModel);
        const chase::assets::Model model = chase::assets::parseModel(archive.read(*entry));
        CollisionWorld world;
        for (const auto& item : model.collision) world.add(item);
        require(model.collision.size() == 153 && world.triangleCount() == 41853, "level collision totals changed");

        GroundTotals ground;
        std::size_t groundTraces = 0;
        for (const fs::path& directory : {locoDirectory, collisionDirectory}) {
            std::vector<fs::path> files;
            for (const auto& item : fs::directory_iterator(directory))
                if (item.path().extension() == ".csv") files.push_back(item.path());
            std::sort(files.begin(), files.end());
            for (const fs::path& file : files) {
                const std::vector<Row> rows = loadCsv(file);
                if (rows.size() < 2) continue;
                checkGround(world, rows, file.filename().string(), ground);
                ++groundTraces;
            }
        }
        std::cout << "ground: " << groundTraces << " traces, " << ground.rows << " updates, " << ground.changed
                  << " with a height change, " << ground.edges << " at a step edge, " << ground.wrong << " wrong, worst error " << ground.worst << '\n';
        require(groundTraces >= 90 && ground.changed >= 200, "too little recorded ground data");
        require(ground.wrong == 0 && ground.edges <= 3, "ground height differs from the recorded traces");

        WallTotals walls;
        std::size_t wallTraces = 0;
        for (const auto& item : fs::directory_iterator(collisionDirectory)) {
            const fs::path mem = item.path();
            if (mem.extension() != ".mem") continue;
            fs::path csv = mem;
            csv.replace_extension(".csv");
            if (!fs::exists(csv)) continue;
            const std::size_t rowCount = loadCsv(csv).size();
            if (rowCount == 0 || fs::file_size(mem) != rowCount * kRecord) continue;
            checkWalls(world, mem, rowCount, walls);
            ++wallTraces;
        }
        std::cout << "walls: " << wallTraces << " traces, " << walls.rows << " updates, " << walls.slides
                  << " with a slide, " << walls.edges << " at a contact edge, " << walls.wrong << " wrong, worst error " << walls.worst << '\n';
        require(walls.slides >= 250, "too little recorded wall data");
        require(walls.wrong == 0 && walls.edges <= 6, "wall slide differs from the recorded traces");
    } catch (const std::exception& error) {
        std::cerr << "collision-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "collision-real-test: ok\n";
    return 0;
}

// Differential test of falling and landing against traces recorded in the original game (Azahar oracle,
// tools/oracle/fall-*.txt; docs/re/player.md falling entry). Needs the extracted RomFS and the local trace
// directory; exits with 77 (skipped) when either is missing.
#include "assets/assets.hpp"
#include "runtime/player_vertical.hpp"
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
constexpr const char* kLevelModel = "models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl";
constexpr std::size_t kRecord = 4 + 0x100 + 0x340 + 0xb0;

constexpr std::uint16_t kStateIdle = 1, kStateRun = 2, kStateWalk = 3, kStateFall = 5, kStateSoftLanding = 6, kStateHardLanding = 7,
                        kStateStepOff = 8;

struct Row {
    float dt = 0, x = 0, y = 0, z = 0, forwardX = 0, forwardZ = 0;
    std::uint16_t state = 0, held = 0;
    float fallDistance = 0, fallSpeed = 0;
};

template <typename T> T read(const std::vector<char>& data, std::size_t offset) {
    T value;
    std::memcpy(&value, data.data() + offset, sizeof value);
    return value;
}

std::vector<Row> loadTrace(const fs::path& csvFile) {
    std::ifstream csv(csvFile);
    if (!csv) throw std::runtime_error("cannot open " + csvFile.string());
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
    fs::path memFile = csvFile;
    memFile.replace_extension(".mem");
    std::ifstream file(memFile, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.size() != rows.size() * kRecord) throw std::runtime_error("unexpected trace record size: " + memFile.string());
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const std::size_t controller = i * kRecord + 4 + 0x100;
        rows[i].state = read<std::uint16_t>(data, controller + 0x6c);
        rows[i].held = read<std::uint16_t>(data, controller + 0xc);
        rows[i].fallDistance = read<float>(data, controller + 0x2b4);
        rows[i].fallSpeed = read<float>(data, controller + 0x2b8);
    }
    return rows;
}

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct Totals {
    std::size_t traces = 0, updates = 0, starts = 0, landings = 0, hardLandings = 0, softStuns = 0, stunChecks = 0, stepOffs = 0, wrong = 0;
    float worstHeight = 0, worstSpeed = 0;
};

bool airborne(std::uint16_t state) { return state == kStateFall; }
bool grounded(std::uint16_t state) { return state == kStateIdle || state == kStateRun || state == kStateWalk; }

void checkUpdates(const CollisionWorld& world, const std::vector<Row>& rows, const std::string& name, Totals& totals) {
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const Row& before = rows[i - 1];
        const Row& after = rows[i];
        if (after.state == kStateStepOff) {
            ++totals.stepOffs;
            break;
        }
        const bool wasAirborne = airborne(before.state);
        if (!wasAirborne && !grounded(before.state)) continue;
        VerticalState state;
        state.posture = wasAirborne ? Posture::Falling : Posture::Ground;
        state.y = before.y;
        state.fallSpeed = wasAirborne ? before.fallSpeed : 0;
        state.fallDistance = wasAirborne ? before.fallDistance : 0;
        VerticalInput input;
        input.x = after.x;
        input.z = after.z;
        input.forwardX = after.forwardX;
        input.forwardZ = after.forwardZ;
        input.rootDelta = std::hypot(after.x - before.x, after.z - before.z);
        input.dt = after.dt;
        input.moveRequested = (after.held & 1) != 0;
        const VerticalStep step = verticalUpdate(state, world, input);
        ++totals.updates;

        const bool expectFall = airborne(after.state);
        const bool expectLanding = after.state == kStateSoftLanding || after.state == kStateHardLanding;
        const bool landed = step.landed;
        const bool expectLanded = wasAirborne && !expectFall;
        const float heightError = std::fabs(state.y - after.y);
        const float speedError = expectFall ? std::fabs(state.fallSpeed - after.fallSpeed) : 0;
        totals.worstHeight = std::max(totals.worstHeight, heightError);
        totals.worstSpeed = std::max(totals.worstSpeed, speedError);
        if (!wasAirborne && expectFall) ++totals.starts;
        if (expectLanded) {
            ++totals.landings;
            if (after.state == kStateHardLanding) ++totals.hardLandings;
            if (after.state == kStateSoftLanding) ++totals.softStuns;
        }
        bool wrong = (state.posture == Posture::Falling) != expectFall || landed != expectLanded || heightError > 2e-4f || speedError > 1e-4f;
        if (expectFall && std::fabs(state.fallDistance - after.fallDistance) > 1e-3f) wrong = true;
        if (expectLanded && (step.hard != (after.state == kStateHardLanding) || (state.posture == Posture::Landing) != expectLanding)) wrong = true;
        if (!wrong) continue;
        ++totals.wrong;
        std::cerr << name << " row " << i << ": predicted y " << state.y << " speed " << state.fallSpeed << " posture "
                  << int(state.posture) << ", recorded y " << after.y << " speed " << after.fallSpeed << " state " << after.state << '\n';
    }
}

void checkStun(const std::vector<Row>& rows, const std::string& name, Totals& totals) {
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const bool soft = rows[i].state == kStateSoftLanding, hard = rows[i].state == kStateHardLanding;
        if (!((soft || hard) && airborne(rows[i - 1].state))) continue;
        std::size_t end = i;
        while (end < rows.size() && rows[end].state == rows[i].state) ++end;
        if (end >= rows.size() || rows[end].state != kStateIdle) continue;
        VerticalState state;
        state.posture = Posture::Landing;
        state.landingDuration = hard ? kHardLandingSeconds : kSoftLandingSeconds;
        VerticalInput input;
        input.dt = rows[i].dt;
        std::size_t updates = 1;
        while (true) {
            const VerticalStep step = verticalUpdate(state, CollisionWorld(), input);
            if (step.stunEnded) break;
            ++updates;
        }
        ++totals.stunChecks;
        if (updates != end - i) {
            ++totals.wrong;
            std::cerr << name << " stun from row " << i << ": predicted " << updates << " updates, recorded " << end - i << '\n';
        }
    }
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: fall-real-test FIB NAMES FALL_TRACE_DIR\n";
        return 2;
    }
    const fs::path fib = argv[1], names = argv[2], directory = argv[3];
    if (!fs::exists(fib) || !fs::exists(names) || !fs::exists(directory)) {
        std::cout << "fall-real-test: data not available, skipped\n";
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

        std::vector<fs::path> files;
        for (const auto& item : fs::directory_iterator(directory))
            if (item.path().extension() == ".csv") files.push_back(item.path());
        std::sort(files.begin(), files.end());
        Totals totals;
        for (const fs::path& file : files) {
            const std::vector<Row> rows = loadTrace(file);
            checkUpdates(world, rows, file.filename().string(), totals);
            checkStun(rows, file.filename().string(), totals);
            ++totals.traces;
        }
        std::cout << "falls: " << totals.traces << " traces, " << totals.updates << " updates, " << totals.starts << " fall starts, "
                  << totals.landings << " landings (" << totals.hardLandings << " hard, " << totals.softStuns << " soft with stun), "
                  << totals.stunChecks << " stun lengths, " << totals.stepOffs << " traces cut at the step-off state, " << totals.wrong << " wrong, worst height error " << totals.worstHeight
                  << ", worst fall speed error " << totals.worstSpeed << '\n';
        require(totals.traces >= 12 && totals.landings >= 8 && totals.hardLandings >= 3 && totals.starts >= 1,
                "too little recorded fall data");
        require(totals.wrong == 0, "fall or landing differs from the recorded traces");
    } catch (const std::exception& error) {
        std::cerr << "fall-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "fall-real-test: ok\n";
    return 0;
}

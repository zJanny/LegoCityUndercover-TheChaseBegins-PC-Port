// Differential test of the player locomotion step against rows recorded in the original game
// (Azahar oracle, tools/oracle/loco-*.txt; docs/re/player-locomotion.md; provenance in data/locomotion_traces.inc).
// Needs the extracted RomFS; exits with 77 (skipped) when it is missing.
#include "assets/assets.hpp"
#include "runtime/pad.hpp"
#include "runtime/player_locomotion.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::runtime;
namespace fs = std::filesystem;

namespace {
// Recorded row: the state after one game update, with the stick that was injected for it.
struct Row {
    float dt = 0;
    int stickX = 0, stickY = 0;
    float x = 0, z = 0;
    float cameraYaw = 0;
    std::int16_t intent = 0, facing = 0, lastIntent = 0;
    std::uint16_t flags = 0;
    float animationScale = 1.0f;
    float runTimer = -1.0f;  // -1: the trace has no run timer
};

struct RecordedTrace {
    const char* name;
    std::vector<Row> rows;
};

const std::vector<RecordedTrace> kTraces = {
#include "data/locomotion_traces.inc"
};

const std::vector<Row>& loadTrace(const std::string& name) {
    for (const RecordedTrace& trace : kTraces)
        if (name == trace.name) return trace.rows;
    throw std::runtime_error("no recorded trace " + name);
}

LocomotionState initialState(const Row& row) {
    LocomotionState state;
    state.x = row.x;
    state.z = row.z;
    state.facing = row.facing;
    state.lastIntent = row.lastIntent;
    state.wasRunning = (row.flags & kHeldMove) && !(row.flags & kHeldSlow);
    state.sprinting = row.animationScale > 1.2f;
    // Traces without the run timer start either right after the first update or deep into a run.
    state.runTimer = row.runTimer >= 0 ? row.runTimer : (state.sprinting ? 2.0f : (state.wasRunning ? row.dt : 0.0f));
    return state;
}

StickIntent recordedIntent(const Row& row) {
    StickIntent intent;
    intent.active = (row.flags & kHeldMove) != 0;
    intent.heldSet = row.flags & (kHeldMove | kHeldSlow);
    intent.heading = row.lastIntent;
    return intent;
}

// Tolerances. Positions are float32 at 870..1110, where one ulp is 6.1e-5..1.2e-4, so the difference
// of two positions carries up to about 1.2e-4; the measured speeds are known to +-0.0003 u/s
// (+-1e-5 u per update at dt 0.033). 2e-4 per update leaves headroom for both without hiding a wrong
// regime (the smallest regime difference per update is 0.05 u). The drift limit covers 45..75 updates.
constexpr float kDeltaTolerance = 2e-4f;
constexpr float kDriftTolerance = 0.01f;
constexpr float kTimerTolerance = 1e-5f;
// ctrl+0x94 shows 0.99999994 and 1.4999 on some rows (slope term of 0x1abe60 on nearly flat ground); the step models 1.0 / 1.5.
constexpr float kScaleTolerance = 2e-3f;

// The original lowers the playback scale (and with it the speed) on slopes (0x1abe60, slope term); the
// player climbed a ramp in turn_run_r90, scale 1.33 at 9.15 u/s. Ground height is not modelled, so rows
// whose scale is not the flat-ground value (1, or 1.5 sprinting) are not compared.
bool slopeAffected(const Row& row) {
    const float scale = row.animationScale;
    return std::fabs(scale - 1.0f) > kScaleTolerance && std::fabs(scale - 1.5f) > kScaleTolerance;
}

struct Totals {
    std::size_t updates = 0, facingResyncs = 0, slopeRows = 0;
    float maxDeltaError = 0, maxDrift = 0, maxTimerError = 0;
};



void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void checkTrace(const std::string& name, Totals& totals) {
    const std::vector<Row>& rows = loadTrace(name);
    LocomotionState state = initialState(rows[0]);
    float startX = rows[0].x, startZ = rows[0].z;
    LocomotionState driftState = state;
    bool sloped = false;
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const Row& previous = rows[i - 1];
        const Row& row = rows[i];
        // One update in each of the three rest turns (row 25, frame 59) moves the player but leaves the
        // facing untouched (docs/re/player.md, "no update at row 59"); in turn_rest_r90 and _l90 the latched
        // intent also lags the new one (ctrl+6 differs from ctrl+0xa). The facing is resynchronised there.
        const bool intentNotLatched = row.intent != 0 && row.intent != row.lastIntent;
        StickIntent intent = recordedIntent(row);
        if (intentNotLatched) intent.heading = previous.lastIntent;
        else if (intent.active) intent.heading = row.intent;

        if (slopeAffected(previous) || slopeAffected(row)) {
            state = initialState(row);
            ++totals.slopeRows;
            sloped = true;
            continue;
        }
        LocomotionState local = state;
        local.x = previous.x;
        local.z = previous.z;
        LocomotionStep step = locomotionUpdate(local, row.dt, intent);
        LocomotionStep driftStep = locomotionUpdate(driftState, row.dt, intent);
        (void)driftStep;

        const float errorX = std::fabs(step.deltaX - (row.x - previous.x));
        const float errorZ = std::fabs(step.deltaZ - (row.z - previous.z));
        totals.maxDeltaError = std::max({totals.maxDeltaError, errorX, errorZ});
        require(errorX < kDeltaTolerance && errorZ < kDeltaTolerance,
                name + " row " + std::to_string(i) + ": movement differs by " + std::to_string(errorX) + ", " + std::to_string(errorZ));
        if (row.facing == previous.facing && local.facing != row.facing) {
            ++totals.facingResyncs;
            local.facing = row.facing;
            driftState.facing = row.facing;
        }
        require(local.facing == row.facing, name + " row " + std::to_string(i) + ": facing " +
                std::to_string(local.facing) + " expected " + std::to_string(row.facing));
        require(std::fabs(step.animationScale - row.animationScale) < kScaleTolerance, name + " row " + std::to_string(i) + ": animation scale");
        if (row.runTimer >= 0) {
            const float timerError = std::fabs(local.runTimer - row.runTimer);
            totals.maxTimerError = std::max(totals.maxTimerError, timerError);
            require(timerError < kTimerTolerance, name + " row " + std::to_string(i) + ": run timer " +
                    std::to_string(local.runTimer) + " expected " + std::to_string(row.runTimer));
        }
        state = local;
        ++totals.updates;
    }
    if (sloped) return;
    const float driftX = std::fabs((driftState.x - startX) - (rows.back().x - startX));
    const float driftZ = std::fabs((driftState.z - startZ) - (rows.back().z - startZ));
    totals.maxDrift = std::max({totals.maxDrift, driftX, driftZ});
    require(driftX < kDriftTolerance && driftZ < kDriftTolerance, name + ": accumulated position drifts by " + std::to_string(driftX) + ", " + std::to_string(driftZ));
}

// Raw stick and camera yaw through the pad pipeline and stickIntent, against the recorded heading. The
// player update reads the camera yaw before the camera moves in the same update: the yaw of the previous
// row reproduces the heading to one unit, the yaw of the row itself (camera turning 0.03..0.045 rad per
// update in the traces) is off by up to 1280 units.
float headingError(const Row& row, float yaw) {
    PadDevice pad;
    PadState state;
    state.stickX = std::int16_t(row.stickX);
    state.stickY = std::int16_t(row.stickY);
    pad.poll(state);
    StickIntentInput input;
    input.slot14 = {pad.slot(14).value, pad.slot(14).deadzone};
    input.slot15 = {pad.slot(15).value, pad.slot(15).deadzone};
    input.entityType = 0x100;
    input.cameraYaw = yaw;
    const StickIntent intent = stickIntent(input);
    return float(std::abs(int(wrapHeading(int(intent.heading) - int(row.intent)))));
}

void checkRootMotionData(const std::string& fibPath, const std::string& namesPath) {
    using namespace chase::assets;
    FuseArchive archive(fibPath);
    archive.loadNames(namesPath);
    for (const char* name : {"walk", "run", "sprint"}) {
        const std::string path = std::string("models/char_minifig/class_police/") + name + "_minifig.bfnanm";
        const FuseEntry* entry = archive.find(path);
        require(entry != nullptr, "not in archive: " + path);
        const Animation animation = parseAnimation(archive.read(*entry));
        require(animation.longCurve == 0 && animation.jointCurves.size() == 21, path + ": curve layout");
        const JointChannels first = decodeChannels(animation.jointCurves[0], 0, animation.frames + 1);
        const JointChannels last = decodeChannels(animation.jointCurves[0], animation.frames, animation.frames + 1);
        // Pelvis translation x/z returns to its start over the loop: the joint curves carry no net travel.
        require(std::fabs(first.values[9] - last.values[9]) < 1e-3f && std::fabs(first.values[11] - last.values[11]) < 1e-3f,
                path + ": root joint travels over the loop");
        require(animation.subAnimations.size() == 2, path + ": sub-animations");
        for (const AnimTrack& track : animation.subAnimations[1])
            if (track.hash >= 0xdcd633e0u && track.hash <= 0xdcd633e2u)
                for (const CurveKey& key : track.keys) require(key.value == 0.0f, path + ": root translation track not zero");
    }
}
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: locomotion-real-test FIB NAMES.csv\n";
        return 2;
    }
    const std::string fib = argv[1], names = argv[2];
    if (!fs::exists(fib) || !fs::exists(names)) {
        std::cout << "skipped: RomFS not found\n";
        return 77;
    }
    try {
        Totals totals;
        for (const RecordedTrace& trace : kTraces) checkTrace(trace.name, totals);

        float maxHeadingError = 0, maxPreviousYawError = 0;
        std::size_t headingRows = 0;
        for (const std::string name : {"turn_rest_r90", "turn_rest_l90", "turn_rest_180", "turn_run_r90", "turn_run_180", "turn_walk_r90", "mag156_push"}) {
            const std::vector<Row>& rows = loadTrace(name);
            for (std::size_t i = 1; i < rows.size(); ++i) {
                if (!(rows[i].flags & kHeldMove)) continue;
                maxHeadingError = std::max(maxHeadingError, headingError(rows[i], rows[i].cameraYaw));
                maxPreviousYawError = std::max(maxPreviousYawError, headingError(rows[i], rows[i - 1].cameraYaw));
                ++headingRows;
            }
        }
        std::cout << totals.updates << " updates over " << kTraces.size() << " traces: max movement error per update "
                  << totals.maxDeltaError << " u, max accumulated drift " << totals.maxDrift << " u, max run timer error "
                  << totals.maxTimerError << ", facing resyncs " << totals.facingResyncs << ", slope rows skipped " << totals.slopeRows << "; stick-to-heading over "
                  << headingRows << " rows: max error " << maxPreviousYawError << " units (with the yaw of the row itself: " << maxHeadingError << ")\n";
        require(totals.facingResyncs <= 3, "more unlatched rows than the three known ones");
        require(maxPreviousYawError <= 2, "stick-to-heading error");
        checkRootMotionData(fib, names);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}

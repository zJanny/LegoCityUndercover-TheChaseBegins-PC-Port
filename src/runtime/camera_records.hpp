#pragma once
// Camera records of a level and the record pose camera, recovered from the loader 0x3062e4, the record query
// 0x1d34ac (volume filter 0x20bf6c, overlap dedupe 0x1aa734), the per-frame update 0x1b4a30, the blend set-up 0x19b8c8
// and the pose blend 0x298cd8; docs/re/camera.md and docs/re/data-tables-config.md. No SDL dependency.
//
// A record is made for every level bound whose name starts with "Camera" and does not contain "LookAt". The `.cam`
// file gives it a pose (Type bit set, Location, Offset, FOV, ...). While the player stands in no record volume the
// level's Room record is the only record and the follow controller 0x2a8bfc runs; in every other case the camera is the
// record pose 0x298cd8 and the shoulder buttons do not turn it.
//
// The pose of a Blocker record is the last camera pose (camera_blocker.hpp).
//
// Not recovered: the order in which the spatial structure at 0x598300 returns the records (level order is used), the
// shake (0x1c607c), the second follow target, the byte 0x3770c8 and the counter 0x3770e0 (both read as 0) and movement of the
// matrices that carry bounds (stand-in CAMERA-S4: static).
#include "assets/assets.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

struct CameraRecord {
    std::string name;
    bool room = false;                  // +0x4e
    bool focus = false;                 // +0x4a: the bound's name contains "Focus"
    bool blocker = false;               // +0x4b: the bound's name contains "Blocker"
    std::uint16_t type = 0;             // +0x44: atoi(Type) | 0x80 once a .cam block set it
    float location[3] = {0, 0, 0};      // +0x24
    std::int8_t offset[3] = {0, 0, 0};  // +0x46: integer part of Offset
    std::uint8_t fov = 43;              // +0x49
    float blendTime = 0.85f;            // +0x30, clamped to [0, 20]
    float offset3d = 5.0f;              // +0x40
    std::uint8_t filters[2] = {32, 28}; // +0x4c / +0x4d, clamped to 50
    // The volume as 0x1e9290 leaves it: a bound without a node (255) keeps its centre; a bound with a node moves the
    // centre into the translation of the node matrix and keeps the centre at zero. Shapes 2 and 4 copy the radius +0x1c
    // into the extents +0x20 and +0x24.
    std::uint8_t shape = 3;
    bool enabled = true;                // bound +0x0a
    std::size_t boundIndex = 0;         // index into Level::bounds
    float radius = 0;                   // bound +0x0c
    float centre[3] = {0, 0, 0};        // +0x10
    float extent[3] = {0, 0, 0};        // +0x1c, +0x20, +0x24
    bool hasMatrix = false;
    std::array<float, 16> matrix{};     // the node matrix after the centre was added to its translation
    float parentOffset[3] = {0, 0, 0};  // node translation (+0x30..) that the overlap dedupe 0x1aa734 adds to the centre
};

class CameraRecords {
public:
    // `camText` is the text of the level's `.cam` file, empty when the level has none.
    CameraRecords(const assets::Level& level, const std::string& camText);
    const std::vector<CameraRecord>& records() const { return records_; }
    std::vector<CameraRecord>& records() { return records_; }
    // Bound byte +0x0a per Level::bounds entry (EnableBound 0x1d0ce8 writes it; the volume filter 0x20bf6c skips a
    // record whose bound byte is 0).
    void setBoundsEnabled(const std::vector<std::uint8_t>& enabled);

private:
    std::vector<CameraRecord> records_;
};

// Result of the record query 0x1d34ac: indices into CameraRecords (kRoomRecord for the level's Room record, which is
// chosen when no volume contains the point) and their weights.
constexpr int kRoomRecord = -1;
struct CameraRecordSet {
    std::vector<int> records;
    std::vector<float> weights;
    bool operator==(const CameraRecordSet& other) const { return records == other.records; }
};

CameraRecordSet queryCameraRecords(const CameraRecords& records, const float point[3]);

enum class CameraCommand { None, Follow, RecordPose };

// The decision that the default preset update 0x1b4a30 and 0x19b8c8 make every frame.
class CameraRecordSelector {
public:
    explicit CameraRecordSelector(const CameraRecords& records) : records_(&records) {}
    // `point` is the world position of the follow target (the player node).
    CameraCommand update(const float point[3]);
    CameraCommand command() const { return command_; }
    // The record set of the record pose (the blend block 0x58d694: records at +0x10, weights at +0x30).
    const CameraRecordSet& blend() const { return blend_; }
    const CameraRecordSet& previous() const { return previous_; }
    // True when the last update ran the hand-over 0x19b8c8, which pushes a new camera command.
    bool handedOver() const { return handedOver_; }

private:
    const CameraRecords* records_;
    CameraCommand command_ = CameraCommand::None;
    bool handedOver_ = false;
    CameraRecordSet previous_, blend_;
};

struct RecordPose {
    float look[3] = {0, 0, 0};  // out+0x00
    float eye[3] = {0, 0, 0};   // out+0x24
    float fovDegrees = 0;       // out+0x30
    float offset3d = 0;         // out+0x38
};

// 0x298cd8 for the blend block `set` and the follow target position `target`.
RecordPose evaluateRecordPose(const CameraRecords& records, const CameraRecordSet& set, const float target[3]);

// Fade time of a record pose command: the plain mean of the BlendTime of the records (0x19b8c8); 0 clears the command stack.
float recordFadeSeconds(const CameraRecords& records, const CameraRecordSet& set);
// Filter bytes (+0x4c eye, +0x4d look) of the first record of the set (0x19b8c8).
std::uint8_t recordEyeFilter(const CameraRecords& records, const CameraRecordSet& set);
std::uint8_t recordLookFilter(const CameraRecords& records, const CameraRecordSet& set);

// The look-at camera of a pose: matrix 0x2ca814 builds the view direction look - eye, the word 0x376894 is its yaw.
float poseYaw(const RecordPose& pose);
float posePitch(const RecordPose& pose);

}

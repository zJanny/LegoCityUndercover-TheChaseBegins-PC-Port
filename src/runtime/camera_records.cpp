#include "runtime/camera_records.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

bool startsWith(const std::string& text, const char* prefix) { return text.rfind(prefix, 0) == 0; }
bool contains(const std::string& text, const char* part) { return text.find(part) != std::string::npos; }

bool sameName(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}

float length3(const float v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

void applySetup(CameraRecord& record, const assets::CameraSetup& setup) {
    record.type = std::uint16_t(setup.type | 0x80);
    for (int a = 0; a < 3; ++a) {
        record.location[a] = float(setup.location[std::size_t(a)]);
        record.offset[a] = std::int8_t(int(setup.offset[std::size_t(a)]));
    }
    record.fov = std::uint8_t(int(setup.fov));
    record.blendTime = std::clamp(float(setup.blendTime), 0.0f, 20.0f);
    for (int a = 0; a < 2; ++a) record.filters[a] = std::uint8_t(std::clamp(int(setup.filters[std::size_t(a)]), 0, 50));
    record.offset3d = float(setup.offset3d);
}

bool inside(const CameraRecord& record, const float point[3]) {
    if (!record.enabled) return false;
    float local[3] = {point[0], point[1], point[2]};
    if (record.hasMatrix) {
        const std::array<float, 16>& m = record.matrix;
        const float dx = point[0] - m[12], dy = point[1] - m[13], dz = point[2] - m[14];
        local[2] = dx * m[8] + dy * m[9] + dz * m[10];
        local[0] = dx * m[0] + dy * m[1] + dz * m[2];
        local[1] = dx * m[4] + dy * m[5] + dz * m[6];
    }
    const float diff[3] = {local[0] - record.centre[0], local[1] - record.centre[1], local[2] - record.centre[2]};
    if (record.shape == 2) return !(length3(diff) > record.extent[0]);
    if (record.shape == 4 || record.shape == 6)
        return record.extent[0] > length3(diff) && record.extent[1] > std::fabs(local[1] - record.centre[1]);
    return !(diff[0] > record.extent[0] || diff[0] < -record.extent[0] || diff[1] > record.extent[1] || diff[1] < -record.extent[1] ||
             diff[2] > record.extent[2] || diff[2] < -record.extent[2]);
}

// 0x1aa734, including its skip of the element that moves into the removed place.
unsigned dropOverlaps(const std::vector<CameraRecord>& all, std::vector<int>& list) {
    unsigned count = unsigned(list.size());
    unsigned first = 0;
    while (count != 0 && first < count) {
        unsigned second = (first + 1) & 0xff;
        while (second < count) {
            const CameraRecord& a = all[std::size_t(list[first])];
            const CameraRecord& b = all[std::size_t(list[second])];
            float depth = 0;
            for (int axis = 0; axis < 3; ++axis) {
                const float overlap = (a.extent[axis] + b.extent[axis]) -
                                      std::fabs((a.centre[axis] + a.parentOffset[axis]) - (b.centre[axis] + b.parentOffset[axis]));
                depth = axis == 0 ? overlap : std::min(depth, overlap);
            }
            if (depth <= 0.5f) {
                if (second + 2 <= count) {
                    unsigned steps = (count - second - 1) & 0xff;
                    unsigned at = second;
                    do {
                        list[at] = list[at + 1];
                        ++at;
                        steps = (steps - 1) & 0xff;
                    } while (steps != 0);
                }
                --count;
            }
            second = (second + 1) & 0xff;
        }
        first = (first + 1) & 0xff;
    }
    list.resize(count);
    return count;
}

// 0x21c1e4 for the segment from the volume centre along `direction` (length twice the bounding radius): the point
// where its slab loop ends. The loop compares t = half / delta without a sign test, so a segment pointing to a negative
// face meets the opposite face.
bool segmentExit(const float centre[3], const float half[3], const float delta[3], float out[3]) {
    float best = 1.0f;
    bool found = false;
    for (int i = 0; i < 3; ++i) {
        const float t = half[i] / delta[i];
        if (best <= t) continue;
        const int j = i + 1 > 2 ? i - 2 : i + 1;
        const int k = i + 2 > 2 ? i - 1 : i + 2;
        const float cj = t * delta[j], ck = t * delta[k];
        if (cj < -half[j] || half[j] < cj) continue;
        if (ck < -half[k] || half[k] < ck) continue;
        out[i] = half[i] + centre[i];
        out[j] = cj + centre[j];
        out[k] = ck + centre[k];
        best = t;
        found = true;
    }
    return found;
}

float weightOf(const CameraRecord& record, const float point[3]) {
    float direction[3] = {point[0] - record.centre[0], 0.0f, point[2] - record.centre[2]};
    const float length = length3(direction);
    if (length > 0) {
        direction[0] /= length;
        direction[2] /= length;
    }
    const float reach = record.radius * 2.0f;
    float delta[3] = {direction[0] * reach, direction[1] * reach, direction[2] * reach};
    float hit[3] = {0, 0, 0};
    segmentExit(record.centre, record.extent, delta, hit);
    const float dx = hit[0] - point[0], dz = hit[2] - point[2];
    return std::sqrt(dx * dx + dz * dz);
}

}

CameraRecords::CameraRecords(const assets::Level& level, const std::string& camText) {
    std::vector<std::array<float, 16>> matrices = level.boundMatrices;
    std::vector<std::size_t> boundOf;
    for (std::size_t index = 0; index < level.bounds.size(); ++index) {
        const assets::LevelBound& bound = level.bounds[index];
        if (!startsWith(bound.name, "Camera") || contains(bound.name, "LookAt")) continue;
        CameraRecord record;
        record.name = bound.name;
        record.focus = contains(bound.name, "Focus");
        record.blocker = contains(bound.name, "Blocker");
        record.shape = bound.shape;
        record.enabled = bound.enabled != 0;
        record.radius = bound.extent;
        for (int a = 0; a < 3; ++a) {
            record.centre[a] = bound.centre[a];
            record.extent[a] = bound.half[a];
        }
        if (bound.node != 0xff && bound.node < matrices.size()) {
            std::array<float, 16>& m = matrices[bound.node];
            for (int a = 0; a < 3; ++a) {
                m[std::size_t(12 + a)] += bound.centre[a];
                record.centre[a] = 0;
            }
            record.hasMatrix = true;
            record.matrix = m;
        } else if (bound.shape == 2 || bound.shape == 4) {
            record.extent[1] = record.extent[2] = bound.half[0];
        }
        records_.push_back(std::move(record));
        boundOf.push_back(index);
    }
    for (CameraRecord& record : records_)
        if (record.hasMatrix)
            for (int a = 0; a < 3; ++a) record.parentOffset[a] = record.matrix[std::size_t(12 + a)];
    if (camText.empty()) return;
    const assets::CameraFollow follow = assets::parseCameraFollow(camText);
    for (const assets::CameraSetup& setup : follow.setups) {
        if (setup.kind != "Bound") continue;
        for (CameraRecord& record : records_)
            if (sameName(record.name, setup.name)) {
                applySetup(record, setup);
                break;
            }
    }
}

CameraRecordSet queryCameraRecords(const CameraRecords& records, const float point[3]) {
    CameraRecordSet set;
    const std::vector<CameraRecord>& all = records.records();
    for (std::size_t index = 0; index < all.size(); ++index)
        if (inside(all[index], point)) set.records.push_back(int(index));
    if (set.records.empty()) set.records.push_back(kRoomRecord);
    const unsigned count = set.records.empty() ? 0 : (set.records[0] == kRoomRecord ? 1u : dropOverlaps(all, set.records));
    for (unsigned i = 0; i < count; ++i) {
        const int id = set.records[i];
        if (id != kRoomRecord && (all[std::size_t(id)].focus || all[std::size_t(id)].blocker)) {
            set.records = {id};
            break;
        }
    }
    set.weights.assign(set.records.size(), 1.0f);
    if (set.records.size() >= 2) {
        float sum = 0.0f;
        for (std::size_t i = 0; i < set.records.size(); ++i) {
            set.weights[i] = weightOf(all[std::size_t(set.records[i])], point);
            sum += set.weights[i];
        }
        if (sum <= 0.0f) {
            set.records.resize(1);
            set.weights.assign(1, 1.0f);
        } else {
            const float scale = 1.0f / sum;
            for (float& weight : set.weights) weight *= scale;
        }
    }
    return set;
}

CameraCommand CameraRecordSelector::update(const float point[3]) {
    CameraRecordSet current = queryCameraRecords(*records_, point);
    bool shares = false;
    for (int old : previous_.records)
        for (int now : current.records)
            if (old == now) shares = true;

    const auto handOver = [&]() {
        if (current.records.size() == 1 && current.records[0] == kRoomRecord) {
            command_ = CameraCommand::Follow;
        } else {
            command_ = CameraCommand::RecordPose;
            blend_ = current;
        }
    };
    if (!shares) {
        handOver();
    } else if (current == previous_) {
        if (current.records.size() > 1) blend_.weights = current.weights;
    } else {
        bool inBlend = false;
        for (int now : current.records)
            for (int member : blend_.records)
                if (now == member) inBlend = true;
        if (inBlend) blend_ = current;
        else handOver();
    }
    previous_ = std::move(current);
    return command_;
}

RecordPose evaluateRecordPose(const CameraRecords& records, const CameraRecordSet& set, const float target[3]) {
    RecordPose out;
    float distance = 0.0f;
    for (std::size_t i = 0; i < set.records.size(); ++i) {
        const CameraRecord& r = records.records()[std::size_t(set.records[i])];
        const float weight = set.weights[i];
        float p[3] = {r.location[0], r.location[1], r.location[2]};
        if (!(r.type & 1)) p[0] = target[0] + r.location[0];
        if (!(r.type & 2)) p[1] = r.location[1] + target[1];
        if (!(r.type & 4)) p[2] = target[2] + r.location[2];
        float look[3];
        const float off[3] = {float(r.offset[0]), float(r.offset[1]), float(r.offset[2])};
        look[0] = !(r.type & 8) ? off[0] + target[0] : (!(r.type & 1) ? target[0] : p[0] + off[0]);
        look[1] = !(r.type & 0x10) ? off[1] + target[1] : (!(r.type & 2) ? target[1] : p[1] + off[1]);
        look[2] = !(r.type & 0x20) ? off[2] + target[2] : (!(r.type & 4) ? target[2] : p[2] + off[2]);
        if (r.type & 0x400)
            for (int a = 0; a < 3; ++a) look[a] = p[a] + off[a];
        const float d[3] = {look[0] - p[0], look[1] - p[1], look[2] - p[2]};
        distance += length3(d) * weight;
        for (int a = 0; a < 3; ++a) {
            out.look[a] += weight * look[a];
            out.eye[a] += weight * p[a];
        }
        out.fovDegrees += float(r.fov) * weight;
        out.offset3d += r.offset3d * weight;
    }
    if (set.records.size() > 1) {
        float direction[3] = {out.eye[0] - out.look[0], out.eye[1] - out.look[1], out.eye[2] - out.look[2]};
        const float length = length3(direction);
        if (length > 0)
            for (float& component : direction) component /= length;
        for (int a = 0; a < 3; ++a) out.eye[a] = out.look[a] + direction[a] * distance;
    }
    return out;
}

float poseYaw(const RecordPose& pose) {
    const float x = pose.look[0] - pose.eye[0], z = pose.look[2] - pose.eye[2];
    return (x == 0.0f && z == 0.0f) ? 0.0f : std::atan2(x, z);
}

float posePitch(const RecordPose& pose) {
    const float x = pose.look[0] - pose.eye[0], z = pose.look[2] - pose.eye[2];
    return std::atan2(pose.look[1] - pose.eye[1], std::sqrt(x * x + z * z));
}

}

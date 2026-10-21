#include "block_reader.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace chase::assets {
namespace {
using detail::cString;
using detail::readF32;
using Reader = detail::BlockReader;

constexpr Matrix identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

void need(const Bytes& block, std::size_t size, const char* what) {
    if (block.size() < size) throw std::runtime_error(std::string("animation block too small: ") + what);
}

Matrix readMatrix(const Bytes& data, std::size_t offset) {
    Matrix m{};
    for (std::size_t i = 0; i < 16; ++i) m[i] = readF32(data, offset + 4 * i);
    return m;
}

// 0x253764: first block (name hash), pointer array, u16(+6) entry blocks, then key times
// (+0x10, f32 each) and key values (+0x14, 5 x f32 each); u16 +4 is the key count.
AnimTrack readTrack(Reader& r, const Bytes& records, std::size_t index) {
    need(records, (index + 1) * 0x18, "track");
    std::size_t at = index * 0x18;
    AnimTrack track;
    Bytes name = r.next();
    need(name, 4, "track name");
    track.hash = readU32(name, 0);
    r.skip();
    for (unsigned k = 0; k < readU16(records, at + 6); ++k) r.skip();
    Bytes times = r.next(), values = r.next();
    std::size_t count = readU16(records, at + 4);
    if (times.size() != count * 4 || values.size() != count * 20) throw std::runtime_error("track key block size mismatch");
    for (std::size_t k = 0; k < count; ++k) {
        CurveKey key;
        key.time = readF32(times, 4 * k);
        key.inDelta = readF32(values, 20 * k);
        key.inValue = readF32(values, 20 * k + 4);
        key.value = readF32(values, 20 * k + 8);
        key.outDelta = readF32(values, 20 * k + 12);
        key.outValue = readF32(values, 20 * k + 16);
        track.keys.push_back(key);
    }
    return track;
}

// LSB-first bit reader 0x247a48; reads past the end see zero bits.
struct Bits {
    const Bytes& data;
    std::uint64_t pos = 0;
    std::uint64_t peek(std::uint64_t at, unsigned count) const {
        std::uint64_t window = 0;
        for (unsigned i = 0; i < 5; ++i) {
            std::uint64_t index = (at >> 3) + i;
            if (index < data.size()) window |= std::uint64_t(data[index]) << (8 * i);
        }
        window >>= at & 7;
        return count >= 64 ? window : window & ((std::uint64_t(1) << count) - 1);
    }
    std::uint64_t read(unsigned count) {
        std::uint64_t value = peek(pos, count);
        pos += count;
        return value;
    }
};

// Channel table 0x376494 (base bits, delta-width bits) with the scales written by 0x347c5c.
struct ChannelFormat {
    unsigned baseBits;
    unsigned widthBits;
    double scale;
};
constexpr ChannelFormat channelFormats[12] = {
    {12, 4, 0.02}, {12, 4, 0.02}, {12, 4, 0.02}, {12, 4, 0.01}, {12, 4, 0.01}, {12, 4, 0.01},
    {12, 4, 0.01}, {12, 4, 0.01}, {12, 4, 0.01}, {16, 5, 0.01}, {16, 5, 0.01}, {16, 5, 0.01}};
constexpr unsigned rawMatrixHeader = 0xfa;

// 0x24c5a0 inverts the bind matrix as a rigid transform (transposed 3x3).
Matrix rigidInverse(const Matrix& m) {
    Matrix out{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) out[std::size_t(i * 4 + j)] = m[std::size_t(j * 4 + i)];
        out[std::size_t(12 + i)] = -(m[12] * m[std::size_t(i * 4)] + m[13] * m[std::size_t(i * 4 + 1)] +
                                     m[14] * m[std::size_t(i * 4 + 2)]);
    }
    out[15] = 1;
    return out;
}

// 0x257ea0: normalizes in place and returns the length; a vector shorter than 2^-23 becomes (0, 0, 1).
float normalizeRow(float* row) {
    const float length = std::sqrt(row[0] * row[0] + row[1] * row[1] + row[2] * row[2]);
    if (length > 1.1920929e-7f) {
        const float inverse = 1.0f / length;
        for (int k = 0; k < 3; ++k) row[k] *= inverse;
    } else {
        row[0] = row[1] = 0;
        row[2] = 1;
    }
    return length;
}

float dotRows(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void subtractScaled(float* row, const float* other, float factor) {
    for (int k = 0; k < 3; ++k) row[k] -= other[k] * factor;
}

// 0x247660: Euler angles of a 3x3 rotation (rows of stride 4); of the two equivalent triples the one with the smaller length wins.
void matrixToEuler(const float* m, float* out) {
    constexpr float pi = 3.14159274f;
    const float cy = std::sqrt(m[0] * m[0] + m[4] * m[4]);
    float a[3];
    if (cy > 1.9073486e-6f) {
        a[0] = std::atan2(-m[9], m[10]);
        a[1] = std::atan2(m[8], cy);
        a[2] = std::atan2(-m[4], m[0]);
    } else {
        a[0] = std::atan2(m[6], m[5]);
        a[1] = std::atan2(m[8], cy);
        a[2] = 0;
    }
    const float b[3] = {a[0] + pi, pi - a[1], a[2] + pi};
    const auto length = [](const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
    const float* chosen = length(a) >= length(b) ? b : a;
    std::copy_n(chosen, 3, out);
}

// 0x1324f8: splits a stored row-vector 4x4 into Euler rotation, scale, shear and translation.
void decomposeMatrix(const float* matrix, float* values) {
    float m[16];
    std::copy_n(matrix, 16, m);
    std::copy_n(matrix + 12, 3, values + 9);
    float* scale = values + 3;
    float* shear = values + 6;
    float* row0 = m;
    float* row1 = m + 4;
    float* row2 = m + 8;
    scale[0] = normalizeRow(row0);
    shear[0] = dotRows(row0, row1);
    subtractScaled(row1, row0, shear[0]);
    scale[1] = normalizeRow(row1);
    if (std::fabs(scale[1]) > 1e-5f) shear[0] /= scale[1];
    shear[1] = dotRows(row0, row2);
    subtractScaled(row2, row0, shear[1]);
    shear[2] = dotRows(row1, row2);
    subtractScaled(row2, row1, shear[2]);
    scale[2] = normalizeRow(row2);
    if (std::fabs(scale[2]) > 1e-5f) {
        shear[1] /= scale[2];
        shear[2] /= scale[2];
    }
    const float determinant = row0[0] * (row1[1] * row2[2] - row1[2] * row2[1]) + row0[1] * (row1[2] * row2[0] - row1[0] * row2[2]) +
                              row0[2] * (row1[0] * row2[1] - row1[1] * row2[0]);
    if (determinant < 0) {
        for (int k = 0; k < 3; ++k) scale[k] = -scale[k];
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column) m[row * 4 + column] = -m[row * 4 + column];
    }
    matrixToEuler(m, values);
}
}

Matrix multiply(const Matrix& a, const Matrix& b) {
    Matrix out{};
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = 0; j < 4; ++j) {
            float sum = 0;
            for (std::size_t k = 0; k < 4; ++k) sum += a[i * 4 + k] * b[k * 4 + j];
            out[i * 4 + j] = sum;
        }
    return out;
}

Skeleton parseSkeleton(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M14);
    Bytes head = r.next();
    need(head, 2, "skeleton header");
    std::size_t count = head[1];
    r.skip();
    r.skip();
    Bytes records = r.next();
    need(records, count * 0x98, "joints");
    Skeleton skeleton;
    for (std::size_t i = 0; i < count; ++i) {
        Joint joint;
        joint.name = cString(r.next());
        joint.parent = std::int32_t(readU32(records, i * 0x98 + 4));
        if (joint.parent < -1 || joint.parent >= int(i)) throw std::runtime_error("joint parent out of order");
        joint.world = readMatrix(records, i * 0x98 + 8);
        joint.local = readMatrix(records, i * 0x98 + 0x48);
        for (std::size_t k = 0; k < 4; ++k) joint.orientation[k] = readF32(records, i * 0x98 + 0x88 + 4 * k);
        skeleton.joints.push_back(std::move(joint));
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed skeleton blocks");
    return skeleton;
}

Animation parseAnimation(const Bytes& data) {
    Reader r(data);
    r.cursor.push(Pool::M14);
    Bytes head = r.next();
    need(head, 0x24, "animation header");
    Animation anim;
    anim.frames = readU16(head, 4) & 0x3fffu;
    anim.headerRateIndex = readU16(head, 4) >> 14;
    if (readU32(head, 8)) {
        Bytes table = r.next();
        need(table, 4, "joint table");
        std::uint32_t word = readU32(table, 0);
        anim.hasJointTable = true;
        anim.jointTableWord = word;
        std::size_t count = (word & 0x1fff) + ((word & 0x10000000) >> 28);
        if (word & 0x20000000) anim.longCurve = (word & 0x10000000) ? int(count) - 1 : 0;
        r.skip();
        for (std::size_t i = 0; i < count; ++i) anim.jointCurves.push_back(r.next());
    }
    auto skipGroups = [&](bool present, unsigned groups, int perGroup) {
        if (!present) return;
        r.skip();
        for (unsigned g = 0; g < groups; ++g)
            for (int k = 0; k < perGroup; ++k) r.skip();
    };
    if (readU32(head, 0xc)) {
        r.skip();
        for (unsigned g = 0; g < head[1]; ++g) {
            anim.objects.push_back(cString(r.next()));
            anim.objectBlocks.push_back({r.next(), r.next()});
        }
    }
    skipGroups(readU32(head, 0x10) != 0, head[2], 2);
    skipGroups(head[3] != 0, head[3], 3);
    if (unsigned tracks = readU16(head, 6)) {
        Bytes records = r.next();
        for (unsigned i = 0; i < tracks; ++i) anim.tracks.push_back(readTrack(r, records, i));
    }
    if (std::uint32_t subs = readU32(head, 0x20)) {
        const Bytes aspects = r.next();
        need(aspects, std::size_t(subs) * 16, "sub-animation aspect list");
        for (std::uint32_t s = 0; s < subs; ++s) anim.subAspects.push_back(readU32(aspects, std::size_t(s) * 16 + 8));
        for (std::uint32_t s = 0; s < subs; ++s) {
            Bytes fixups = r.next();
            Bytes countBlock = r.next();
            need(fixups, 4, "fixups");
            need(countBlock, 4, "sub-animation track count");
            Bytes records = r.next();
            std::vector<AnimTrack> tracks;
            for (std::uint32_t i = 0; i < readU32(countBlock, 0); ++i) tracks.push_back(readTrack(r, records, i));
            anim.subAnimations.push_back(std::move(tracks));
            anim.subData.push_back(r.next());
            const std::uint32_t fixupCount = readU32(fixups, 0);
            if (fixups.size() != 4 + std::size_t(fixupCount) * 4) throw std::runtime_error("sub-animation fixup list size mismatch");
            std::vector<AnimationTarget> targets;
            for (std::uint32_t i = 0; i < fixupCount; ++i) targets.push_back({readU32(fixups, 4 + std::size_t(i) * 4), cString(r.next())});
            anim.subAnimationTargets.push_back(std::move(targets));
        }
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed animation blocks");
    return anim;
}

float animationFramesPerSecond(const Animation& animation) {
    constexpr float rates[4] = {15.0f, 30.0f, 60.0f, 0.0f};
    return rates[animation.hasJointTable ? (animation.jointTableWord >> 13) & 3 : animation.headerRateIndex];
}

bool animationLoops(const Animation& animation) {
    return animation.hasJointTable && (animation.jointTableWord & 0x20000000u) != 0;
}

std::uint16_t animationLength(const Animation& animation) {
    const std::uint16_t frames = std::uint16_t(animation.frames & 0x3fffu);
    return animationLoops(animation) ? frames : std::uint16_t(frames - 1u);
}

float animationDuration(const Animation& animation) {
    const float rate = animationFramesPerSecond(animation);
    return rate > 0 && animation.frames > 1 ? float(animation.frames - 1) / rate : 0.0f;
}

std::optional<RootMotion> rootMotion(const Animation& animation) {
    const std::uint32_t word = animation.jointTableWord;
    if (!animation.hasJointTable || !(word & 0x10000000u)) return std::nullopt;
    const std::size_t curve = word & 0x1fffu;
    const unsigned samples = ((word >> 15) & 0x1fffu) + ((word >> 29) & 1u);
    if (curve >= animation.jointCurves.size() || samples == 0) return std::nullopt;
    auto position = [&](unsigned frame) {
        const JointChannels c = decodeChannels(animation.jointCurves[curve], std::min(frame, samples - 1), samples);
        return std::array<float, 3>{c.values[9], c.values[10], c.values[11]};
    };
    const std::array<float, 3> origin = position(0);
    RootMotion motion;
    motion.translation.push_back({0, 0, 0});
    for (unsigned frame = 1; frame <= samples; ++frame) {
        const std::array<float, 3> p = position(frame);
        std::array<float, 3> relative{};
        for (std::size_t k = 0; k < 3; ++k) {
            relative[k] = p[k] - origin[k];
            motion.minimum[k] = std::min(motion.minimum[k], relative[k]);
            motion.maximum[k] = std::max(motion.maximum[k], relative[k]);
        }
        if (frame < samples) motion.translation.push_back(relative);
    }
    return motion;
}

JointChannels decodeChannels(const Bytes& curve, unsigned frame, unsigned frames) {
    JointChannels out;
    Bits bits{curve};
    unsigned head = unsigned(bits.read(8));
    if (head == rawMatrixHeader) {
        const std::size_t offset = 1 + std::size_t(frame) * 64;
        if (curve.size() < offset + 64) throw std::runtime_error("raw matrix frame out of range");
        float matrix[16];
        for (std::size_t k = 0; k < 16; ++k) matrix[k] = readF32(curve, offset + 4 * k);
        out.rawMatrix = true;
        out.mask = 0xfff;
        decomposeMatrix(matrix, out.values);
        return out;
    }
    unsigned shift = head >> 4, extra = 0;
    for (unsigned group = 0; group < 4; ++group)
        if (head & (1u << group)) out.mask |= unsigned(bits.read(3)) << (3 * group);
    if (shift & 8) {
        shift &= 7;
        extra = 2;
    }
    for (unsigned channel = 0; channel < 12; ++channel) {
        if (!(out.mask & (1u << channel))) continue;
        ChannelFormat format = channelFormats[channel];
        if (shift) {
            format.baseBits += shift + (channel >= 9 ? extra : 0);
            format.widthBits += shift;
            format.scale /= double(1u << shift);
        }
        auto base = std::int64_t(bits.read(format.baseBits));
        if (base & (std::int64_t(1) << (format.baseBits - 1))) base -= std::int64_t(1) << format.baseBits;
        unsigned width = unsigned(bits.peek(bits.pos, format.widthBits) & 0xff);
        if (width) base += std::int64_t(bits.peek(bits.pos + format.widthBits + std::uint64_t(frame) * width, width));
        bits.pos += format.widthBits + std::uint64_t(frames) * width;
        double value = double(base) * format.scale;
        if (channel >= 3 && channel < 6 && value < 0.0001) value = 0.0001;
        out.values[channel] = float(value);
    }
    return out;
}

std::array<float, 4> eulerToQuaternion(float x, float y, float z) {
    double sx = std::sin(x / 2.0), cx = std::cos(x / 2.0);
    double sy = std::sin(y / 2.0), cy = std::cos(y / 2.0);
    double sz = std::sin(z / 2.0), cz = std::cos(z / 2.0);
    return {float(-(cy * sx * cz) - sy * cx * sz), float(cy * sx * sz - sy * cx * cz),
            float(-(sy * sx * cz) - cy * cx * sz), float(cy * cx * cz - sy * sx * sz)};
}

JointPose jointPose(const JointChannels& channels) {
    JointPose pose;
    const float* c = channels.values;
    pose.rotation = eulerToQuaternion(c[0], c[1], c[2]);
    for (int i = 0; i < 3; ++i) {
        pose.scale[i] = c[3 + i];
        pose.shear[i] = c[6 + i];
        pose.translation[i] = c[9 + i];
    }
    pose.mask = channels.mask;
    return pose;
}

JointPose blendPoses(const std::vector<std::pair<JointPose, float>>& layers) {
    if (layers.size() == 1) return layers.front().first;
    JointPose out;
    for (const auto& layer : layers) out.mask |= layer.first.mask;
    std::array<float, 4> rotation{}, reference{};
    float scale[3] = {0, 0, 0}, shear[3] = {0, 0, 0}, translation[3] = {0, 0, 0};
    bool haveReference = false;
    for (const auto& [pose, weight] : layers) {
        if (pose.mask & 0x7) {
            float signedWeight = weight;
            if (!haveReference) {
                reference = pose.rotation;
                haveReference = true;
            } else {
                float dot = 0;
                for (int k = 0; k < 4; ++k) dot += reference[std::size_t(k)] * pose.rotation[std::size_t(k)];
                if (dot < 0) signedWeight = -weight;
            }
            for (int k = 0; k < 4; ++k) rotation[std::size_t(k)] += pose.rotation[std::size_t(k)] * signedWeight;
        }
        for (int i = 0; i < 3; ++i) {
            if (out.mask & 0x38) scale[i] += pose.scale[i] * weight;
            if (pose.mask & out.mask & 0x1c0) shear[i] += pose.shear[i] * weight;
            if (pose.mask & out.mask & 0xe00) translation[i] += pose.translation[i] * weight;
        }
    }
    if (out.mask & 0x7) {
        float length = std::sqrt(rotation[0] * rotation[0] + rotation[1] * rotation[1] +
                                 rotation[2] * rotation[2] + rotation[3] * rotation[3]);
        if (length > 1.1920929e-7f)
            for (auto& value : rotation) value /= length;
        else
            rotation = {0, 0, 0, 1};
        out.rotation = rotation;
    }
    std::copy_n(scale, 3, out.scale);
    std::copy_n(shear, 3, out.shear);
    std::copy_n(translation, 3, out.translation);
    return out;
}

Matrix poseMatrix(const JointPose& pose) {
    Matrix m = identity;
    if (pose.mask & 0x7) {
        auto [x, y, z, w] = pose.rotation;
        m[0] = 1 - 2 * (y * y + z * z);
        m[1] = 2 * (x * y - w * z);
        m[2] = 2 * (x * z + w * y);
        m[4] = 2 * (x * y + w * z);
        m[5] = 1 - 2 * (x * x + z * z);
        m[6] = 2 * (y * z - w * x);
        m[8] = 2 * (x * z - w * y);
        m[9] = 2 * (y * z + w * x);
        m[10] = 1 - 2 * (x * x + y * y);
    }
    auto addRow = [&](int to, int from, float factor) {
        for (int j = 0; j < 3; ++j) m[std::size_t(to * 4 + j)] += m[std::size_t(from * 4 + j)] * factor;
    };
    if (pose.mask & 0x1c0) {
        addRow(2, 1, pose.shear[2]);
        addRow(1, 0, pose.shear[1]);
        addRow(0, 2, pose.shear[0]);
    }
    if (pose.mask & 0x38)
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) m[std::size_t(i * 4 + j)] *= pose.scale[i];
    if (pose.mask & 0xe00)
        for (int j = 0; j < 3; ++j) m[std::size_t(12 + j)] = pose.translation[j];
    return m;
}

Matrix localMatrix(const JointChannels& channels) { return poseMatrix(jointPose(channels)); }

Matrix objectAnimationMatrix(const Animation& animation, std::size_t object, unsigned frame) {
    return localMatrix(decodeChannels(animation.objectBlocks.at(object)[0], frame, animation.frames));
}

float evaluateTrack(const AnimTrack& track, float time) {
    const auto& keys = track.keys;
    if (keys.empty()) return 0;
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;
    std::size_t i = 1;
    while (keys[i].time < time) ++i;
    const CurveKey& a = keys[i - 1];
    const CurveKey& b = keys[i];
    double x0 = a.time, x1 = a.time + a.outDelta, x2 = b.time - b.inDelta, x3 = b.time;
    auto bezier = [](double p0, double p1, double p2, double p3, double s) {
        double u = 1 - s;
        return u * u * u * p0 + 3 * u * u * s * p1 + 3 * u * s * s * p2 + s * s * s * p3;
    };
    double lo = 0, hi = 1, s = 0.5;
    for (int step = 0; step < 40; ++step) {
        s = (lo + hi) / 2;
        if (bezier(x0, x1, x2, x3, s) < time) lo = s; else hi = s;
    }
    return float(bezier(a.value, a.outValue, b.inValue, b.value, s));
}

JointPose orientedPose(const JointChannels& channels, const Joint& joint) {
    JointPose pose = jointPose(channels);
    const auto& o = joint.orientation;
    if (!(o[3] < 1 && o[3] > -1)) return pose;
    const auto p = pose.rotation;
    pose.rotation = {p[3] * o[0] + p[0] * o[3] + p[1] * o[2] - p[2] * o[1],
                     p[3] * o[1] + p[1] * o[3] + p[2] * o[0] - p[0] * o[2],
                     p[3] * o[2] + p[2] * o[3] + p[0] * o[1] - p[1] * o[0],
                     p[3] * o[3] - p[0] * o[0] - p[1] * o[1] - p[2] * o[2]};
    pose.mask |= 7;
    return pose;
}

std::vector<Matrix> skinMatrices(const Skeleton& skeleton, const Animation& animation, double frame) {
    unsigned frames = animation.frames ? animation.frames : 1;
    double wrapped = std::fmod(frame, double(frames));
    if (wrapped < 0) wrapped += frames;
    unsigned first = unsigned(wrapped) % frames, second = (first + 1) % frames;
    float fraction = float(wrapped - std::floor(wrapped));
    std::vector<Matrix> world, skin;
    for (std::size_t i = 0; i < skeleton.joints.size(); ++i) {
        const Joint& joint = skeleton.joints[i];
        Matrix local = joint.local;
        if (i < animation.jointCurves.size()) {
            unsigned samples = animation.frames + (int(i) == animation.longCurve ? 1 : 0);
            JointChannels a = decodeChannels(animation.jointCurves[i], first, samples);
            if (fraction == 0) {
                local = poseMatrix(orientedPose(a, joint));
            } else {
                JointChannels b = decodeChannels(animation.jointCurves[i], second, samples);
                local = poseMatrix(blendPoses({{orientedPose(a, joint), 1 - fraction}, {orientedPose(b, joint), fraction}}));
            }
        }
        world.push_back(joint.parent < 0 ? local : multiply(local, world[std::size_t(joint.parent)]));
        skin.push_back(multiply(rigidInverse(joint.world), world.back()));
    }
    return skin;
}

Matrix jointAttachment(const Joint& joint, const Matrix& skin) {
    Matrix bindPosition = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, joint.world[12], joint.world[13], joint.world[14], 1};
    return multiply(bindPosition, skin);
}

void skinVertex(const Vertex& vertex, const std::vector<Matrix>& skin, float position[3], float normal[3]) {
    unsigned total = 0;
    for (auto weight : vertex.weights) total += weight;
    if (!total || skin.empty()) {
        std::copy_n(vertex.position, 3, position);
        std::copy_n(vertex.normal, 3, normal);
        return;
    }
    std::fill_n(position, 3, 0.0f);
    std::fill_n(normal, 3, 0.0f);
    for (int k = 0; k < 4; ++k) {
        if (!vertex.weights[k]) continue;
        if (vertex.joints[k] >= skin.size()) continue;
        const Matrix& m = skin[vertex.joints[k]];
        float w = float(vertex.weights[k]) / float(total);
        for (std::size_t j = 0; j < 3; ++j) {
            const float* p = vertex.position;
            const float* n = vertex.normal;
            position[j] += w * (p[0] * m[j] + p[1] * m[4 + j] + p[2] * m[8 + j] + m[12 + j]);
            normal[j] += w * (n[0] * m[j] + n[1] * m[4 + j] + n[2] * m[8 + j]);
        }
    }
}
} // namespace chase::assets

#include "block_reader.hpp"
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

// 0x253764: first block (name hash), pointer array, u16(+6) entry blocks, +0x10, +0x14.
void skipTrack(Reader& r, const Bytes& records, std::size_t index) {
    need(records, (index + 1) * 0x18, "track");
    r.skip();
    r.skip();
    for (unsigned k = 0; k < readU16(records, index * 0x18 + 6); ++k) r.skip();
    r.skip();
    r.skip();
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
        out[std::size_t(12 + i)] = -(m[12] * m[std::size_t(i)] + m[13] * m[std::size_t(4 + i)] +
                                     m[14] * m[std::size_t(8 + i)]);
    }
    out[15] = 1;
    return out;
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
    anim.frames = readU16(head, 4);
    if (readU32(head, 8)) {
        Bytes table = r.next();
        need(table, 4, "joint table");
        std::uint32_t word = readU32(table, 0);
        std::size_t count = (word & 0x1fff) + ((word & 0x10000000) >> 28);
        r.skip();
        for (std::size_t i = 0; i < count; ++i) anim.jointCurves.push_back(r.next());
    }
    auto skipGroups = [&](bool present, unsigned groups, int perGroup) {
        if (!present) return;
        r.skip();
        for (unsigned g = 0; g < groups; ++g)
            for (int k = 0; k < perGroup; ++k) r.skip();
    };
    skipGroups(readU32(head, 0xc) != 0, head[1], 3);
    skipGroups(readU32(head, 0x10) != 0, head[2], 2);
    skipGroups(head[3] != 0, head[3], 3);
    if (unsigned tracks = readU16(head, 6)) {
        Bytes records = r.next();
        for (unsigned i = 0; i < tracks; ++i) skipTrack(r, records, i);
    }
    if (std::uint32_t subs = readU32(head, 0x20)) {
        r.skip();
        for (std::uint32_t s = 0; s < subs; ++s) {
            Bytes fixups = r.next();
            Bytes countBlock = r.next();
            need(fixups, 4, "fixups");
            need(countBlock, 4, "sub-animation track count");
            Bytes records = r.next();
            for (std::uint32_t i = 0; i < readU32(countBlock, 0); ++i) skipTrack(r, records, i);
            r.skip();
            for (std::uint32_t i = 0; i < readU32(fixups, 0); ++i) r.skip();
        }
    }
    r.cursor.pop();
    if (!r.cursor.exhausted()) throw std::runtime_error("unconsumed animation blocks");
    return anim;
}

JointChannels decodeChannels(const Bytes& curve, unsigned frame, unsigned frames) {
    JointChannels out;
    Bits bits{curve};
    unsigned head = unsigned(bits.read(8));
    if (head == rawMatrixHeader) {
        out.rawMatrix = true;
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

Matrix localMatrix(const JointChannels& pose) {
    Matrix m = identity;
    const float* c = pose.values;
    if (pose.mask & 0x7) {
        auto [x, y, z, w] = eulerToQuaternion(c[0], c[1], c[2]);
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
        addRow(2, 1, c[8]);
        addRow(1, 0, c[7]);
        addRow(0, 2, c[6]);
    }
    if (pose.mask & 0x38)
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) m[std::size_t(i * 4 + j)] *= c[3 + i];
    if (pose.mask & 0xe00)
        for (int j = 0; j < 3; ++j) m[std::size_t(12 + j)] = c[9 + j];
    return m;
}

std::vector<Matrix> skinMatrices(const Skeleton& skeleton, const Animation& animation, unsigned frame) {
    unsigned frames = animation.frames ? animation.frames : 1;
    std::vector<Matrix> world, skin;
    for (std::size_t i = 0; i < skeleton.joints.size(); ++i) {
        const Joint& joint = skeleton.joints[i];
        Matrix local = joint.local;
        if (i < animation.jointCurves.size()) {
            JointChannels pose = decodeChannels(animation.jointCurves[i], frame % frames, animation.frames);
            if (!pose.rawMatrix) local = localMatrix(pose);
        }
        world.push_back(joint.parent < 0 ? local : multiply(local, world[std::size_t(joint.parent)]));
        skin.push_back(multiply(rigidInverse(joint.world), world.back()));
    }
    return skin;
}
} // namespace chase::assets

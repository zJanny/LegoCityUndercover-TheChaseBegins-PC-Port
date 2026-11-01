// Bone attaches of characters (docs/re/script-natives-animation.md, entry "Bone attaches of characters"): the local matrix of 0x20f33c, the
// world of 0x109c2c / 0x10f800 (local * skin * parent world) and the link bookkeeping. Made-up matrices; transcribed from the code by
// 20261101-mission-A on 2026-11-01 (REA of game.arm.elf: 0x1bf718, 0x20f33c, 0x237c48, 0x23d39c, 0x109c2c, 0x10f800).
#include "runtime/bone_attach.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Matrix;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(const Matrix& a, const Matrix& b, float tolerance = 1e-5f) {
    for (int i = 0; i < 16; ++i)
        if (std::fabs(a[std::size_t(i)] - b[std::size_t(i)]) > tolerance) return false;
    return true;
}

Matrix translation(float x, float y, float z) { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1}; }

void argumentIsTheRotationOfTheAngles() {
    const float angles[3] = {45.0f, 0.0f, 0.0f};
    const float h = 0.70710678f;
    require(near(boneAttachArgument(angles), {1, 0, 0, 0, 0, h, h, 0, 0, -h, h, 0, 0, 0, 0, 1}), "45 degrees about x: rows (0 c s) and (0 -s c)");
    const float sixty[3] = {60.0f, 0.0f, 0.0f};
    const float c = 0.5f, s = 0.8660254f;
    require(near(boneAttachArgument(sixty), {1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0, 0, 0, 0, 1}), "60 degrees about x");
    const float none[3] = {0, 0, 0};
    require(near(boneAttachArgument(none), translation(0, 0, 0)), "no angle is the identity");
}

void localAddsOnlyTheBindTranslation() {
    const float angles[3] = {45.0f, 0.0f, 0.0f};
    const Matrix argument = boneAttachArgument(angles);
    const Matrix bind = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0.5f, 1.25f, -2.0f, 1};
    const Matrix local = boneAttachLocal(argument, bind);
    for (int i = 0; i < 12; ++i) require(local[std::size_t(i)] == argument[std::size_t(i)], "the rotation rows are the argument's, not the bone's");
    require(local[12] == 0.5f && local[13] == 1.25f && local[14] == -2.0f && local[15] == 1.0f, "the translation row is the bind translation");
    Matrix withTranslation = argument;
    withTranslation[12] = 1.0f;
    require(boneAttachLocal(withTranslation, bind)[12] == 1.5f, "the argument's own translation is added to the bind translation");
}

void worldIsLocalTimesSkinTimesParent() {
    const Matrix local = translation(0, 1, 0);
    const Matrix skin = translation(0, 0, 3);
    Matrix parent = translation(10, 0, 0);
    require(near(boneAttachedWorld(local, skin, parent), translation(10, 1, 3)), "translations add through the three matrices");
    const float quarter[3] = {0, 90.0f, 0};
    parent = chase::assets::multiply(boneAttachArgument(quarter), translation(10, 0, 0));
    const Matrix world = boneAttachedWorld(translation(0, 0, 1), translation(0, 0, 0), parent);
    require(std::fabs(std::fabs(world[12] - 10.0f) - 1.0f) < 1e-5f, "the parent's turn carries the offset");
    require(near(boneAttachedWorld(local, skin, parent), chase::assets::multiply(chase::assets::multiply(local, skin), parent)), "order local, skin, parent");
}

Attachment boneRotated(const std::string& parent, std::uint32_t sequence) {
    Attachment attachment;
    attachment.kind = AttachKind::BoneRotated;
    attachment.parent = parent;
    attachment.parentBone = "weaponright";
    attachment.rotationDegrees[0] = 45.0f;
    attachment.sequence = sequence;
    return attachment;
}

void linksKeepTheLocalMatrixUntilTheNextCall() {
    BoneAttachments links;
    const Matrix bind = translation(0, 1, 0);
    require(links.attach("camera", boneRotated("man", 1), bind), "the first call is taken");
    require(!links.attach("camera", boneRotated("man", 1), translation(5, 5, 5)), "the same call is not taken twice");
    require(links.find("camera")->local[13] == 1.0f, "the local matrix of the first call stays");
    require(links.attach("camera", boneRotated("man", 2), translation(5, 5, 5)), "a new call takes the matrix again");
    require(links.find("camera")->local[12] == 5.0f, "the new bind translation");
    require(links.attach("camera", boneRotated("other", 2), bind), "a new parent is taken");
    require(links.find("camera")->parent == "other", "the parent changed");
}

void wholeBindForAttachToBone() {
    BoneAttachments links;
    Attachment attachment;
    attachment.kind = AttachKind::Bone;
    attachment.parent = "man";
    attachment.parentBone = "weaponright";
    attachment.sequence = 3;
    const Matrix bind = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0.5f, 1.25f, -2.0f, 1};
    require(links.attach("cup", attachment, bind), "taken");
    require(near(links.find("cup")->local, bind), "AttachToBone without a child bone uses the whole bind matrix (SCRIPT-S317)");
}

void otherKindsDetach() {
    BoneAttachments links;
    require(links.attach("camera", boneRotated("man", 1), translation(0, 0, 0)), "taken");
    Attachment root;
    root.kind = AttachKind::WorldRoot;
    require(!links.attach("camera", root, translation(0, 0, 0)), "a root attach is not a bone link");
    require(links.find("camera") == nullptr, "the link is gone");
}
}

int main() {
    try {
        argumentIsTheRotationOfTheAngles();
        localAddsOnlyTheBindTranslation();
        worldIsLocalTimesSkinTimesParent();
        linksKeepTheLocalMatrixUntilTheNextCall();
        wholeBindForAttachToBone();
        otherKindsDetach();
    } catch (const std::exception& error) {
        std::cerr << "bone-attach-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "bone-attach-test: ok\n";
    return 0;
}

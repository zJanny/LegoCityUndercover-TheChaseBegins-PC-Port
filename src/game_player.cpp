#include "game_player.hpp"
#include "runtime/acrobat_pole.hpp"
#include "runtime/brick_grab.hpp"
#include "runtime/player_motion.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace game_player {
using namespace chase::assets;
using namespace chase::render;
namespace runtime = chase::runtime;

// Stand-ins, not the original: the water flag (ctrl+0x317 bit 3) and the ground object of type 11 are not modelled, so
// the landing always plays the default surface sound (docs/re/audio.md).
constexpr bool kProvisionalWaterLanding = false, kProvisionalObject11Landing = false;
// The sprint animation plays at its own 30 frames per second: the recorded footsteps are 10.14 updates apart, which
// is half a cycle at scale 1.0, not at the movement scale ctrl+0x94 = 1.5 (docs/re/audio.md).
constexpr float kSprintAnimationScale = 1.0f;

namespace {
CharacterAppearance readCharacter(FuseArchive& archive, const std::string& path) {
    const FuseEntry* entry = archive.find(path);
    if (!entry) throw std::runtime_error("not in archive: " + path);
    return characterAppearance(parseLevel(archive.read(*entry)));
}
}

CharacterAppearance playerAppearance(FuseArchive& archive) {
    return readCharacter(archive, kPlayerCharacter);
}

Mat4 PlayerRig::matrix() const {
    const float heading = runtime::headingRadians(motion.facing), s = std::sin(heading), c = std::cos(heading);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, motion.x, vertical.y, motion.z, 1};
}

void applyAiPosition(PlayerRig& player, const runtime::AiCharacter& ai) {
    if (!ai.position) return;
    player.motion.x = (*ai.position)[0];
    player.motion.z = (*ai.position)[2];
    if (ai.heading) {
        player.motion.facing = static_cast<std::int16_t>(*ai.heading);
        player.motion.lastIntent = player.motion.facing;
    }
}

PlayerAnimation aiMoveAnimation(const runtime::AiCharacter& ai) {
    return {ai.move && ai.move->run ? runtime::kAnimationRun : runtime::kAnimationWalk, 1.0f};
}

void PlayerRig::animate(Scene& scene, float dt, int role, float scale) {
    const bool switched = role != animation;
    if (switched) {
        const auto locomotionRole = [](int id) { return id == runtime::kAnimationWalk || id == runtime::kAnimationRun || id == runtime::kAnimationSprint; };
        const bool keepPhase = locomotionRole(role) && locomotionRole(animation);
        const double phase = frame / double(animations.at(animation).frames);
        animation = role;
        frame = keepPhase ? std::floor(phase * double(animations.at(animation).frames)) : 0.0;
    }
    const Animation& current = animations.at(animation);
    const double rate = double(animationFramesPerSecond(current));
    frame = std::fmod(frame + double(dt) * rate * double(scale), double(current.frames));
    pose(scene);
    const float value = runtime::footstepTrackValue(current, float(frame / double(current.frames)));
    if (switched) footsteps.restart(value);
    else if (footsteps.update(value)) sounds.push_back(characterSounds.footstep(position()));
}

void PlayerRig::pose(Scene& scene) const {
    const Mat4 player = matrix();
    skin = poseBody(scene, character, instance, animations.at(animation), frame);
    placeCharacter(scene, character, instance, skin, player);
}

std::optional<Mat4> PlayerRig::jointWorld(const std::string& lowerJointName) const {
    return chase::render::jointWorld(character, lowerJointName, skin, matrix());
}

int PlayerRig::carriedRole(int role, int carryingMode, int stateClip) const {
    if (stateClip != 0 && animations.count(stateClip)) return stateClip;
    if (carryingMode == 0 || !carryAnimations) return role;
    const int clip = carryAnimations->clip(role, carryingMode);
    return animations.count(clip) ? clip : role;
}

void PlayerRig::setVisible(Scene& scene, bool visible) const {
    scene.setInstanceVisible(instance.bodyInstance, visible);
    scene.setInstanceVisible(instance.headInstance, visible);
    if (instance.hatInstance) scene.setInstanceVisible(*instance.hatInstance, visible);
}

std::int16_t headingFromDegrees(float degrees) {
    return runtime::wrapHeading(std::int32_t(std::lround(degrees / 360.0f * 65536.0f)));
}

// Places the character at the level's player start. The head model sits on the animated head joint.
// Returns the rig, or nothing when the level has no "Player1Start".
std::optional<PlayerRig> placePlayer(FuseArchive& archive, Scene& scene, const Level& level) {
    auto start = playerStart(level);
    if (!start) return std::nullopt;
    const CharacterAppearance base = readCharacter(archive, kPlayerBaseCharacter);
    const CharacterAppearance character = readCharacter(archive, kPlayerCharacter);
    PlayerRig rig;
    rig.character = loadCharacterRig(archive, meshModelPath(character.body));
    const std::pair<int, const char*> roles[] = {
        {0, "idle"}, {runtime::kAnimationWalk, "walk"}, {runtime::kAnimationRun, "run"}, {runtime::kAnimationSprint, "sprint"},
        {runtime::kAnimationFall, "fall"}, {runtime::kAnimationLand, "land"}, {runtime::kAnimationFallLand, "fallland"},
        {runtime::kAnimationJump, "jump"}, {runtime::kAnimationJumpRun, "jump_run"}};
    for (const auto& [id, name] : roles)
        rig.animations[id] = loadAnimation(archive, std::string(kPlayerAnimationDirectory) + name + kPlayerAnimationSuffix);
    rig.jumpStanding = runtime::makeJumpProfile(rig.animations.at(runtime::kAnimationJump));
    rig.jumpMoving = runtime::makeJumpProfile(rig.animations.at(runtime::kAnimationJumpRun));
    rig.vertical.landingAnimations =
        runtime::landingAnimations(rig.animations.at(runtime::kAnimationLand), rig.animations.at(runtime::kAnimationFallLand));
    const FuseEntry* soundTable = archive.find("tables/tablessoundfx.bxls");
    const FuseEntry* characterTable = archive.find("tables/tablescharacters.bxls");
    if (soundTable && characterTable) {
        const SoundTables sounds = parseSoundTables(parseTable(archive.read(*soundTable)));
        const std::uint8_t category = characterSoundCategory(parseTable(archive.read(*characterTable)), character.characterEnum);
        if (category < sounds.characterSounds.size()) rig.characterSounds = runtime::CharacterSounds(sounds.characterSounds[category]);
    }

    if (const FuseEntry* animationTable = archive.find("tables/tablesanimations.bxls")) {
        rig.carryAnimations.emplace(parseTable(archive.read(*animationTable)));
        std::vector<int> clips = rig.carryAnimations->carriedClips();
        for (const int id : {runtime::kAnimationBuildit, runtime::kAnimationPickupBig, runtime::kAnimationPickupSmall, runtime::kAnimationPutdownBig, runtime::kAnimationPutdownSmall,
                             runtime::kAnimationScannerOut, runtime::kAnimationScannerIn,
                             runtime::kAnimationPoleMount, runtime::kAnimationPoleIdle, runtime::kAnimationPoleUp, runtime::kAnimationPoleDown,
                             runtime::kAnimationPoleDismount})
            clips.push_back(id);
        clips.insert(clips.end(), runtime::kBrickGrabAnimations.begin(), runtime::kBrickGrabAnimations.end());
        for (const int id : clips) {
            const std::string path = std::string(kCarryAnimationDirectory) + rig.carryAnimations->name(id) + kPlayerAnimationSuffix;
            if (!rig.animations.count(id) && archive.find(path)) rig.animations[id] = loadAnimation(archive, path);
        }
        for (const int id : {runtime::kAnimationLedgeIdle, runtime::kAnimationLedgeLeft, runtime::kAnimationLedgeRight, runtime::kAnimationAcrobatPoleIdle,
                             runtime::kAnimationAcrobatPoleFlip, runtime::kAnimationAcrobatPoleJump, runtime::kAnimationAcrobatPoleJumpOff}) {
            const std::string path = std::string(kCarryAnimationDirectory) + rig.carryAnimations->name(id) + kPlayerAnimationSuffix;
            if (archive.find(path)) rig.animations[id] = loadAnimation(archive, path);
        }
        for (const int id : {runtime::kAnimationZiplineSlide, runtime::kAnimationZiplineGrab, runtime::kAnimationZiplineDismount}) {
            const std::string path = std::string(kPlayerAnimationDirectory) + rig.carryAnimations->name(id) + kPlayerAnimationSuffix;
            if (archive.find(path)) rig.animations[id] = loadAnimation(archive, path);
        }
    }
    rig.vertical.y = start->position[1];
    rig.motion.x = start->position[0];
    rig.motion.z = start->position[2];
    rig.motion.facing = headingFromDegrees(start->orientation[1]);
    rig.motion.lastIntent = rig.motion.facing;
    rig.startX = rig.motion.x;
    rig.startZ = rig.motion.z;
    rig.instance = addCharacter(scene, base, character, "", "player", rig.matrix());
    rig.pose(scene);
    std::cout << "player at " << start->position[0] << ' ' << start->position[1] << ' ' << start->position[2]
              << ", yaw " << start->orientation[1] << ", character " << character.characterEnum << " ("
              << character.head << ")\n";
    return rig;
}

// One player update: locomotion, wall slide (0x21a73c), then the vertical step (jump, ground probe, fall, landing).
// A landing stun suppresses locomotion; in the air the stick keeps its walk or run speed, times 0.8 after a jump.
PlayerAnimation updatePlayer(PlayerRig& player, const runtime::CollisionWorld& collision, const runtime::StickIntent& intent,
                             bool jumpPressed, float dt) {
    using runtime::Posture;
    const runtime::JumpProfiles jumps{&player.jumpStanding, &player.jumpMoving};
    const runtime::PlayerMotionStep update =
        runtime::playerMotionUpdate(player.motion, player.vertical, jumps, collision, intent, jumpPressed, dt);
    const runtime::VerticalStep& vertical = update.vertical;
    const auto landingAnimation = [&]() {
        return PlayerAnimation{player.vertical.hardLanding ? runtime::kAnimationFallLand : runtime::kAnimationLand, 1.0f};
    };
    if (update.stunned) return landingAnimation();
    if (vertical.jumpStarted) {
        player.jumpMovingAnimation = update.jumpMoving;
        player.sounds.push_back(player.characterSounds.jump(player.position()));
    }
    if (vertical.landed) {
        const std::vector<runtime::SoundCall> calls =
            vertical.hard ? player.characterSounds.hardLanding(player.position())
                          : player.characterSounds.landing(update.fallBefore, kProvisionalWaterLanding, kProvisionalObject11Landing, player.position());
        player.sounds.insert(player.sounds.end(), calls.begin(), calls.end());
    }
    if (player.vertical.posture == Posture::Falling || player.vertical.posture == Posture::Sliding) return {runtime::kAnimationFall, 1.0f};
    if (player.vertical.posture == Posture::Jumping || vertical.jumpStarted)
        return {player.jumpMovingAnimation ? runtime::kAnimationJumpRun : runtime::kAnimationJump, 1.0f};
    if (player.vertical.posture == Posture::Landing) return landingAnimation();
    const runtime::LocomotionStep& step = update.locomotion;
    return {step.animation, step.animation == runtime::kAnimationSprint ? kSprintAnimationScale : step.animationScale};
}

}

#include "runtime/player_attack.hpp"
#include "runtime/collision.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {
constexpr float kHeadingUnits = 10430.378f;         // 0x205fbc: atan2 * this, truncated to 16 bits
constexpr float kDegreesToRadiansPi = 3.1415927f;   // 0x21a1d8 / 0x1f7f28: angle * pi * (1 / 180)
constexpr float kDegreesToRadiansScale = 0.0055555557f;
constexpr float kEventThreshold = 0.5f;             // 0x319ffc: an event acts when its value is above 0.5
constexpr float kScoreBase = 1.25f;                 // 0x21a1d8
constexpr std::uint32_t kTinyLengthBits = 0x34000000;   // 0x257ea0: a length at or below this normalises to (0, 0, 1)
constexpr std::uint32_t kVerticalGapBits = 0xbe4ccccc;  // 0x1f7f28: the gaps must compare above this as unsigned words (at or below -0.2)
constexpr float kUpdatesPerSecond = 30.0f;

const PlayerAttackTables& tablesOf(const AttackInput& input) {
    static const PlayerAttackTables kDefaults;
    return input.tables ? *input.tables : kDefaults;
}

std::uint32_t bitsOf(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

std::array<float, 3> transformPoint(const assets::Matrix& m, const std::array<float, 3>& p) {
    return {p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12], p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13],
            p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14]};
}

std::array<float, 3> nodePosition(const assets::Matrix& m) { return {m[12], m[13], m[14]}; }

std::array<float, 3> difference(const std::array<float, 3>& a, const std::array<float, 3>& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

float length3(const std::array<float, 3>& v) {
    float sum = v[0] * v[0];
    sum += v[1] * v[1];
    sum += v[2] * v[2];
    return std::sqrt(sum);
}

float normalise(std::array<float, 3>& v) {
    const float length = length3(v);
    if (bitsOf(length) <= kTinyLengthBits && !std::signbit(length)) {
        v = {0.0f, 0.0f, 1.0f};
        return length;
    }
    const float inverse = 1.0f / length;
    v = {v[0] * inverse, v[1] * inverse, v[2] * inverse};
    return length;
}

float dot3(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

float clampCosine(float value) { return std::min(1.0f, std::max(-1.0f, value)); }

float angleLimit(float degrees) { return degrees * kDegreesToRadiansPi * kDegreesToRadiansScale; }

std::array<float, 3> headingDirection(std::int16_t heading) {
    const float angle = headingRadians(heading);
    return {gameSin(angle), 0.0f, gameCos(angle)};
}

const assets::AnimTrack* findTrack(const assets::Animation& animation, std::uint32_t hash) {
    for (const auto& sub : animation.subAnimations)
        for (const assets::AnimTrack& track : sub)
            if (track.hash == hash) return &track;
    return nullptr;
}

std::vector<float> keyFrames(const assets::Animation& animation, std::uint32_t hash) {
    std::vector<float> frames;
    if (const assets::AnimTrack* track = findTrack(animation, hash))
        for (const assets::CurveKey& key : track->keys)
            if (key.value > kEventThreshold) frames.push_back(key.time * float(animation.frames - 1));
    return frames;
}

std::vector<float> sampleRootCurve(const assets::Animation& animation, unsigned updates, float framesPerUpdate) {
    const std::optional<assets::RootMotion> motion = assets::rootMotion(animation);
    std::vector<float> values;
    for (unsigned k = 0; k <= updates; ++k) {
        if (!motion || motion->translation.empty()) {
            values.push_back(0.0f);
            continue;
        }
        const float frame = float(k) * framesPerUpdate;
        const std::size_t last = motion->translation.size() - 1;
        const std::size_t low = std::min<std::size_t>(std::size_t(frame), last);
        const std::size_t high = std::min(low + 1, last);
        const float fraction = std::min(1.0f, frame - float(low));
        values.push_back(motion->translation[low][2] + (motion->translation[high][2] - motion->translation[low][2]) * fraction);
    }
    return values;
}
}

bool AttackTap::update(bool held) {
    if (held) {
        if (heldUpdates_ != 0xff) ++heldUpdates_;
        return false;
    }
    const bool tap = heldUpdates_ != 0 && heldUpdates_ < kAttackTapUpdates;
    heldUpdates_ = 0;
    return tap;
}

AttackClip makeAttackClip(int id, const assets::Animation& animation) {
    AttackClip clip;
    clip.id = id;
    clip.updates = unsigned(std::ceil(float(assets::animationLength(animation)) * kUpdatesPerSecond / assets::animationFramesPerSecond(animation) - 1e-3f));
    clip.framesPerUpdate = assets::animationFramesPerSecond(animation) / kUpdatesPerSecond;
    clip.hitFrames = keyFrames(animation, kAttackHitTrackStandIn);
    clip.windowFrames = keyFrames(animation, kAttackComboTrackStandIn);
    clip.root = sampleRootCurve(animation, clip.updates, clip.framesPerUpdate);
    return clip;
}

bool AttackClip::passes(const std::vector<float>& frames, unsigned k) const {
    if (k == 0) return false;
    const float before = float(k - 1) * framesPerUpdate, after = float(k) * framesPerUpdate;
    for (const float frame : frames)
        if (before < frame && frame <= after) return true;
    return false;
}

const AttackClip* AttackClips::find(int id) const {
    for (const AttackClip& clip : clips)
        if (clip.id == id) return &clip;
    return nullptr;
}

AttackBody attackBody(const assets::Matrix& node, const std::array<float, 3>& centre, const std::array<float, 3>& half) {
    AttackBody body;
    body.node = node;
    body.centre = centre;
    body.half = half;
    body.radius = length3(half);
    return body;
}

AttackBody playerAttackBody(float x, float y, float z, std::int16_t facing) {
    const std::array<float, 3> forward = headingDirection(facing);
    const assets::Matrix node{forward[2], 0, -forward[0], 0, 0, 1, 0, 0, forward[0], 0, forward[2], 0, x, y, z, 1};
    const PlayerBox box;
    return attackBody(node, {box.centre[0], box.centre[1], box.centre[2]}, {box.half[0], box.half[1], box.half[2]});
}

std::vector<AttackTargetObject> attackTargetObjects(const std::vector<assets::ObjectCollision>& placed) {
    std::vector<AttackTargetObject> objects;
    for (const assets::ObjectCollision& entry : placed) {
        if (entry.className != "leprops/leprop_wobblydestructible") continue;
        AttackTargetObject object;
        object.instance = entry.instance;
        object.collisionDefault = entry.collisionDefault;
        object.body = attackBody(entry.matrix, {entry.centre[0], entry.centre[1], entry.centre[2]}, {entry.half[0], entry.half[1], entry.half[2]});
        objects.push_back(object);
    }
    return objects;
}

float attackScore(float range, float maxAngle, float weight, const std::array<float, 3>& from, const std::array<float, 3>& direction,
                  const std::array<float, 3>& to) {
    std::array<float, 3> v = difference(to, from);
    const float length = normalise(v);
    if (length <= range) {
        v[1] *= 0.75f;
        const float cosine = clampCosine(dot3(v, direction));
        if (std::acos(cosine) <= angleLimit(maxAngle)) return length * (kScoreBase - cosine) * 2.0f * weight;
    }
    return FLT_MAX;
}

std::optional<std::size_t> findAttackTarget(const AttackSearch& search, const AttackEntity& attackerEntity, const AttackBody& attacker,
                                            const std::array<float, 3>& direction, const std::vector<AttackCandidate>& candidates) {
    const std::array<float, 3> from = transformPoint(attacker.node, attacker.centre);
    std::optional<std::size_t> best;
    float bestScore = FLT_MAX;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const AttackCandidate& candidate = candidates[i];
        if (!candidate.collision || !candidate.listed) continue;
        if (!attackCandidatePasses(candidate.entity, attackerEntity, search.flags)) continue;
        const AttackBody& body = candidate.body;
        std::array<float, 3> to = transformPoint(body.node, body.centre);
        float extra = 0.0f;
        if (attacker.half[0] < body.half[0]) extra += body.half[0] - attacker.half[0];
        if (attacker.half[2] < body.half[2]) extra += body.half[2] - attacker.half[2];
        const float rise = attacker.node[13] - body.node[13];
        if (rise < body.half[1] - attacker.half[1] && -body.half[1] < rise) to[1] = from[1];
        const float angle = attackSearchAngle(candidate.entity, search.angle, search.allAround);
        const float score = attackScore(search.range + extra, angle, attackWeight(candidate.entity), from, direction, to);
        if (score < bestScore) {
            best = i;
            bestScore = score;
        }
    }
    return best;
}

AttackEntity playerAttackEntity() {
    AttackEntity player;
    player.kind = kEntityKindCharacter;
    player.player = true;
    return player;
}

std::optional<std::size_t> findAttackTarget(float range, float maxAngle, const AttackBody& attacker, const std::array<float, 3>& direction,
                                            const std::vector<AttackCandidate>& candidates) {
    return findAttackTarget({range, maxAngle, kAttackSearchFlags, true}, playerAttackEntity(), attacker, direction, candidates);
}

PlayerAttackChoice choosePlayerAttack(const AttackBody& attacker, const AttackEntity& player, const std::array<float, 3>& direction, bool running,
                                      const std::vector<AttackCandidate>& candidates, const PlayerAttackTables& tables) {
    PlayerAttackChoice choice;
    choice.running = running;
    const std::optional<std::size_t> grab = findAttackTarget({kGrabSearchRange, kGrabSearchAngle, kAttackSearchFlags, true}, player, attacker, direction, candidates);
    if (grab) {
        const AttackEntity& target = candidates[*grab].entity;
        const bool clearLine = !isAttackCharacter(target) || !tables.lineOfSight || tables.lineOfSight(attacker, candidates, *grab);
        const bool fight = isAttackCharacter(target) && fightTargetPasses(target, player, tables.fightMoveAbilities, clearLine);
        if (fight || (target.word & kEntityWordBoss)) {
            if (!grabStateAllows(target.character.state)) return choice;
            const bool struggle = hasAbility(target.character.abilities, 0x32) || (target.word & kEntityWordBoss) || hasAbility(target.character.abilities, 0x35);
            choice.move = struggle ? PlayerAttackMove::Struggle : PlayerAttackMove::FightMove;
            choice.target = grab;
            choice.running = false;
            return choice;
        }
    }
    const float range = running ? kPlayerMeleeRangeStandIn * kRunningAttackRangeScale : kPlayerMeleeRangeStandIn;
    const std::optional<std::size_t> melee = findAttackTarget({range, kPlayerAttackAngleStandIn, kAttackSearchFlags, true}, player, attacker, direction, candidates);
    if (melee && (!isAttackCharacter(candidates[*melee].entity) || tables.weaponRow == kWeaponRowCharacterMelee)) choice.target = melee;
    if (choice.running && choice.target &&
        length3(difference(nodePosition(attacker.node), nodePosition(candidates[*choice.target].body.node))) < kPlayerMeleeRangeStandIn)
        choice.running = false;
    return choice;
}

bool attackHitTest(float range, float maxAngle, float minAngle, const AttackBody& attacker, const AttackBody& target) {
    const std::array<float, 3> a = transformPoint(attacker.node, attacker.centre);
    const std::array<float, 3> t = transformPoint(target.node, target.centre);
    const float above = (t[1] - target.half[1]) - (a[1] + attacker.half[1]);
    const float below = (a[1] - attacker.half[1]) - (t[1] + target.half[1]);
    if (!(bitsOf(above) > kVerticalGapBits && bitsOf(below) > kVerticalGapBits)) return false;
    if (!(length3(difference(a, t)) - target.radius < range)) return false;
    std::array<float, 3> direction = difference(nodePosition(target.node), nodePosition(attacker.node));
    direction[1] = 0.0f;
    normalise(direction);
    const std::array<float, 3> forward{attacker.node[8], attacker.node[9], attacker.node[10]};
    const float angle = std::acos(clampCosine(dot3(direction, forward)));
    return angleLimit(minAngle) <= angle && angle <= angleLimit(maxAngle);
}

std::optional<std::int16_t> attackTargetHeading(const AttackBody& attacker, const AttackCandidate& target) {
    const std::array<float, 3> point = target.collision ? transformPoint(target.body.node, target.body.centre) : nodePosition(target.body.node);
    const std::array<float, 3> d = difference(point, nodePosition(attacker.node));
    float sum = d[0] * d[0];
    sum += d[2] * d[2];
    if (!(attacker.half[0] < std::sqrt(sum))) return std::nullopt;
    return std::int16_t(std::int32_t(gameAtan2(d[0], d[2]) * kHeadingUnits));
}

AttackStep PlayerAttack::update(const AttackInput& input, LocomotionState& motion, const AttackClips& clips) {
    AttackStep step;
    if (state_ == 0) {
        if (!input.tap || !input.allowed) return step;
        enter(input, motion, clips, input.running, step);
        pendingTap_ = input.tap;
        if (state_ != 0) advance(input, motion, step);
        return step;
    }
    const bool tapBefore = pendingTap_;
    pendingTap_ = input.tap;
    if (tapBefore && window_) {
        enter(input, motion, clips, false, step);
        if (state_ != 0) advance(input, motion, step);
        return step;
    }
    if (elapsed_ >= clip_->updates) {
        step.finished = true;
        step.runAfter = input.intent.active && (clip_->id == kAttackRunningClip || clip_->id == kAttackRunningClip - 1);
        state_ = 0;
        comboIndex_ = 0;
        target_.reset();
        clip_ = nullptr;
        return step;
    }
    advance(input, motion, step);
    return step;
}

void PlayerAttack::enter(const AttackInput& input, LocomotionState& motion, const AttackClips& clips, bool running, AttackStep& step) {
    static const std::vector<AttackCandidate> kNoCandidates;
    const std::vector<AttackCandidate>& candidates = input.candidates ? *input.candidates : kNoCandidates;
    const AttackBody attacker = playerAttackBody(input.x, input.y, input.z, motion.facing);
    const std::array<float, 3> direction = input.intent.active ? headingDirection(input.intent.heading) : headingDirection(motion.facing);
    const PlayerAttackChoice choice = choosePlayerAttack(attacker, input.player, direction, running, candidates, tablesOf(input));
    target_ = choice.target;
    running = choice.running;
    if (choice.move == PlayerAttackMove::Struggle) {
        step.fight = FightStart{*target_, choice.move, -1};
        target_.reset();
        return;
    }
    std::optional<int> move;
    if (choice.move == PlayerAttackMove::FightMove) {
        if (input.random) move = chooseFightMove(lastFightMove_, input.player.character.abilities, tablesOf(input).moves, *input.random);
        if (!move) {
            target_.reset();
            return;
        }
    }
    const int clipId = move ? tablesOf(input).moves[std::size_t(*move)].clip : running ? kAttackRunningClip : kAttackFirstComboClip + int(comboIndex_);
    const AttackClip* clip = clips.find(clipId);
    if (!clip || clip->updates == 0) {
        target_.reset();
        return;
    }
    if (move) {
        lastFightMove_ = *move;
        step.fight = FightStart{*target_, choice.move, *move};
        variant_ = kFightMoveVariant;
    } else {
        if (!running) comboIndex_ = comboIndex_ + 1 >= kPlayerComboLengthStandIn ? 0 : comboIndex_ + 1;
        variant_ = running ? 3 : 0;
    }
    state_ = state_ == kAttackStateAction ? kAttackStateRepeat : kAttackStateAction;
    clip_ = clip;
    elapsed_ = 0;
    window_ = false;
    step.started = true;
    if (target_) {
        if (const std::optional<std::int16_t> heading = attackTargetHeading(attacker, candidates[*target_])) motion.lastIntent = *heading;
        if (!move && length3(difference(nodePosition(attacker.node), nodePosition(candidates[*target_].body.node))) < kPlayerMeleeRangeStandIn)
            step.messages.push_back({*target_, kAttackMessageNotice, 0, 0});
    }
}

void PlayerAttack::advance(const AttackInput& input, LocomotionState& motion, AttackStep& step) {
    static const std::vector<AttackCandidate> kNoCandidates;
    const std::vector<AttackCandidate>& candidates = input.candidates ? *input.candidates : kNoCandidates;
    ++elapsed_;
    const unsigned k = elapsed_;
    if (variant_ == 0 && input.intent.active) motion.lastIntent = input.intent.heading;
    motion.facing = turnFacing(motion.facing, motion.lastIntent);
    const float rootStep = k >= 2 ? clip_->root[k - 1] - clip_->root[k - 2] : 0.0f;
    const std::array<float, 3> along = headingDirection(motion.lastIntent);
    step.deltaX = rootStep * along[0];
    step.deltaZ = rootStep * along[2];
    step.active = true;
    step.state = state_;
    step.clip = clip_->id;
    if (clip_->passes(clip_->hitFrames, k)) {
        const AttackBody attacker = playerAttackBody(input.x + step.deltaX, input.y, input.z + step.deltaZ, motion.facing);
        if (!target_) {
            const std::array<float, 3> direction = input.intent.active ? headingDirection(input.intent.heading) : headingDirection(motion.facing);
            const bool punch = (variant_ == 0 || variant_ == 3) && tablesOf(input).weaponRow != kWeaponRowCharacterMelee;
            const unsigned flags = punch ? kAttackStrikeFlagsPunch : kAttackStrikeFlags;
            target_ = findAttackTarget({kPlayerMeleeRangeStandIn, kPlayerAttackAngleStandIn, flags, true}, input.player, attacker, direction, candidates);
        } else if (*target_ >= candidates.size() || !candidates[*target_].collision ||
                   !attackCandidatePasses(candidates[*target_].entity, input.player, kAttackStrikeFlags)) {
            target_.reset();
        }
        if (target_ && attackHitTest(kPlayerMeleeRangeStandIn, kPlayerAttackAngleStandIn, 0.0f, attacker, candidates[*target_].body))
            step.messages.push_back({*target_, kAttackMessageHit, kAttackDamage, variant_ == 0 || variant_ == 3 ? kAttackHitKind : 0u});
    }
    if (clip_->passes(clip_->windowFrames, k)) window_ = true;
}

}

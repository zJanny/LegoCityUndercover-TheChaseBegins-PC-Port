#include "game_level.hpp"
#include "runtime/ability_table.hpp"
#include "runtime/attack_facts.hpp"
#include "runtime/level_start.hpp"

#include <cctype>
#include <cstring>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
constexpr std::int32_t kDefaultCharacterEnum = 192;
constexpr std::size_t kCharacterWeaponRowByte = 0x17;
constexpr std::size_t kCharacterClassFlagsByte = 0x1c;
constexpr std::uint8_t kTargetModeNobody = 1;

std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::int32_t intParam(const assets::LevelObject& object, const assets::Level* cls, const char* name, std::int32_t fallback) {
    if (!cls) return fallback;
    const assets::LevelParam* value = runtime::effectiveParam(object, *cls, assets::nameHash(name));
    if (!value) return fallback;
    if (value->type == 5 || value->type == 7) return std::int32_t(value->value);
    if (value->type == 0) {
        float f = 0;
        std::memcpy(&f, &value->value, 4);
        return std::int32_t(f);
    }
    return fallback;
}

runtime::AbilitySet rowAbilities(const assets::TableSheet& sheet, std::size_t row) {
    runtime::AbilitySet set{};
    if (row >= sheet.rows || sheet.stride < runtime::kCharacterAbilityMaskOffset + set.size()) return set;
    for (std::size_t i = 0; i < set.size(); ++i) set[i] = sheet.rowData[row * sheet.stride + runtime::kCharacterAbilityMaskOffset + i];
    return set;
}

std::uint8_t rowByte(const assets::TableSheet& sheet, std::size_t row, std::size_t offset) {
    return row < sheet.rows && offset < sheet.stride ? sheet.rowData[row * sheet.stride + offset] : 0;
}
}

// The level characters as attack targets (docs/re/player-attack.md): kind 27, `AIBadGuy` as entity +0xc bit 2 (create 0x182f0c), `Boss_IsBoss`
// as bit 14, the class's abilities and flags from tables/tablescharacters.bxls by `CharacterEnum`; the fight moves of tables/tablescombat.bxls
// sheet 1 and their clips.
void LevelSession::loadAttackCharacters(assets::FuseArchive& archive) {
    attackCharacters.clear();
    attackTables = runtime::PlayerAttackTables{};
    attackPlayer = runtime::playerAttackEntity();
    assets::Table characters;
    if (const assets::FuseEntry* entry = archive.find("tables/tablescharacters.bxls")) characters = assets::parseTable(archive.read(*entry));
    if (const assets::FuseEntry* entry = archive.find("tables/tablescombat.bxls")) attackTables.moves = runtime::fightMoves(assets::parseTable(archive.read(*entry)));
    attackTables.fightMoveAbilities = runtime::fightMoveAbilities(attackTables.moves);
    if (!characters.sheets.empty()) {
        const assets::TableSheet& sheet = characters.sheets[0];
        attackPlayer.character.abilities = rowAbilities(sheet, game_player::kPlayerCharacterId);
        attackTables.weaponRow = rowByte(sheet, game_player::kPlayerCharacterId, kCharacterWeaponRowByte);
        for (const assets::CharacterObject& character : drawable.characters) {
            const assets::LevelObject& object = level.objects[character.placement.objectIndex];
            const assets::Level* cls = classOf(character.placement.className);
            const std::size_t row = std::uint8_t(intParam(object, cls, "CharacterEnum", kDefaultCharacterEnum));
            AttackCharacter entry;
            entry.name = lowerCase(character.placement.instance);
            entry.room = runtime::roomNumber(character.placement.room);
            entry.entity.kind = runtime::kEntityKindCharacter;
            if (intParam(object, cls, "AIBadGuy", 0) != 0) entry.entity.word |= runtime::kEntityWordBadGuy;
            if (intParam(object, cls, "Boss_IsBoss", 0) != 0) entry.entity.word |= runtime::kEntityWordBoss;
            if (cls) {
                const runtime::EntityBox box = runtime::characterEntityBox(*cls, intParam(object, cls, "BoundCollision", 1) != 0);
                entry.entity.boxCentre = box.centre;
                entry.entity.boxHalf = box.half;
                if (!box.collision) entry.entity.flags &= std::uint16_t(~runtime::kEntityFlagCollision);
            } else {
                entry.entity.flags &= std::uint16_t(~runtime::kEntityFlagCollision);
            }
            entry.entity.character.abilities = rowAbilities(sheet, row);
            entry.entity.character.classFlag0 = (rowByte(sheet, row, kCharacterClassFlagsByte) & 1) != 0;
            attackCharacters.push_back(entry);
        }
    }
    if (player && player->carryAnimations)
        for (const runtime::FightMove& move : attackTables.moves) {
            if (attackClips.find(move.clip)) continue;
            const std::string path = std::string(game_player::kCarryAnimationDirectory) + player->carryAnimations->name(move.clip) + game_player::kPlayerAnimationSuffix;
            const assets::FuseEntry* entry = archive.find(path);
            if (!entry) continue;
            if (!player->animations.count(move.clip)) player->animations[move.clip] = assets::parseAnimation(archive.read(*entry));
            attackClips.clips.push_back(runtime::makeAttackClip(move.clip, player->animations.at(move.clip)));
        }
    if (!characterHitSink)
        characterHitSink = [this](const runtime::CharacterHit& hit) {
            std::cout << "attack: hit character " << hit.name << " (message 0, damage " << hit.damage << ", kind " << hit.kind << ") at update " << updates << '\n';
        };
    if (!characterFightSink)
        characterFightSink = [this](const runtime::CharacterFightStart& fight) {
            std::cout << "attack: fight move " << fight.move << " on character " << fight.name << " (state " << fight.state << ") at update " << updates << '\n';
        };
    installAttackFacts();
    std::cout << "attack: " << attackCharacters.size() << " characters, " << attackTables.moves.size() << " fight moves\n";
}

// The level characters as candidates: the box is the class's CollisionBound (entity +0x60 / +0x6c), owner flag 0x200 only with it.
// STAND-IN PLAYER-S15: state 1, on the ground, ctrl+0x240 at the node height, abilities of the class row without the state changes, the target
// mode 0 unless EnableTargetable switched it off, and held by nobody unless attackTables.engagedOn says so.
void LevelSession::appendAttackCharacters(std::vector<runtime::AttackCandidate>& candidates) {
    if (!levelCharacters) return;
    for (std::size_t i = 0; i < attackCharacters.size(); ++i) {
        const AttackCharacter& character = attackCharacters[i];
        if (objectState.isDisabled(character.name) || !roomShown(character.room)) continue;
        const std::optional<assets::Matrix> world = levelCharacters->world(character.name);
        if (!world) continue;
        runtime::AttackCandidate candidate;
        candidate.entity = character.entity;
        candidate.body = runtime::attackBody(*world, candidate.entity.boxCentre, candidate.entity.boxHalf);
        candidate.collision = (candidate.entity.flags & runtime::kEntityFlagCollision) != 0;
        candidate.entity.character.groundY = (*world)[13];
        const runtime::ObjectNativeFlags* flags = levelScripts.objectNatives().flags(character.name);
        if (flags && flags->targetable && !*flags->targetable) candidate.entity.character.targetMode = kTargetModeNobody;
        if (attackTables.engagedOn) candidate.entity.character.heldByOther = attackTables.engagedOn(character.name);
        candidate.name = character.name;
        candidates.push_back(candidate);
    }
}

void LevelSession::deliverAttackCharacterMessages(const runtime::AttackStep& step, const std::vector<runtime::AttackCandidate>& candidates) {
    if (step.fight && step.fight->target < candidates.size()) {
        runtime::CharacterFightStart fight;
        fight.entity = step.fight->target;
        fight.name = candidates[step.fight->target].name;
        fight.move = step.fight->index;
        fight.state = step.fight->move == runtime::PlayerAttackMove::Struggle ? runtime::kStruggleTargetState : runtime::kFightTargetState;
        if (characterFightSink) characterFightSink(fight);
    }
    for (const runtime::AttackMessage& message : step.messages) {
        if (message.id != runtime::kAttackMessageHit || message.target >= candidates.size()) continue;
        if (!runtime::isAttackCharacter(candidates[message.target].entity)) continue;
        runtime::CharacterHit hit;
        hit.entity = message.target;
        hit.name = candidates[message.target].name;
        hit.damage = message.damage;
        hit.kind = message.kind;
        if (characterHitSink) characterHitSink(hit);
    }
}

}

// Message 1 of the character handler 0x18e4c8 (0x18e93c) and the fall-apart body 0x1d3b50 / 0x231a08 / 0x1d27f4 (src/runtime/character_death.*,
// src/runtime/fall_apart.*), docs/re/script-natives-world-death.md entry 2026-10-19T13:21:05. The rules and constants were read in llvm-objdump of
// artifacts/game/game.arm.elf; the expected motion values are worked by hand from those rules. Real-data values: a throwaway C++ census over the 856
// level files and the KillObject / DestroyProp targets (2026-10-19, scratch programs classdump.cpp, kills.cpp and bits.cpp of agent r31-D). Exits with
// 77 without data.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/character_death.hpp"
#include "runtime/character_respawn.hpp"
#include "runtime/fall_apart.hpp"
#include "runtime/level_start.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {

using namespace chase;
using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const std::string& what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what.c_str());
        ++failures;
    }
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

void steps() {
    {
        const CharacterDeathSteps s = characterDeathSteps({});
        expect(s.fallApart && s.dropsBodyParts && s.hidden && !s.smashBricks && !s.deathSound && !s.comesBack && s.afterDeathAi &&
                   s.studDrop == CharacterStudDrop::Dropped,
               "mode 0, minifig: 0x1d3b50, then the stud drop 0x21e2e4 (mode 1), the disable and 0x21e218");
    }
    {
        CharacterDeathInput input;
        input.minifig = false;
        const CharacterDeathSteps s = characterDeathSteps(input);
        expect(!s.fallApart && !s.smashBricks && s.dropsBodyParts && s.deathSound && s.hidden, "mode 0, not a minifig, no SmashDebrisColour: 0x21e854, disable, 0x21e734");
        input.smashColours = std::pair{0xff0000ffu, 0xff00ff00u};
        expect(characterDeathSteps(input).smashBricks, "mode 0, not a minifig, SmashDebrisColour: the bricks 0x23aebc");
    }
    {
        CharacterDeathInput input;
        input.mode = CharacterDeathMode::Smash;
        input.smashColours = std::pair{0xff0000ffu, 0xff0000ffu};
        const CharacterDeathSteps s = characterDeathSteps(input);
        expect(!s.fallApart && s.smashBricks && s.deathSound, "mode 1 skips the minifig test");
    }
    {
        CharacterDeathInput input;
        input.mode = CharacterDeathMode::Vanish;
        const CharacterDeathSteps s = characterDeathSteps(input);
        expect(!s.fallApart && !s.dropsBodyParts && !s.deathSound && s.hidden && s.studDrop == CharacterStudDrop::Dropped, "mode 2: only the disable, then the tail");
        input.mode = CharacterDeathMode::None;
        const CharacterDeathSteps n = characterDeathSteps(input);
        expect(!n.fallApart && !n.dropsBodyParts && n.hidden && n.afterDeathAi, "mode 3: no branch; the tail disables");
    }
    {
        CharacterDeathInput input;
        input.player = true;
        const CharacterDeathSteps s = characterDeathSteps(input);
        expect(s.fallApart && s.comesBack && !s.hidden && s.studDrop == CharacterStudDrop::None && !s.afterDeathAi, "the player's entity comes back (0x18eb48)");
        input.player = false;
        input.respawnEveryone = true;
        expect(characterDeathSteps(input).comesBack, "0x22b0b4 brings everyone back");
        input.respawnEveryone = false;
        input.friendly = true;
        expect(characterDeathSteps(input).comesBack, "a friendly character with AIRespawnIfFriendly (default 1) comes back");
        input.respawnIfFriendly = false;
        expect(!characterDeathSteps(input).comesBack, "a friendly character with AIRespawnIfFriendly 0 stays dead");
    }
    {
        CharacterDeathInput input;
        input.skipsDeathBranch = true;
        const CharacterDeathSteps s = characterDeathSteps(input);
        expect(!s.fallApart && !s.dropsBodyParts && s.hidden && s.studDrop == CharacterStudDrop::Thirty, "byte +0x12e bit 5: no branch, the stud drop of mode 0 with 30");
    }
    expect(kCharacterSmashBricks == 2 && kCharacterStudDropThirty == 30, "0x23aebc count 2 (0x18ea14), stud drop count 30 (0x18eb94)");
}

std::optional<DebrisLineHit> groundAtZero(const DebrisVec3& from, const DebrisVec3& to) {
    if (from[1] <= 0.0f || to[1] > 0.0f) return std::nullopt;
    const float t = from[1] / (from[1] - to[1]);
    return DebrisLineHit{{from[0] + (to[0] - from[0]) * t, 0.0f, from[2] + (to[2] - from[2]) * t}, {0, 1, 0}};
}

std::optional<DebrisLineHit> nothing(const DebrisVec3&, const DebrisVec3&) { return std::nullopt; }

FallApartMatrix translation(float x, float y, float z) { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1}; }

void motion() {
    expect(kFallApartSlots == 4 && kFallApartParts == 5, "608 bytes at 0x15cec0 / 152 per slot; five parts per slot");
    expect(kFallApartSound == 548 && kFallApartPhaseUpdates == 32, "pool entry +0x94 = 548 (0x15cf50); 32 updates per phase (0x1d3e28, 0x231b70)");
    expect(kFallApartGravity == 0.075f && kFallApartBounce == 0.5f, "0x37716c = 0.075 (0x350708, 0x2e65e8); 0x231bf4 = 0.5");
    GameRandom random(1);
    {
        FallApartBody body;
        body.matrix = translation(0, 0.5f, 0);
        body.velocity = {0.05f, -0.3f, 0};
        const bool rested = fallApartStep(body, {{0, 0, 0}, {0.3f, 0.3f, 0.3f}}, kFallApartBounce, kFallApartGravity, groundAtZero, random);
        expect(rested, "a slow hit with x and z of the new velocity below 0.125 rests for the update");
        expect(near(body.velocity[0], 0.025f) && near(body.velocity[1], 0.15f) && near(body.velocity[2], 0.0f), "reflected and halved, no gravity: (0.025, 0.15, 0)");
        expect(near(body.matrix[13], 0.295918f), "moved until the sphere's leading point touches: 0.5 - 0.3 x 0.3 / |v|");
    }
    {
        FallApartBody body;
        body.matrix = translation(0, 0.5f, 0);
        body.velocity = {0.3f, -0.3f, 0};
        const bool rested = fallApartStep(body, {{0, 0, 0}, {0.3f, 0.3f, 0.3f}}, kFallApartBounce, kFallApartGravity, groundAtZero, random);
        expect(!rested && near(body.velocity[0], 0.15f) && near(body.velocity[1], 0.075f), "x 0.15 after the bounce: moves on, gravity 0.075 off y");
        expect(std::fabs(body.spin[0]) <= 0.25f && std::fabs(body.spin[1]) <= 0.25f && std::fabs(body.spin[2]) <= 0.25f && (body.spin[0] != 0 || body.spin[1] != 0),
               "a bounce draws a new spin (r - 0.5) / 2");
    }
    {
        FallApartBody body;
        body.matrix = translation(0, 2.0f, 0);
        body.velocity = {0, -2.5f, 0};
        expect(!fallApartStep(body, {{0, 0, 0}, {0.3f, 0.3f, 0.3f}}, kFallApartBounce, kFallApartGravity, groundAtZero, random) && near(body.velocity[1], 1.175f),
               "a speed of 2.0 or more never rests (0x1d292c)");
        expect(near(body.matrix[13], 0.7f), "to the contact (0.3), then on by half the rest of the travel ((2.5 - 1.7) x 0.5)");
    }
    {
        FallApartBody body;
        body.matrix = translation(1, 2, 3);
        body.velocity = {0.1f, 0.2f, -0.1f};
        expect(!fallApartStep(body, {{0, 0, 0}, {0.3f, 0.3f, 0.3f}}, kFallApartBounce, kFallApartGravity, nothing, random), "no hit");
        expect(near(body.matrix[12], 1.1f) && near(body.matrix[13], 2.2f) && near(body.matrix[14], 2.9f) && near(body.velocity[1], 0.125f),
               "no hit: moved by the velocity, then 0.075 off y");
    }
    const FallApartMatrix carried = fallApartCarried(translation(10, 6.4f, -3), translation(10, 5, -3), translation(1, 2, 3));
    expect(near(carried[12], 1) && near(carried[13], 3.4f) && near(carried[14], 3), "the head keeps its offset from the character (stand-in SCRIPT-S265)");
}

void pool() {
    GameRandom random(7);
    std::array<FallApartBounds, kFallApartParts> bounds{};
    for (unsigned part = 1; part < kFallApartParts; ++part) bounds[part] = {{0, 0, 0}, {0.3f, 0.3f, 0.3f}};
    FallApartPool pool(random, bounds);
    const auto first = pool.start("goon", translation(10, 5, -3), std::nullopt);
    expect(first == 0u, "the first start takes slot 0");
    const FallApartSlot& slot = pool.slots()[0];
    expect(slot.inUse && slot.owner == "goon" && slot.phase == FallApartPhase::Flying && slot.counter == 32, "slot in use, flying, 32 updates");
    expect(slot.lowerPart == FallApartPart::Legs || slot.lowerPart == FallApartPart::Hips, "the lower part is the legs or the hips (3 + a draw modulo 2)");
    expect(near(slot.parts[0].matrix[13], 4.4f) && near(slot.parts[1].matrix[13], 5.8f) && near(slot.parts[2].matrix[13], 5.8f) &&
               near(slot.parts[unsigned(slot.lowerPart)].matrix[13], 5.8f),
           "the head 0.6 below the character's node, the other parts 0.8 above it");
    unsigned enabled = 0;
    for (unsigned part = 0; part < kFallApartParts; ++part) {
        const FallApartBody& body = slot.parts[part];
        enabled += body.enabled;
        if (!FallApartPool::partUsed(slot, part)) continue;
        expect(near(body.velocity[1], 0.2f) && std::fabs(body.velocity[0]) <= 0.1f && body.velocity[2] == 0.0f, "start velocity ((r - 0.5) 2, 2, z) x 0.1, z kept (0)");
        expect(std::fabs(body.spin[0]) <= 0.125f && std::fabs(body.spin[1]) <= 0.125f && std::fabs(body.spin[2]) <= 0.125f, "start spin (r - 0.5) / 4");
    }
    expect(enabled == 4, "four parts are enabled: head, body, arm and one lower part");
    for (int i = 0; i < 3; ++i) pool.start("other", translation(0, 0, 0), std::nullopt);
    expect(!pool.start("fifth", translation(0, 0, 0), std::nullopt), "a fifth start finds no free slot");
    const FallApartMatrix before = slot.parts[1].matrix;
    const DebrisVec3 velocity = slot.parts[1].velocity;
    pool.update(nothing, 1);
    expect(near(slot.parts[1].matrix[12], before[12] + velocity[0]) && near(slot.parts[1].matrix[13], before[13] + velocity[1]) && near(slot.parts[1].velocity[1], 0.125f),
           "an update moves the parts by their velocity and takes the gravity off");
    for (int i = 1; i < 32; ++i) pool.update(nothing, 1);
    expect(slot.inUse && slot.phase == FallApartPhase::Blinking && slot.counter == 32, "after 32 updates the slot blinks for 32 more");
    pool.update(nothing, 1);
    expect(slot.parts[1].enabled, "an odd clock leaves the parts as they are");
    pool.update(nothing, 2);
    expect(!slot.parts[1].enabled && !slot.parts[0].enabled, "an even clock toggles them");
    for (int i = 0; i < 29; ++i) pool.update(nothing, 2);
    expect(slot.inUse && slot.counter == 1, "still blinking on the 63rd update");
    pool.update(nothing, 2);
    expect(!slot.inUse && !slot.parts[0].enabled && !slot.parts[1].enabled && !slot.parts[2].enabled, "freed with its parts disabled on the 64th update");
    expect(pool.start("again", translation(0, 0, 0), std::nullopt) == 0u, "the freed slot is taken again");
}

std::uint8_t kindOf(const assets::Level& cls) {
    for (const assets::LevelParam& param : cls.defaults)
        if (param.hash == assets::nameHash("ObjectType")) return std::uint8_t(param.value);
    return 0;
}

assets::Level loadLevel(assets::FuseArchive& archive, const char* path) {
    const assets::FuseEntry* entry = archive.find(path);
    if (!entry) {
        std::printf("FAIL: not in archive: %s\n", path);
        ++failures;
        return {};
    }
    return assets::parseLevel(archive.read(*entry));
}

std::optional<CharacterDeathInput> targetInput(const assets::Level& level, const assets::ClassLookup& classOf, const std::string& instance, std::uint8_t kind) {
    for (const assets::LevelObject& object : level.objects) {
        if (object.instance != instance) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls || kindOf(*cls) != kind) return std::nullopt;
        return characterDeathInput(kind, object, *cls);
    }
    return std::nullopt;
}

// The tail of 0x18e93c after the death branch, docs/re/script-natives-world-death.md entry 2026-10-19 "The respawn tail": the stud drop request of
// 0x18ec60 / 0x18eb94, the after-death 0x21e218 and the spawn counter 0x225f5c with the DEAD update 0x1d4ba0 (llvm-objdump; values by hand).
void respawn() {
    const auto dropped = characterStudDropRequest(CharacterStudDrop::Dropped, {1, 2, 3});
    expect(dropped && dropped->mode == StudDropMode::Random && dropped->count == 0 && dropped->skip == 30 && dropped->position[1] == 2.0f &&
               dropped->spread == 1.0f && !dropped->collectAtOnce,
           "0x18ec60: 0x21e2e4(1.0, 1.0, mode 1, count 0, the node position, 0x56d400, 0, 0, 0, 30, 0, 0)");
    const auto thirty = characterStudDropRequest(CharacterStudDrop::Thirty, {1, 2, 3});
    expect(thirty && thirty->mode == StudDropMode::Count && thirty->count == 30, "0x18eb94 (byte +0x12e bit 5): mode 0, count 30");
    expect(!characterStudDropRequest(CharacterStudDrop::None, {1, 2, 3}), "a character that comes back drops nothing");

    expect(characterAfterDeath({}) == CharacterAfterDeath::LeaveEngaged, "0x21e218: not friendly, flags clear: 0x21decc");
    expect(characterAfterDeath({true, false, false}) == CharacterAfterDeath::BackToSpawn, "AIFriendly: back to the spawn place");
    expect(characterAfterDeath({true, true, false}) == CharacterAfterDeath::MillAbout, "AIFriendly and AIMillAbout: 0x22471c");
    expect(characterAfterDeath({false, false, true}) == CharacterAfterDeath::BackToSpawn, "the global flags 0x3771d7 & 0x377540 count like AIFriendly");

    expect(characterComeBackColour(66) == 0xff5f5fffu && characterComeBackColour(70) == 0xffdfdfffu && characterComeBackColour(72) == 0xffffffffu &&
               characterComeBackColour(79) == 0xff1f1fffu && characterComeBackColour(136) == 0xffffffffu,
           "colour: n = counter & 15 folded at 8, green and blue n x 32 | 31 (66: 95, 70: 223, 72: 255, 79: 31), white at 136");
    CharacterComeBack state;
    unsigned leftAt = 0, firstShown = 0, colours = 0;
    bool event = false;
    for (unsigned update = 1; update <= 150; ++update) {
        const CharacterComeBackStep step = characterComeBackStep(state, true, 27);
        if (step.leavesDead) leftAt = update, event = step.raisesEvent;
        if (step.shown && !firstShown) firstShown = update;
        colours += step.colour.has_value();
    }
    expect(leftAt == 65 && event, "the DEAD update leaves for IDLE at the first counter above 64 and raises the event *0x3775ec");
    expect(firstShown == 66 && colours == 71 && state.counter == 136, "shown and coloured for the counters 66..136, then the counter stays at 136");
    CharacterComeBack mech;
    unsigned mechColours = 0;
    bool mechEvent = false;
    for (unsigned update = 1; update <= 70; ++update) {
        const CharacterComeBackStep step = characterComeBackStep(mech, false, 159);
        mechColours += step.colour.has_value();
        mechEvent = mechEvent || step.raisesEvent;
    }
    expect(mechColours == 0 && !mechEvent, "kind 159 gets no colour; without AIFriendly and AIRespawnIfFriendly no event");
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    {
        const assets::Level level = loadLevel(archive, "levels/levels/lvl_02_albatrossisland/lvl_02_albatrossisland_main.blvl");
        for (const char* goon : {"00_GoonFaller", "2-4_GoonA01", "2-4_GoonA02", "2-4_GoonB01", "2-4_GoonB02"}) {
            const auto input = targetInput(level, classOf, goon, 27);
            expect(input.has_value(), std::string(goon) + " is a char_ai_prisoner of kind 27");
            if (!input) continue;
            expect(input->mode == CharacterDeathMode::FallApart && input->minifig && !input->smashColours,
                   std::string(goon) + ": no DisableFallapart, CharacterEnum 142 (not 145), no SmashDebrisColour");
            const CharacterDeathSteps s = characterDeathSteps(*input);
            expect(s.fallApart && s.hidden && !s.comesBack && s.studDrop == CharacterStudDrop::Dropped, std::string(goon) + " falls apart and stays dead");
        }
        const auto faller = targetInput(level, classOf, "00_GoonFaller", 27);
        expect(faller && faller->friendly && !faller->respawnIfFriendly, "00_GoonFaller: instance AIFriendly 1, AIRespawnIfFriendly 0");
        const auto fighter = targetInput(level, classOf, "2-4_GoonA01", 27);
        expect(fighter && !fighter->friendly && fighter->respawnIfFriendly, "2-4_GoonA01: class AIFriendly 0, AIRespawnIfFriendly 1");
        expect(faller && !faller->millAbout && characterAfterDeath({faller->friendly, faller->millAbout, false}) == CharacterAfterDeath::BackToSpawn,
               "00_GoonFaller (no AIMillAbout; probe run 2026-10-19): 0x21e218 puts its node back to its spawn place");
        expect(fighter && !fighter->millAbout && characterAfterDeath({fighter->friendly, fighter->millAbout, false}) == CharacterAfterDeath::LeaveEngaged,
               "2-4_GoonA01: 0x21decc takes it out of the engaged table");
    }
    {
        const assets::Level level = loadLevel(archive, "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl");
        const auto mech = targetInput(level, classOf, "6-9_PlayerMech", 242);
        expect(mech.has_value() && !mech->minifig && !mech->smashColours, "6-9_PlayerMech (chars/char_mech, kind 242): the create 0x15a3bc clears the minifig bit");
        if (mech) {
            const CharacterDeathSteps s = characterDeathSteps(*mech);
            expect(!s.fallApart && !s.smashBricks && s.dropsBodyParts && s.deathSound && s.hidden, "the mech vanishes without bricks");
        }
    }
    {
        const assets::Level charbits = loadLevel(archive, "levels/charbits.blvl");
        const char* parts[] = {"BrokenHead", "BrokenBody", "BrokenArm", "BrokenLegs", "BrokenHips"};
        const char* meshes[] = {"", "char_bits_body", "char_bits_arm", "char_bits_leg", "char_bits_hips"};
        for (unsigned part = 0; part < kFallApartParts; ++part)
            for (unsigned slot = 1; slot <= kFallApartSlots; ++slot) {
                const std::string name = std::string(parts[part]) + "0" + std::to_string(slot);
                bool found = false;
                std::string mesh;
                for (const assets::LevelObject& object : charbits.objects) {
                    if (object.instance != name) continue;
                    found = true;
                    for (const assets::LevelParam& param : object.params)
                        if (param.hash == assets::nameHash("meshname") && !param.blocks.empty()) mesh = reinterpret_cast<const char*>(param.blocks[0].data());
                }
                expect(found && mesh == meshes[part], name + " is in levels/charbits.blvl with meshname '" + meshes[part] + "'");
            }
        float centre[3], half[3];
        expect(assets::modelBounds(archive, assets::meshModelPath("char_bits_body"), centre, half) && near(half[0], 0.359993f) && near(half[1], 0.311641f),
               "char_bits_body bounds half (0.359993, 0.311641, 0.190539): the body part's radius is 0.36");
    }
}

}

int main(int argc, char** argv) {
    steps();
    respawn();
    motion();
    pool();
    if (argc == 3) {
        if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
            std::printf("character-death-test: data not available, skipped\n");
            return failures ? 1 : 77;
        }
        real(argv[1], argv[2]);
    }
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}

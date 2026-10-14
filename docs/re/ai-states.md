# AI states: AISuperJump, face target, run away, stalk, pursuit

Update functions and set-up functions behind the AI natives that are not movement along a path ([ai-natives.md](ai-natives.md), [ai-navigation.md](ai-navigation.md)). Evidence: llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf`; every address is for the Europe v1.0 build ([game-version.md](game-version.md)).

<a id="ts-20261014-212158"></a>

## 2026-10-14T21:21:58+02:00 — AISuperJump set-up 0x213300, state 10 (0x2c5bc4), state 11 arrival value, state numbers of stalk, run away and pursuit

[Corrects ai-navigation.md §2026-10-14T19:47](ai-navigation.md#ts-20261014-194710): ctrl+`0x8c`, which state 11 calls "MinDistance" (default -1.0), is `ATTR_RunSpeed`.

**State table.** Record N of the table built at 0x2a3c24 (global 0x3771f0) is at +16 N; word 0 is the update function. Read from the literals 0x2a3ea4..0x2a3f64: state 2 0x2c4708, 4 and 5 0x2c5d3c, 9 0x2c3a50, **10 0x2c5bc4**, 11 0x2c3bd8, 12 0x2c3d1c, 22 0x2cbe14, **25 0x28b7a0**, **27 0x2c1c90**, 35 and 36 both 0x2ab6cc.

**AISuperJump set-up 0x213300(entity, target, frames, height)** (AISuperJump passes frames = 28 and its height argument). v = target - node position (0x257f14, node = 0x250810(entity+0x3c), position at +0x30); the target is stored at ctrl+0xb8 (0x2554c0); heading ctrl+6 = int(atan2(vx, vz) * 10430.378) and bit 0 of ctrl+0xc are set. distance = sqrt(vx^2 + vz^2) (0x20e968; 0x25b578 is vsqrt). When vy > 0: frames = int((pi - asin(vy / (vy + height))) * 8.9126768) (0x1ff204 jumps to 0x2406c4, a fdlibm asinf: pio2_hi 0x3fc90000, pio2_lo 0x39fdaa22 and a sqrt; literals 0x40490fdb = pi and 0x410e9a53). ctrl+0x308 = distance / float(uint frames), ctrl+0x30c = max(vy, 0) + height. Then 0x2414b4(entity, ctrl+0x50, 72, 0) starts locomotion state 72 and 0x218b38(entity, ctrl, 11) requests state 11. Ported as `planSuperJump` (`src/runtime/ai_super_jump.*`); the formula is hand-evaluated for two cases in `ai-core` (the asin is the C library's, the original's polynomial may differ in the last bit, the result is truncated to an integer).

**State 11 arrival value.** ctrl+0x8c is written by the character set-up 0x236d74 at 0x236ed4 from `0x241280(entity, "ATTR_RunSpeed", 0, *0x37450c)`, default 0.23 ([player-locomotion.md](player-locomotion.md): the player's value read in the oracle). The -1.0 comes from constructors that run before (0x2309c8..0x2309e8 write one literal into +0x84..+0xa0). So state 11 ends when 0.23 (or the class value) is greater than the horizontal distance to the target: a jump lands within 0.23 of its target. Class overrides of `ATTR_RunSpeed` were not read for non-player characters.

**State 10, face target (0x2c5bc4).** Requested by 0x211110(entity, target) (stores 10 at ctrl+0x15, the target at +0xc4, bit 3 of +0xdb when the target is the player entity *0x3772b8, home position +0xb8 = node position). Each tick: node x and z are set to ctrl+0xb8 / ctrl+0xc0 (the home position) and the node is applied (0x24c118); when the target exists and bit 3 of +0xdb is set, +0xc4 is reset to the player entity; when a target with a node exists, heading ctrl+6 and ctrl+10 = int(atan2(dx, dz) * 10430.378) of target - node (0x23df44). Ported as `stepFaceTarget` (`src/runtime/ai_face_target.*`) and run by `AiCore::update`. The state never ends by itself.

**Set-up of the other natives.** AIRunAway 0x228c70(entity, -1.0, target 0x3772b8): requests state 25, sub-structure (ctrl+0xe0) +0xac = target, +0xa8 = -1.0. AIStalkPlayer 0x282b88(entity, distance, target 0x3772b8): requests 27, +0xc4 = target, bit 5 of +0xd9 = bit 4 of +0xdb (bits 3 and 5 cleared first), sub-structure +0xa4 = distance, then 0x1fc080(entity, target). AIPursuit 0x1d4688(entity, first node, takedown): clears bit 4 of byte +0xde of the entity's node flag word and of ctrl+0x16c, requests 35 (takedown non-zero) or 36, bit 5 of +0xd9 = bit 4 of +0xdb. State 2 (0x2c4708, requested by 0x21102c) tests bit 3 of a class table word, calls 0x206ad0, and goes idle (0x2246c0) when the target +0xc4 is invalid (0x245b00) or its ctrl+0x6c fails the tests (324 / 0x2107e0), else continues in 0x2bb558. `AiCore` records the requested state numbers 25 and 27 of the natives.

**Follow-up lookup 0x1f8d5c** (target to attack after a move, shared by the arrival sites 0x2c3c68 and 0x2c630c) is a target search over the player and entity lists (about 0x200 bytes; uses +0xd9 bit 0 and +0xc4); not transcribed. `AiCore` stands in "no target": idle.

Unknowns:
- The flight itself: locomotion state 72 (0x2414b4 -> animation 0x2eafc8) moves the character; no reader of ctrl+0x308 / +0x30c was identified (the candidates 0x2a4af4 and 0x33b01c read a different object). The unit of "frames" (30 Hz updates assumed) and the arc are stood in.
- Bit 6 of ctrl+0x314 (a character below the target counts as arrived) and who sets it; the value of component byte 0 (event argument).
- Update functions of states 25 (0x28b7a0), 27 (0x2c1c90), 35 / 36 (0x2ab6cc) and 2 (0x2bb558 and callees): they use the path follower 0x1faf04 / 0x1fae48 / 0x1faa6c with target and attack code; read only as far as listed above.
- Class values of `ATTR_RunSpeed` for characters other than the player.

# Build-its (props/prop_buildit)

The build-it class (object type 0x15): record, use zone, state machine, completion. Entries are dated; new entries go at the end. For developers; no game assets.

<a id="ts-20261014-235539"></a>

## 2026-10-14T23:55:39+02:00 — Class record, messages and state machine of the build-it (read from the code)

Source: llvm-objdump of `artifacts/game/game.arm.elf` (`llvm-objdump -d --triple=armv6k --mattr=+vfp2`), no REA session. Evidence of the numbers: the next entry.

**Class descriptor.** Built at 0x15d274 (`mov r0,#21; bl 0x246240` at 0x15d2d0), layout of [doors.md](doors.md#ts-20261013-211700): create 0x16c4ec, post-link 0x16459c, destroy 0x176130, update 0x16cc78, slot +0x10 0x16c4e8 (`bx lr`), message handler 0x176160, link 0x16c808, save 0x15d380, restore 0x15d348 ([save-format-state.md §2026-10-14T13:41](save-format-state.md#ts-20261014-134146)). The create function allocates a 0x98-byte record at entity+0x7c.

**Parameters.** Post-link 0x16459c: `Icon` (record +0x20, an object), `BuilditFinalProp` (+0x1c, an object), `PercentTriggerFWD` (+0x8c) and `PercentTriggerBWD` (+0x90), all object references (hash of the lower-cased instance name at block +4). Create 0x16c4ec: `SaveState` (entity +0x82), `ATTR_SFXBitDone` (+0x88), `ATTR_SFXBuilt` (+0x8a), `BIAMaxDist` (+0x84), `BIAPlaceType` (+0x3a), `BuildType` (+0x36 bits 6..7), `NoCameraFollow` (+0x38), `DontUseRadius` (+0x44), `SpawnCheck` (+0x3b, default 1), `TransformDelay` (+0x50, default 0.2), `TransformSpeed` (+0x54, default 0.04), `Reverse` (+0x36 bit 4), `NoRandFreq` (+0x37 bit 2), `TriggerPercent` (+0x94, default 75). Link 0x16c808: `UseBound`, `UseRadius`, `UseExcludeBound` (+0x18), `HideObject`, `MoveViaPoint`. The class file `props/prop_buildit` lists `StudValue` (hash f15d693d, default 25.0) and no bound; `1-1_BI_DonutVanParts` sets only `BuilditFinalProp`, `TriggerObject` and the mesh.

**Object references of `1-1_BI_DonutVanParts`** (name hashes in the instance's blocks, resolved against all object names of lvl_01): `BuilditFinalProp` = `1-1_Donut_Van_Body` (props/prop_prop), `TriggerObject` = `1-1_SO_VanFixed` (scriptobject). The build-it class itself reads no `TriggerObject` in its post-link (0x16459c reads the three above); the parameter is read by many other classes (the string occurs in about 55 places of the executable).

**Record fields used.** +2 state, +4 target state, +0x1c final prop entity, +0x20 Icon, +0x24 array of per-brick records of 228 bytes (+0x0c, +0x18, +0x20, +0x24 progress float, +0x28 start vector, +0xe2 active byte), +0x28 use zone, +0x34 brick count (signed byte), +0x35 current brick (-1 before the first), +0x36 / +0x37 flags (+0x37 bit 4 = built, the byte saved by the type-0x15 save record), +0x40 progress rate, +0x48 rise angle in degrees.

**Messages (handler 0x176160).** 0 and 0x10 are ignored. 3 is the use query (below). 8 resets: enables the build-it (0x242cf0), disables the final prop (0x2428c0), puts all bricks back (progress 0, active 0), brick -1, rate 0.1, rise 0, state 0 and target 2. 0x14 sets flag bit 2 of +0x36 and rate 0.006. 0x16 returns the brick count. 0x44 / 0x45 are the enable / disable notices (0x45 clears +0x3c and +0x37 bit 0). 0xfc forwards sound requests (`ATTR_SFXBitDone`, `ATTR_SFXBuilt`). 0xfe sets state 0 from state 2 (and clears bit 1 of +0x36), 0xff sets bit 1 of +0x36 from state 0 and refreshes the bricks (0x2318c8): state 0 is the deactivated state, state 2 the ready state.

Polarity of the enable helpers: 0x2428c0 sets bit 0 of the entity flags (`EnableObject 0`, hides the node via 0x23f734(node, 0)), 0x242cf0 clears it ([objects.md §2026-10-14T15:05](objects.md#ts-20261014-150541)). Message 8 therefore enables the build-it and disables its final prop; the change to state 11 enables the final prop and disables the build-it. The oracle record agrees (next entry).

**Use query, message 3 (0x176160, case at 0x176238).** Sent every update by the hint code 0x1cba8c for the object whose use zone contains the player (the parameter block holds the player at +0, its character byte at +4 and the action flag at +5, which the query sets to 0). The handler returns 1 when the object is usable, 2 from state 10 up. With the action flag set and state 2, 3 or 4 it sets target 8, asks the player for animation 9 (0x2414b4) and returns 1. It refuses when the player is inside `UseExcludeBound` (+0x18), within `DontUseRadius` (+0x44) of the object, or when the player record +0x13c is non-zero.

**Use zone.** Without `UseBound` and `UseRadius` the link function builds a box (0x23e130 -> 0x23e2a8): centre and half extents of the object's node bounds (0x242df4: node +0xa0 / +0xac minus the node position, halved), each half extent plus the three floats at 0x56d40c (read in the oracle: 1.0, 1.0, 1.0), shape byte 3 (box), active byte 1.

**State machine.** Update 0x16cc78 calls 0x2424d0 with the table {pre 0, post 0x1d428c} and then the state change 0x1aad90; the post function runs only while the state equals the target.
- Post, state 8: progress of the current brick >= 1.0 gives target 9; otherwise the brick's progress += rate and rate += 0.006, capped at 0.5.
- Post, state 9: last brick (+0x35 = count - 1) gives target 10, else target 2.
- Post, state 10: rise angle += 12 degrees, += 24 from 90 degrees on; from 180 degrees on target 11. The final prop's y is its base plus sin(angle) (one metre amplitude).
- Change to 2: 0x2318c8 puts the bricks back and sets rate 0.1. To 8: first brick resets the rate, the next brick becomes current, its active byte is set. To 9: the brick's progress is set to 1.0, active byte cleared, sound `ATTR_SFXBitDone` (0x23ae84). To 10: the build-it's node is hidden (no `Icon`), the final prop's node is shown and its y stored in +0x4c. To 11: sound `ATTR_SFXBuilt`, the final prop enabled (0x242cf0), the build-it disabled (0x2428c0, no `Icon`), bricks reset, brick -1, rate 0.1, rise 0, `StudValue` studs spawned from the object (0x2149ac(object, final prop, value, 1, ...), parameters `SpawnCone`, `SpawnLocal`, `SpawnDir`), +0x37 bit 4 set, target 2.

Unknowns:
- The functions behind 0x206ea0 (idle wobble of the unbuilt bricks, constants 0.2, 0.3, 10, 15 degrees, 1.1), 0x19c9e0 (called every building update), 0x2318c8 (brick transforms), the shape of the brick flight and the camera/animation of the player.
- The meaning of `BuildType`, `BIAPlaceType`, `Icon`, the percent triggers and message 0x14.

<a id="ts-20261014-235937"></a>

## 2026-10-14T23:59:37+02:00 — Build of 1-1_BI_DonutVanParts measured (oracle state m11-4-van-parts)

Source: Azahar oracle, GDB only (no RPC), `--renderer opengl`, New 3DS state `m11-4-van-parts` (user state: the player stands at 876.914, 52.435, 700.427 in front of the parts, mission 1-1 in progress). Commands (scratch scenarios, not kept): `memtrace 120 holdA 0x08a75580:0xa0 0x08e114e0:0x558 0x08a74040:0x60 0x08a797a0:0x60` after `hold A` (object 0x08a74040, record 0x08a75580, brick array 0x08e114e0, final prop 0x08a797a0; addresses of this state only), `memtrace` with `hold B` / `hold X` / `hold Y` for 5 updates each, `hold A` for 3 updates then released with `memtrace 30`, and `ctrace 0x176160,0x1d428c,0x23cee8` for 40 idle updates. The state loaded as described; nothing was poked or teleported.

**Idle.** Every update the object gets message 3 (use query) and the state check 0x1d428c; record at rest: state 2, target 2, brick -1, flags +0x36 = 0x01, +0x37 = 0, rate 0.1, `TransformDelay` 0.2, `TransformSpeed` 0.04, `TriggerPercent` 75, brick count 6, `StudValue` not in the record, final prop entity flags 0x1749 (bit 0 set: disabled), object flags 0x00c0 (bit 0 clear: enabled). The use zone record (0x08e11a50): shape 3, active 1, centre (-0.4423, 0.9966, 0.9803), half (4.2706, 2.5589, 4.0072) relative to the object; the three floats at 0x56d40c are 1.0, 1.0, 1.0.

**Build with A held** (row i = record after i updates; update 1 is the first with A held):
- update 1: state 2, target 8. Update 2: state 8, brick 0, brick active byte 1. Updates 3 to 11: the brick's progress grows by the rate (0.1, 0.206, 0.318, 0.436, 0.56, 0.69, 0.826, 0.968, 1.116) and the rate by 0.006 (0.106 .. 0.154). Update 12: state 9, progress 1.0, active 0. Update 13: state 2 (target 8 again, A still held). Update 14: state 8, brick 1. So 12 updates per brick; the brick starts 13 updates after the previous one started.
- Brick 5 (the last) is in state 9 at update 72; update 73: state 10. The rise angle (+0x48) is 0 at update 73 and 12, 24, 36, 48, 60, 72, 84, 96 at updates 74 to 81, then 120, 144, 168 at updates 82 to 84 (24 degree steps from 90 degrees on). Update 85: state 2 / target 2, brick -1, all brick progress 0, rate 0.1, angle 0, flags +0x37 = 0x10 (built), object flags 0x00c1 (disabled). The final prop's flags read 0x1748 (enabled) at update 85 and 0x1749 (disabled) at update 86 and later: the script `1-1_SO_VanFixed` (calls: `EnableObject 1-1_BrokenVan 0`, `EnableObject 1-1_Donut_Van_Body 0`, `EnableObject 1-1_DonutVan 1`, `EnableObject 1-1_BI_DonutVanParts 0`, `SetActiveWayPoint 1-1_Van_Waypoint_Enter 1`) had run its first calls by update 86, one update after the build ended. Total from the first press: 85 updates (about 2.8 s at 30 updates per second).
- `hold B`, `hold X` and `hold Y` for 5 updates each: state and target stay 2 (no build). Only A starts it.
- A released after 3 updates (press in update 0 of that run): the brick in progress is finished (state 8 until progress >= 1, state 9, then state 2) and the next brick does not start; the state stays 2 with brick 0 for the remaining 20 updates.

**Not traced.** A `ctrace` of the trigger start 0x21a560 and the fire 0x23cee8 over the build was started four times; each attempt (with 0x244fc8, the generic object message send, and 0x23cee8, then 0x21a560 alone) ran for 5 to 10 minutes without finishing on the loaded machine, so the call path that fires `ObjectTriggered` of `1-1_SO_VanFixed` is not recorded. The effect (script calls visible one update after the state change to 11) is.

Unknowns:
- Who fires the event and with which owner: the build-it class has no `TriggerObject` read in its post-link or link; the change to state 11 (0x1ab00c) does not call 0x23cee8 itself in the part read.
- Whether the action flag of the use query comes from the A channel only (B, X and Y do nothing, other buttons and the touch screen were not tried).
- The player's reaction (animation 9, position) during the build was not recorded.

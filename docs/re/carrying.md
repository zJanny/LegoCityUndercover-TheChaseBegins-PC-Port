# Carrying (props/prop_carry, props/prop_carrytarget*)

The carry objects (object type 122, ASCII `z`) and the carry targets (type 123, `{`): class records, the action-button pickup, the carried state, the drop, the placing into a target and the script objects they fire. Entries are dated; new entries go at the end. For developers; no game assets.

Subtopics: [carrying-animation.md](carrying-animation.md)

## Current state

- Carry objects are type 122 (record 0xa0 bytes), targets type 123 (record 0x2c); object references hold the target's name hash. [§2026-10-15T00:17](#ts-20261015-001729)
- Use zones are axis-aligned, translation-only boxes (class `UseBound`, else node bounds plus about (1, 1.2, 1)); the A press walks the registry and sends object message 3 (pickup, drop or place). [§2026-10-15T00:17](#ts-20261015-001729)
- Placing fires the target state change 0x25e354: UseObject off, PlaceObject on, `TriggerObject` via 0x244f40, drop sound. The code-side 0x244f40 enables its object first; the script native `TriggerObject` does not. [§2026-10-15T11:11](#ts-20261015-111115), [§2026-10-15T13:08](#ts-20261015-130800)
- The build-it does fire `TriggerObject` through a wrapper the static search missed. [Corrected by buildit.md §2026-10-15T13:07](buildit.md#ts-20261015-130720)
- A player holding a big object never jumps: the B branch of 0x20b984 skips the jump request (state 4) while ctrl+0x13c is set and the carry test is not 1. Ported. [§2026-10-16T14:53:22](#ts-20261016-145322)
- A carrying player never sprints: 0x1abe60 runs the run timer and the sprint only for the run roles 2, 0x6f and 0x83, and the carried run roles are 552 (big) and 151 (small). Ported. [§2026-10-16T14:13:15](#ts-20261016-141315)
- The carry animations, the release, the end of the carry states and the track events are in [carrying-animation.md](carrying-animation.md).
- Open: sender of message 0x80000003, 0x2256e4 and the reader 0x20bb7c of the carry test, where released boxes land beyond the ray, other carry classes and `TargetObject01..04`.

<a id="ts-20261015-001729"></a>

## 2026-10-15T00:17:29+02:00 — Carry objects and targets: classes, pickup, drop, placing, fired events (REA Ghidra pseudo code, llvm-objdump, oracle states m11-7, m11-8, m11-9)

Corrected by [carrying-animation.md §2026-10-18T10:02:21](carrying-animation.md#ts-20261018-100221): at a fixed dt of 0.033 the place state ends in update 27 and the drop releases in update 8 (the put-down clip and its event track); the 17 and 4 came from runs without a fixed dt.
Corrected by [carrying.md §2026-10-16T14:53:22](#ts-20261016-145322): ctrl+0xe bit 1 is the B press edge (A is bit 2); the registry walk of 0x20b984 sits on the B edge.

Source: REA (Ghidra 12.1.2) pseudo code of `artifacts/game/game.arm.elf` (sha256 5152ed14…0881; `batch_decompile` evidence ev_453627c5…, ev_68f82cbe…, ev_bbcec8e3…, ev_e4f0ed0e…, ev_ee657a3d…, ev_6aed9ef2…, ev_8039a9d1…) and `llvm-objdump` for data words; oracle runs with the GDB stub only (`ctrace`, `watch`, `read`) on the user states `m11-7-collect-donuts`, `m11-8-donuts-in-hand` and `m11-9-delivery-goal` (mission 1-1 in progress: before collecting the donuts, carrying the box, in front of the delivery target). The three user states loaded as described; at `m11-9-delivery-goal` the player stands 3.06 m from the target, outside its use zone (see Placing), which is normal play, not a fault of the state.

**Classes.** Type 122 descriptor built at 0x15d3c8 (`mov r0,#122; bl 0x246240`), layout as in [doors.md](doors.md#ts-20261013-211700): create 0x16d080, post-link (parameter reader) 0x1646c4, destroy 0x1766dc, update 0x16d420, message handler 0x1767c8. Type 123 at 0x182940: create 0x19cd10, post-link 0x18e0f8, update 0x19cf5c, message handler 0x1ab3d4, state-change function 0x25e354. The class files carry ObjectType 122 (`prop_carry.blvl`, classparam 0x06fef256) or 123 (`prop_carrytarget*.blvl`).

**Carry object record (0xa0 bytes at entity+0x7c).** +0x10 the `TriggerPickup` entity, +0x14 `TriggerOnRespawn`, +0x74..+0x80 `TargetObject01..04`, +0x84..+0x90 `TargetBound01..04`, +0xc the carrier (0 when free), +0x18 position, +0x24 orientation, +0x30/+0x3c start position and orientation, +0x98 u16 reset countdown, +0x9a u16 `ResetTime`*30 (5 s gave 150 in the oracle), +0x9c `ATTR_CarryType`, +0x9d `ParticleDef`, +0x9e bit 1 update ran, bit 2 `ATTR_StrengthOnly`, bit 3 `ATTR_SmallObject` (default 1; the donut box sets 0), bit 4 can be taken (create sets it), bit 7 `FreeplayObject`. Create also reads `ATTR_SFXReset` (+0x94) and `ATTR_SFXComplete` (+0x96).

**References.** Object-reference parameters (type 4) hold the name hash of the target instance at block byte +4 (`objectReferenceHash`); the converter printed them as `None` only because it prints the block as text. The post-link readers resolve them with 0x238354 to entities. In lvl_01: `1-1_DonutsDEMO.TriggerPickup` = `1-1_SO_Donuts_PickedUp` (hash 0xc4cc3456); the unnamed parameter 0xc23d689b of the box holds the hash of `1-1_DonutTarget_DEMO`; `1-1_DonutTarget_DEMO.UseObject` = `1-1_DonutsDEMO` (hash 0x8edad609), `.TriggerObject` = `1-1_SO_MissionEnd` (hash 0xe0e857b9).

**Target record (0x2c bytes).** +0x18 `UseObject`, +0x1c `PlaceObject` (a second reference the target enables when used), +0x20 `TriggerObject`, +0x24 `dropAnim` (animation role, 0 here), +0x26 `CarryTargetType` (0), +0x28 `ATTR_SFX_DROP` (72), +2 state, +4 requested state. The class file lists the bound `UseBound` (box, centre (0, 0.5, 0), half (2, 2, 2), node 0xff).

**Interaction zones.** Objects with a use zone are listed in a registry (entries {entity, zone record}); 0x23e130 adds an entity. Without a class bound `UseBound` it builds the zone from the node bounds (0x242df4) and adds the vector *0x56d40c = (1, 1, 1) plus 0.2 on the second component to the half extents. Measured in `m11-8-donuts-in-hand`: zone record of the donut box centre (0, 0.4073, 0), half (1.7607, 1.6064, 1.3828); the model's half extents are (0.762, 0.4035, 0.3813) (difference 1.0, 1.2, 1.0). The test 0x221570 → 0x22f650 is axis-aligned and translation-only (point minus zone centre, absolute value below the half extent, node 0xff): the object's yaw is not applied (a pickup at dx 1.42 from the box turned 90 degrees worked). The build-it zone ([buildit.md](buildit.md)) is the same mechanism.

**Pickup.** Each update the player update 0x20b984 handles the action: when the A press edge (ctrl+0xe bit 1) arrives, the interaction flag ctrl+0x314 bit 6 is set and ctrl+0xe0+0x130 bit 2 is clear, 0x1fee94 walks the registry and sends the object message 3 (0x244fc8) with {player, character byte ctrl+0x2cc, pressed 1}; the first entity whose handler returns 1 takes it. The same message with pressed 0 is the hint query 0x1cba8c (prompt "PICKUP CARRY IT", HUD). Carry handler 0x1767c8 message 3: the object is free (+0xc = 0), +0x9e bit 4 set, `StrengthOnly` objects need character ability 7, ability 6 is needed (0x23a87c; passed in the oracle with character byte 9), the player holds nothing (ctrl+0x13c = 0); with pressed 1 it casts a ray (0x22f134, 3 m) and, unless ctrl+0x316 & 0xc0, sets ctrl+0x130 = the object, turns ctrl+10 and ctrl+6 to the object (angle*10430.378) and enters locomotion state 0xa7 ("PICKUP CARRY IT": update 0x2b3494 which returns to state 1 when the animation ends, enter 0x29b170 playing animation role 0x22c for a big object or 0x9b for a small one). Oracle at 1.57 m from the box (inside the zone): 0x29b170 ran once on A; with B, X or Y nothing happened. The attachment itself is the routine 0x28db30: ctrl+0x13c = the object, the object's node `carryhandle` is hung on a player node (`weaponleft`) and message 0x11 is sent to the object; the same routine is the script native `PickupObject` (0x1bf9ac). Message 0x11 in the object handler stores the carrier at +0xc and calls 0x244f40(+0x10, carrier): message 0xff to the `TriggerPickup` entity (the same call as the script native `TriggerObject 1`, which fires its ObjectTriggered event) and the broadcast 0x23cee8.

**Carried state.** The player stays in locomotion state 1 (idle) with ctrl+0x13c set (oracle `m11-8`: ctrl+0x6c = ctrl+0x6e = 1). `0x225754(player)` returns 2 when ctrl+0x13c is a type 122 object whose +0x9e bit 3 is clear, 1 when it is set, else 0; the vehicle search 0x225d94 refuses while it is 2. Other callers of the test: 0x186b44, 0x19de8c, 0x19fa38, 0x1a2294, 0x203e14, 0x2061f8, 0x206540, 0x20bb7c (a carried object blocks the other action unless the mode is 1), 0x219644 (animation roles), 0x21e064; not read.

**Drop (no target in the zone).** 0x2011b0 (action button, states 2 and 3 through 0x26aca8): with ctrl+0x13c set and no matching target it enters state 0xa8 ("DROP CARRY IT": enter 0x283118 plays role 0x22e for a big object, 0x9d otherwise; update 0x2b3494). Oracle at `m11-9` (outside the use zone), A pressed: the object handler got message 3 (returns 0), 0x2414b4(168) ran, and 4 updates later the state hook (dispatcher 0x32bf58, hook 0x20101c, then 0x2256e4) released the object: 0x2256e4 detaches the node, sends message 0x12 to the object and clears ctrl+0x13c. Message 0x12 in the object handler casts a ray ±3 m down from the node (0x22b108), sets the node matrix, clears the carrier, and sets the countdown +0x98 = +0x9a. The update then counts down; below 2*30 updates the object blinks (0x23f124); at 0 it plays effect 0xb (0x23de24) and returns to the start position (0x22fc84).

**Placing.** Target message 3 (0x1ab3d4), reached through the same registry when the player is inside `UseBound` of the target: the target needs `UseObject` set and the player carrying exactly that object (else 0); for `CarryTargetType` 0 with pressed 1 it sets ctrl+0x134 = target, enters state 0xd6 ("DROP TARGET", registered at 0x18295c: enter 0x2768bc, update 0x29aa5c, hook 0x26b084 → 0x20101c) and turns ctrl+10 towards the target or the PlaceObject node. Enter 0x2768bc copies ctrl+0x134 to ctrl+0x130 and plays the animation role `dropAnim` (+0x24) when set, else role 0x22e (big carried object) or 0x9d. Oracle (player at 1.67 m from the target, A held 2 updates; counted from the update the state was entered): target handler and state 0xd6 enter at update 0, release (0x2256e4, message 0x12 to the box) at update 8, state 1 again at update 17. The target state change 0x25e354 runs at the update after the release (update 9): requested state 1 gives 0x2428c0 on `UseObject` (the box disappears, [buildit.md](buildit.md#ts-20261014-235539) polarity note), 0x242cf0 on `PlaceObject`, 0x244f40(`TriggerObject`, target) (message 0xff and the ObjectTriggered broadcast 0x23cee8), `FadeAfterUse`, `SpawnDebris`, `DropParticle01/02`, sound `ATTR_SFX_DROP` (0x23ae84) and 0x236624 on the target. Oracle trace: 0x244f40 with r0 = the script object and r1 = the target at update 9.

**What fires which script (lvl_01).** Pickup: `1-1_SO_Donuts_PickedUp` (trigger 5, ObjectTriggered: tip texts). Placing: `1-1_SO_MissionEnd` (trigger 6, ObjectTriggered; the script ends the mission) at update 9 after the press; and trigger 1 `1-1_Deliver_Donuts` (ObjectBoundEntered of `1-1_DonutsDEMO` into the bound `1-1_Deliver_Donuts`: box shape, node 47, centre (882.628, 18.674, 1121.1), half (0.688, 2.285, 0.895); the script shows a tip) is the generic trigger pass: it follows the box position and fires on the update the box enters the bound, which can be while it is carried. `1-1_Carry_Object_Tutorial` (started by `1-1_Waypoint_Donuts`) enables the box, switches the waypoint and the placeholder `1-1_DonutsDEMO01` off and enables the trigger `1-1_Deliver_Donuts`.

**Port.** `src/runtime/carry.*` transcribes the zones, the A-press choice (pick up when free, place when inside the matching target zone, else drop), the timers above, the reset countdown and the actions; fired events use `ScriptRunner::fireObjectTriggered`. Stand-ins and open items: [level-objects-carry.md](../plans/features/level-objects-carry.md).

Unknowns:
- The update at which the attachment (0x28db30) happens after the press and the end of state 0xa7: the pickup oracle runs did not start (the waypoint trigger of step 9 did not fire at the standing points tried, 0.56 to 1.1 m from `1-1_Waypoint_Donuts`); the port copies the measured place timing.
- The release update of a drop is measured once (4 updates after state 0xa8 starts); the animation event that calls the state hook was not found.
- What writes the target's requested state 1 (the watchpoint on +4 stopped inside the release, update 8); the order of the registry entries when several zones contain the player; the ray test in message 3 (0x22f134) and its use as a veto.
- The character ability table behind 0x23a87c (abilities 6 and 7); the port takes ability 6 as given and treats ability 7 (strength) as missing.
- Walking speed while carrying and what else the player cannot do (the readers of 0x225754 in the locomotion code were not read).
- Where the dropped box lands (ray in message 0x12), the turn of the player towards the object or the target, the sounds `carryit_pickup_big_01` and `carryit_putdown_big_01` and which event plays them.
- Use of `TriggerOnRespawn`, `TargetObject01..04`, `TargetBound01..04` of the carry class and of the unnamed parameter 0xc23d689b (value: the target instance).

<a id="ts-20261015-111115"></a>

## 2026-10-15T11:11:15+02:00 — The code-side ObjectTriggered call 0x244f40: what it does, who calls it for carry objects and build-its (llvm-objdump)

Corrected by [buildit.md §2026-10-15T13:07](buildit.md#ts-20261015-130720): the build-it fires `TriggerObject` through the reference-trigger step 0x23211c, a wrapper the static search here did not follow. The other callers of 0x22fc84 and PlaceObject: see the next entries of this file.

Source: `llvm-objdump -d --triple=armv6k --mattr=+vfp2` of `artifacts/game/game.arm.elf` (no REA session, no emulator run). Extends [the carry entry](#ts-20261015-001729).

**0x244f40(object, instigator).** Three steps: (1) 0x242cf0(object): when the entity flag bit 0 (+0x10) is set it clears it and runs the enable work (the same helper the build-it uses on its final prop); (2) 0x244fc8(object, 0xff, instigator), the generic object message send (dispatch through the class table entry +0x14 of the class byte at entity+0x12); (3) 0x23cee8(`*0x3775d0` = ObjectTriggered, object, instigator, 0xff, 0). 0x244f88 is the untrigger twin without the enable (message 0xfe, `*0x3775d4`). 0x23cee8 finds the trigger record with 0x23cff4(type, owner entity) (scene list +0x2f4/+0x2f8, entries with enable byte set, +8 = owner, +4 = type) and calls 0x21a560(trigger, instigator, 0xffff, ...); a name loop for the `Player1`/`Player%d` variants runs first. The owner is therefore the entity named by the reference parameter itself, the instigator is the second argument (the carrier, the target, ...). The port keys triggers by the owner's instance name, which is the same lookup; it has no instigator.

**The script native TriggerObject (0x1cddf8) is not 0x244f40.** It sends 0xff / 0xfe with 0x244fc8 and calls 0x23cee8 with the object as its own instigator; it does not enable the object. The code-side callers (about 100 call sites of 0x244f40) enable first.

**Carry object (type 122).** Message 0x11 (attach, handler 0x1767c8, call at 0x176a48): 0x244f40(+0x10 `TriggerPickup`, carrier = the message argument). The respawn function 0x22fc84 (called from the update at 0x16d520 and 0x16d5f8, and from the message handler at 0x1769e4 and 0x176d40) starts with 0x244f40(+0x14 `TriggerOnRespawn`, the carry object) when the reference is set, then places the object at its start. The countdown the port models (reset after `ResetTime`) is one of these callers; the other callers were not matched to messages.

**Carry target (type 123).** State change 0x25e354 to requested state 1: 0x2428c0(`UseObject` +0x18), 0x242cf0(`PlaceObject` +0x1c), 0x244f40(`TriggerObject` +0x20, the target) at 0x25e3a4, then the drop sound and 0x236624. Confirms the order in [the carry entry](#ts-20261015-001729).

**Build-it, type 21 (`props/prop_buildit`).** No function of the class calls 0x244f40, 0x244f88, 0x244fc8, 0x23cee8 or 0x21a560 for the build-it's own `TriggerObject`: create 0x16c4ec, link 0x16c808, update 0x16cc78, destroy 0x176130, handler 0x176160, the state-check table function 0x1d428c and the state change 0x1aad90 were searched for those calls. The only object message the state change sends is message 8 (reset) at 0x1ab360 to each carry object (class byte 122, not disabled) found by the sphere query 0x237f30 around the final prop when the build ends (flag +0x3b `SpawnCheck`). So the build of 1-1_BI_DonutVanParts reaches `1-1_SO_VanFixed` by a path not found in the class code ([buildit.md](buildit.md#ts-20261014-235539) measured the effect: script calls visible one update after the build ends). A different function does fire `TriggerObject` on completion: 0x26d550 (a state change with the same state/requested-state layout at record +2/+4; its registration was not found, likely the super build-it, type 98 of [save-format-state.md](save-format-state.md)): at its state-3 branch it runs 0x2428c0 on +0x24 and on itself, 0x242cf0 on the final prop (+0x1c), 0x244f40(+0x28, self) when set, 0x242cf0 on +0x2c, and a second 0x244f40(`TriggerObject` read by name with 0x24242c, self).

**Other callers of 0x244f40.** A message handler at 0x1bc4d4 forwards message 0xff to every child entity (entity +0x14 first child index, +0x16 count, scene list +0x24) with 0x244f40(child, self) and 0xfe with 0x244f88; the class was not identified (candidate: `scriptobject`, which would make a triggered script object trigger its child script objects). Class-code callers are spread over the executable; 0x25de86 and about 50 other code strings hold `TriggerObject`.

Unknowns:
- Which class code (or which generic step) fires `TriggerObject` for a type 21 build-it; the registration of 0x26d550.
- Whether the class at 0x1bc4d4 is `scriptobject` and whether lvl_01 script objects have children.
- The other callers of 0x22fc84 (0x1769e4, 0x176d40) and what raises them; `TargetObject01..04`, `TargetBound01..04` and the other carry classes of lvl_01 were not read.
- The instigator is not used by the port; what 0x21a560 stores of it was not read here.

<a id="ts-20261015-130800"></a>

## 2026-10-15T13:08:00+02:00 — Carry target PlaceObject and the callers of the respawn function 0x22fc84 (llvm-objdump)

Corrected by [particles.md §2026-10-18T09:40](particles.md#ts-20261018-094004): 0x23aebc starts the debris burst (0x20c938), not a sound
Extends [the 0x244f40 entry](#ts-20261015-111115); the build-it question of that entry is answered in [buildit.md §2026-10-15T13:07](buildit.md#ts-20261015-130720).

- **PlaceObject.** The carry target state change enables the `PlaceObject` reference (record +0x1c) with 0x242cf0 after disabling `UseObject` and before the `TriggerObject` call. The port now erases it from the disabled set in `applyCarryActions` (test `carry`).
- **0x22fc84 callers in the carry object handler 0x1767c8.** The call at 0x1769e4 belongs to message 0x80000003 (case at 0x1769a0): it runs when the entity word +0xc has bit 5, the object byte +0x9e bit 0 is clear and the float at +0x70 is above a constant (0x176ad0); not identified further. The call at 0x176d40 belongs to message 8, the reset the build-it state change sends to carry objects near its final prop: it plays a sound (0x23aebc) when a flag byte is set, then respawns the object (r2 = 0, r1 = 0x100). Both start with `TriggerOnRespawn` (0x244f40) when the reference is set, like the countdown caller.
- The port does not send message 8 from the build-it to nearby carry objects; the sphere query 0x237f30 radius was not read.

Unknowns:
- What sends message 0x80000003 and the meaning of +0x70 / +0x9e bit 0.
- The sphere of the message 8 query (flag +0x3b `SpawnCheck`) and whether mission 1-1 has a carry object in it.

<a id="ts-20261016-141315"></a>

## 2026-10-16T14:13:15+02:00 — No sprint while carrying: the run-role gate of 0x1abe60 (REA with Ghidra on `artifacts/game/game.arm.elf`: 0x1abe60, 0x19d568, 0x1ab910, 0x2061f8 / 0x20614c, 0x206540 / 0x206510, 0x19de38; no emulator)

User report (2026-10-16): while carrying the donut box Chase walks with the carrying clip, but after a while he starts to run and the animation breaks; in the original he cannot run while carrying.

- **The gate.** The run update 0x1abe60 asks `role = 0x219614(entity, 2)` and runs its whole speed block only when `role` is 2, 0x83 or 0x6f (and byte 0x51640e is not 1). That block sets ctrl+0x94 to 1.0, applies the slope term, adds dt to the run timer ctrl+0x24c, and above 1.0 switches to role 12 (sprint) with ctrl+0x94 x 1.5 (x 1.3 when 0x219b80 and 0x2cd540 answer). Without the block it only keeps the clip of `role` playing (0x21941c).
- **The carried run roles.** Record 2 of the role table is (2, 111, 131, 151, 162, 552, 272) ([player-locomotion.md §2026-10-13T17:02:03](player-locomotion.md#ts-20261013-170203)). 0x219614 picks index 5 (552 `supercarry_walk`) for a big object and index 3 (151 `itemleft_run`) for a small one ([§2026-10-16T09:40:04](carrying-animation.md#ts-20261016-094004)). Neither is 2, 0x6f or 0x83, so a carrying player never runs the timer and never sprints, and ctrl+0x94 keeps the 1.0 that the run-state entry 0x19d568 set (0x19d568 also keeps a sprint going when ctrl+0x94 is above 1.0 in states 0, 4 and 5, and otherwise resets the timer to 0). 0x1ab910 (an exit of the run state) also clears the timer.
- **Port.** `runRoleSprints` and `LocomotionState::runRole` (`src/runtime/player_locomotion.*`); `chase-game` sets the run role from the carried variant (`CarryAnimations::clip(2, mode)`). Before, the port ran the timer whatever the role and switched to the plain sprint clip after 1 s of full stick. Tests `runtime-core` (60 updates with role 552 and 151) and `carry-animation-real-data` (the real roles of record 2 do not sprint).
- **Other readers of the carry test 0x225754 read here.** 0x20614c (via 0x2061f8): when the player uses an object of type 0xc7 (-0x39) that 0x210f30 accepts and carries a big object, 0x2256e4(player, ctrl, 0) runs first (not read; probably drops the carried object). 0x206510 (via 0x206540): the use of an object of type 0xbb (-0x45) by the player needs ctrl+0x13c clear or the carry test 1 (small), and an entity byte +0x12 other than 0x9f. 0x19de38: a push or bump handler (messages 3 and 4) that skips a sender carrying a big object. None of them is ported.

Unknowns:
- Byte 0x51640e (when 1, the speed block is skipped for every role) and the 1.3 sprint scale (0x219b80, 0x2cd540).
- 0x2256e4 and the object types 0xc7 and 0xbb of 0x20614c and 0x206510; the last unread reader 0x20bb7c.

<a id="ts-20261016-145322"></a>

## 2026-10-16T14:53:22+02:00 — No jump with a big object: the B branch of 0x20b984 (REA with Ghidra on `artifacts/game/game.arm.elf`; chase-game run; no emulator)

[Corrects §2026-10-15T00:17:29](#ts-20261015-001729) (the pickup paragraph calls ctrl+0xe bit 1 the A press edge).

User report (2026-10-16): after the sprint fix, jumping with the donut box still breaks the carrying animation.

- **The press bits.** ctrl+0xe bit 1 (value 2) is the B press edge and bit 2 (value 4) the A press edge ([player-falling-jumping.md](player-falling-jumping.md), "Which button"). In 0x20b984 the A edge goes to 0x2011b0 (states 1 to 3; through 0x26aca8 otherwise), the B edge to the rest of the function.
- **The jump gate.** On the B edge, with the interaction flag ctrl+0x314 bit 6 set and `*(ctrl+0xe0)+0x130` bit 2 clear, 0x1fee94 first offers the press to a registered object (message 3); when none takes it and the entity is the player (`*0x3772c0`), the jump request `0x2414b4(entity, ctrl+0x50, 4, 0)` (state 4) is skipped when ctrl+0x13c is set and the carry test 0x225754 is not 1, when the state is 0x101, when `*(ctrl+0x1e8)+0x1c` is 9, when the object at `*(ctrl+0x1e4)+0x10` has type byte 0xb7, or when the probe 0x203c10 (reach ctrl+0x88 x 0.25, from 0x23d4c0 with 3.0) finds an obstacle whose normal y is below 0.6088. So the player never jumps with a big object (carry test 2) and still jumps with a small one (test 1).
- **Port.** `CarrySet::allowsJump` (false while a big object is held) gates the jump press in `chase-game`; test `carry` (big object: no jump, after the drop and with a small object: jump). Run: the mission 1-1 route to the donut box, A, 70 updates, B and 10 updates: the height stays 23.94. Before, the port jumped and played the plain jump clip, which broke the carrying pose.

Unknowns:
- Which press the registry walk 0x1fee94 of carry objects answers in the original: in 0x20b984 it sits on the B edge, while the oracle pickups of mission 1-1 were made with A (0x2011b0 also turns the player to the box).
- The state 0x101, the value 9 at `*(ctrl+0x1e8)+0x1c`, the type 0xb7 and the obstacle probe 0x203c10 are not ported.

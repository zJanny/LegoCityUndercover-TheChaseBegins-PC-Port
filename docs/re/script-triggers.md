# Script triggers: check functions per type

What each trigger type checks in the trigger pass 0x1746c0, and which types the mission 1 levels use. The record layout, the queue 0x21a560 and the first transcription are in [scripts.md](scripts.md); the start rule of the records is in [scene-start.md](scene-start.md). Items: [script-system-triggers.md](../plans/features/script-system-triggers.md).

<a id="ts-20261014-155345"></a>

## 2026-10-14T15:53:45+02:00 — Check functions of all bound trigger types, census of lvl_01 and lvl_00, oracle runs of BoundLeft and PlayerOnFootBoundEntered (llvm-objdump on game.arm.elf, Azahar oracle `street-at-entrance` New 3DS, port 24810, GDB `trap` and `read` only, no RPC)

Replica: `src/runtime/script_triggers.*` (`classifyTriggerEvent`, `applyTriggerCheck`), `ScriptRunner::checkTriggers` (src/runtime/script_runner.cpp). Tests: `script-triggers` (synthetic, one case per branch below), `script-runner` (`objectBoundTriggersUseTheObjectPosition`), `script-triggers-real-data` (lvl_01 police entrance, oracle values below). No REA session was run for this entry; every address was read with llvm-objdump.

### Census (scratch tool over `Level::triggers`, main level files)

- lvl_01 (241): ObjectTriggered 123, BoundEntered 62, ObjectBoundEntered 23, PlayerOnFootBoundEntered 10, PlayerVehicleBoundEntered 5, AIMove 4, BoundLeft 4, CharacterArrested 3, CharacterArrestable 2, Player1BoundEntered 1, CharacterAvoidArrest 1, PlayerVehicleEntered 1, PursuitEnded 1, VehicleChaseEnded 1.
- lvl_00 (53): ObjectTriggered 26, BoundEntered 22, BoundLeft 5.
- Not used in either: BoundInside, BoundOutside, ObjectBoundInside, ObjectBoundLeft, BoundBothEntered, Player2BoundEntered, CharacterCulled.
- Records with an object name on a bound event: 1 BoundEntered group `PlayerRaceVehicle` (12 blockers, triggers 208 to 219; only the disabled test of the owner uses the name, the bound test is the player's), 23 ObjectBoundEntered (objects `1-1_DonutsDEMO`, `1-2_Doorlock_Holmes`, `1-2_Dogtruck`, `1-3_Ledge_Platform`, `1-4_AI_HeadRobber`, `PlayerRaceVehicle`, `OpponentRaceVehicle`, `1-8_AI_Tony_Knuckles`).

### The pass 0x1746c0

- For every trigger i of the scene array (+0x12f4 count, +0x12f8 pointers): skipped when the enable byte +0 is 0 or when flag bit 0 (`ldrh [owner+0x10] & 1`, the disabled flag) of the owner entity (+8) is set; otherwise `blx [[trigger+4]+0x28]` (the check function of the type, 0x16babc / 0x15b888). Then the queue (scene +0x12fc / +0x1300) becomes threads from the last entry with 0x22ce9c.
- Every check function ends the same way: `+0x1c = state`, `+1 = 0` (skip cleared). Skip is 1 until a check has stored a state once.

### Check functions (point = node translation (node+0x30 of 0x250810(entity+0x3c)) minus the level root's translation, test 0x22f650(point, bound, level data))

- BoundEntered 0x28aef4, BoundBothEntered 0x2b0cd0, Player1BoundEntered 0x2c185c: `n = 0x201a54(owner, bound)` is the player entity [0x3772c0] when bound byte +0xa and +0xb are both set and the player's node lies in the bound, else 0 (BoundBothEntered takes [0x3772b8] after the same test). Fires `0x21a560(trigger, who, 0xffff, ...)` when skip is 0, n is set and +0x1c differs from n; stores n.
- BoundLeft 0x26a20c: same n; fires when skip is 0, n is 0 and +0x1c is not 0; stores n. A switched-off bound (+0xa = 0) or an unloaded room (+0xb = 0) reads as n = 0, so such a bound never stores 1 and never fires.
- BoundInside 0x282028: fires whenever n is set, with no skip test, every pass; BoundOutside 0x28af68: fires whenever n is 0, no skip test; both store n.
- PlayerOnFootBoundEntered 0x2ca6b8: when 0x24255c([0x3772b8]) is 0 it jumps to 0x2c185c, else it returns without touching the record. PlayerVehicleBoundEntered 0x2cb578: the opposite test. 0x24255c reads the controller state (entity+0x7c, u16 +0x6c): 0x11c or 0x19b returns the vehicle pointer (+0x130), else 0.
- Player2BoundEntered 0x2c194c: only clears +1 and +0x1c; never fires.
- ObjectBoundEntered 0x2bc67c, ObjectBoundLeft 0x2aae30, ObjectBoundInside 0x2b7ecc: object = trigger+8; when its class byte +0x12 is 0x41 and its name hash +8 equals the hash of the string "Player1" (0x359c0c) the player [0x3772c0] is used instead. Return at once, changing nothing, when bound byte +0xa is 0 (not +0xb) or `object+0x10 & 0x21` is set. Point is the object's node translation. Entered: fires on skip 0, inside and +0x1c different from inside, stores inside. Left: stores and compares the outside flag (inside ^ 1); fires when skip is 0, outside and +0x1c differs. Inside: fires on skip 0 and inside; stores inside. The fourth argument of 0x21a560 is `object+0xc & 0x100000 >> 20` (the forcing flag).
- CharacterCulled 0x2aadc8: fires when `0x23de70(trigger+8)` is set and the stored word is 0 (character presence); not modelled.
- Types fired by code (not by the pass): ObjectTriggered *0x3775d0, ObjectUntriggered *0x3775d4 (TriggerObject), and AIMove, PlayerVehicleEntered, Character*, PursuitEnded, VehicleChaseEnded, Reset and the other names through 0x23cee8 -> 0x23cff4 -> 0x21a560 from the AI, vehicle and character code.

### Oracle (state `street-at-entrance`, New 3DS; traps 0x26a20c and 0x2ca6b8 dump the record at r0; evidence files `artifacts/oracle/agentTT-out/tt1a..tt1e.txt`, `tt2a..tt2d.txt`, `tt4a.txt`)

- Walk from (903.72, 16.57, 1113.33) into `Police_Station_Transition_Lower` (pad: stick (60, 100) for 6 updates, (140, 20) for 4, (0, 100), then A on the dialog). Triggers 203 (BoundLeft `Police_ExtToInt_SwitchOn`) and 204 (PlayerOnFootBoundEntered `Police_ExtToInt`): enable 1, skip 0, stored word 0 at every pass until the scenario's frame 15; at the entry of frame 16 both hold 1 (the check of frame 15 saw the player inside). The dialog opens; while it is open the trap stops 0 times: the trigger pass does not run.
- After A: the entry of frame 24 shows 203 and 204 with word 1; the entry of frame 25 shows 204 with word 0 (the check of frame 24, after the placement, stored outside) and 203 is no longer visited for the rest of the run, so its enable byte went 0 (inference from the missing stops: the script `Police_ExtToInt_SwitchOn` that BoundLeft starts switches triggers, see [scripts.md](scripts.md)). BoundLeft therefore fired at the check where the stored 1 turned to 0.
- Why a teleport did not fire BoundLeft: reading the bound records of the four BoundLeft triggers (`read` of the record at trigger+0x14, bytes +8..+0xb): bound 36 `1-1_Get_Back_To_Donuts` +0xa = 0, bound 42 `1-9_Chase_Leave_Bound` +0xa = 0, bound 100 `Music_Bound_01` +0xa = 0 and +0xb = 0 (room17 not loaded), bound 87 `Police_Station_Transition_Lower` +0xa = 1, +0xb = 1. A teleport into the switched-off bound 42 (891.47, 16.6, 1112.14), three updates inside and back out left trigger 128's word 0 at every pass (`tt2a..tt2d`): it never saw "inside". The edge needs a stored inside, so a leave type fires only for an enabled, loaded bound.
- A teleport into the enabled bound 87 (906.0, 16.57, 1115.5, `tt4a`): trigger 203's word is 0 at the entry of the first pass and 1 at the entry of the second: one pass is enough, the skip byte was already clear.

### Port behaviour

- `applyTriggerCheck` transcribes the table above for BoundEntered (and its aliases), BoundLeft, BoundInside, BoundOutside, PlayerOnFoot / PlayerVehicle (the port has no vehicle: always on foot), Player2, ObjectBoundEntered / Left / Inside. The object's position comes from the placement of the named object (`objectStart`), the object bound test needs only +0xa.
- Fired and not modelled: AIMove, Character*, PlayerVehicleEntered, PursuitEnded, VehicleChaseEnded, CharacterCulled; `chase-game` still lists them once.

Unknowns:
- The meaning of object flag bit 5 (0x20) in the object checks and of bit 0x100000 of `object+0xc` (forces a start in 0x21a560); both are taken as clear.
- How the owner entity (trigger+8) of a trigger whose object lives in a room that is not loaded is represented in the pass (null cannot be dereferenced at 0x1746f0).
- What a player in a vehicle does to the records of the on-foot variants over time (they are not touched while in a vehicle), and the position used for the vehicle variants (the player node, not the vehicle).
- Who calls CharacterCulled and what 0x23de70 reads.

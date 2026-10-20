# Level transitions

How the original switches between the police station and the street, and what the port still lacks for it.

## Current state

- The only entry moves the open details of TRANS-5 (natives the transition scripts need) and TRANS-8 (loading page code) out of the feature files. [§2026-10-14T13:39:58](#ts-20261014-133958)
- `WarningDialogBox`, `PositionObject` (player), `SceneChange` and `FadeScreen` (stand-in) run in `chase-game`; nine natives (`PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`) still have no handler. [§2026-10-14T13:39:58](#ts-20261014-133958)
- The loading page object is vtable 0x361fb4 (enter 0x2e1324); the A wait is in the generic menu widget (state table 0x2a8540). [§2026-10-14T13:39:58](#ts-20261014-133958)
- Corrected by the entry below: of the nine natives, only `CharSetAllowVehicleEntry` and `RelockChasePlainClothes` had no handler, and they have one now (stand-ins); the 31 transition scripts of the shipped levels call only handled natives except `SetTrafficMaxVehicles`, `PlayCredits`, `RoomStreamingWait`, `PlayScriptOnStartup` and `StudMagnetScene`. [§2026-10-19T15:54:20](#ts-20261019-155420)
- Manager slot +0x28 (destination point in a loaded scene) pushes the same mode node as +0x2c without the loading page, and 0x303480 then only moves the player; the lvl_00 lifts and six master objects use it; no OnSceneExit/OnSceneEnter on that path. [§2026-10-20T09:16:06](#ts-20261020-091606)
- Open: which input read gives the A edge on the loading page, what the badge and line do while loading, the text key and the top key art source.

<a id="ts-20261014-133958"></a>

## 2026-10-14T13:39:58+02:00 — Details moved from feature items TRANS-5, TRANS-8

Corrected by [level-transitions.md §2026-10-19T15:54:20](#ts-20261019-155420): of the nine natives listed for TRANS-5 only two lacked a handler, and they have one now.

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **TRANS-5** (level-transitions.md): Natives needed by the transition scripts: `WarningDialogBox`, `PositionObject` (player), `SceneChange` and `FadeScreen` (stand-in) run in `chase-game` (SCRIPT-7); still without handler: `PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`. evidence: [scripts-runtime.md §2026-10-14T10:54](scripts-runtime.md#ts-20261014-105432). needs: SCRIPT-1.
- **TRANS-8** (level-transitions.md): Loading page code: which input read gives the A edge, whether A counts before the load finishes, what the badge and line do while loading, the text key and the source of the top key art. The page object is vtable 0x361fb4 (enter 0x2e1324), the A wait is in the generic menu widget (0x2a8540 state table). evidence: [streaming-transitions.md §2026-10-14T12:02](streaming-transitions.md#ts-20261014-120258). blocks: TRANS-S3, TRANS-S4. needs: FE-3.

<a id="ts-20261019-155420"></a>

## 2026-10-19T15:54:20+02:00 — The natives of the transition scripts

[Corrects level-transitions.md §2026-10-14T13:39:58](#ts-20261014-133958): the list of nine natives without a handler is out of date.

Method: `parseLevel` of every `*.blvl`, all scripts that call `SceneChange`, `CharSetAllowVehicleEntry` or `RelockChasePlainClothes` (31 scripts in lvl_01 to lvl_07), each call looked up with `scriptFunctionSupport`. No emulator run.

- Calls without a handler in those scripts: `SetTrafficMaxVehicles` (lvl_01 script d5a16ff1), `PlayCredits` and `PlayScriptOnStartup` (lvl_07 b26fa928), `RoomStreamingWait` and `StudMagnetScene` (lvl_07 6645da8e).
- Of the nine natives, `PositionPlayer`, `SnapCamera`, `AIMove`, `AIEnable`, `DCamActive` run as stand-ins, `StartDCam` and `WaitForDCam` run, `CharSetAllowVehicleEntry` and `RelockChasePlainClothes` record their effect since batch 35 ([script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302)).
- The other stand-ins these scripts use most: `AIEnable` 27 calls, `FadeScreen` 25, `PositionPlayer` 14, `AttachObject` 13, `SnapCamera` 8.

Unknowns: what the five unhandled natives do in the original (batches SCRIPT-29, SCRIPT-36, SCRIPT-38, SCRIPT-39).

<a id="ts-20261020-091606"></a>

## 2026-10-20T09:16:06+02:00 — Manager slot +0x28 and entries inside a loaded level (TRANS-11, TRANS-6)

Method: `llvm-objdump --mcpu=arm1176jzf-s` of `artifacts/game/game.arm.elf` (0x3038a0, 0x3037e8, 0x303480, 0x30378c, 0x1c47e8, 0x1a4c3c, 0x25d26c, 0x238354, 0x2159a4, 0x2284bc); level data from `parseLevel` and `tools/convert_levels.py`. No REA session and no emulator run; nothing below was measured on the oracle.

**Slot +0x28 (0x3038a0) against slot +0x2c (0x3037e8)**
- Both need `[[0x37758c]+0x30]` or r2, run `0x2c2a10`, store the destination reference `[[entity+0x7c]+0x14]` at manager +0x28, `0x23c3a8()` at +0x2c, the entity at +0x34, clear the flag byte `[0x37758c]+0x30` and call `0x1e7750`. They differ in three places.
- Byte +0x38: slot +0x2c stores 1, slot +0x28 stores 0. The enter 0x30378c of the pushed node reads it: 1 selects the float pair at +0x54, 0 the pair at +0x44 as arguments of `0x22b6d4(1, pair)`; then it clears the byte.
- The argument of `0x1e7750` (writes `byte [0x376af8+1] = arg ^ 1`): slot +0x2c passes 1; slot +0x28 passes whether the name hash of `SceneChange_TruckChaseToDowntown` (`0x24da28` of the string at 0x303968) equals the entity's name hash at +8. The meaning of the byte is not read.
- The node: slot +0x2c calls `0x25d26c(owner, 6, floats +0x4c)`, which pushes the loading page mode (the sequence is the same as in 0x1a4c3c, which only skips the search for an existing node of type 6 or 7). Slot +0x28 calls `0x1a4c3c([0x37758c], floats +0x3c, byte +0x64)`: the same node build (type bytes +0x10..+0x14, owner at +0xc = the manager, `0x25cb04`) with the mode argument taken from the byte at +0x64 instead of 6. In both, a returned node gets `+0x40 = manager +0x2c`.
- Who runs 0x303480 (TRANS-11): the node's owner is the manager in both cases, and the main loop (0x1098b0..0x1098bc) calls slot +8 of the owner of the current node, so 0x303480 runs for slot +0x28 exactly as for slot +0x2c. No loading page object is pushed and no A is awaited, so no input is involved.

**Execution 0x303480 for a destination in a loaded scene**
- r6 = `0x238354(ref)`: the destination object. `0x2159a4(ref)` finds the scene: a reference whose world level hash (+0) names no scene gets scene index +0xd = 0 (the default scene, `[0x589a1c]`), otherwise the index of the world level (cached in +0xd, flag bit 1 of +0xc). The object is looked up by the point hash (+4) in that scene and, when missing, in the default scene.
- r7 = the scene (`0x589a18`: index 0 default, else the array). `0x2284bc(scene)` (state byte [state]+8 == 2 and [state+0x14] != 0) tells loaded. Loaded: r4 = 0, r6 = `0x213284(r6, matrix)` (the room of the point and its matrix), then `0x2284bc(room)` and `0x27e2fc(room)` must both be non-zero to take the direct path 0x303574; otherwise the path of an unloaded destination (OnSceneExit event, progress setter, loading page) runs with r4 = 0.
- Direct path: `0x303610` re-resolves the reference, stores the room at `[0x376c78]`, `0x1f4244(room, 1)`, `0x30249c(scene+0x131c)`, `0x1b42e8(room)`, and when the player node has no parent (`[[[0x3772c0]+0x3c]+4] == 0`) `0x245008([room+0x14])` and the matrix to the player node with `0x24c118`; then `0x242b08(player)`. The loading page push and the events `OnSceneExit` (0x303718) and `OnSceneEnter` (0x303738) are only on the path where r6 was cleared (the destination scene or room not loaded). So a same-scene entry fires neither event and shows no loading page. The 0x1f23f0 progress setter is not called on it.
- The camera is not touched by 0x303480 (no camera call on this path).

**Who takes slot +0x28 (0x1c47e8, 0x238354)**
- The update (walk-in of an enabled bound, [streaming-transitions.md §2026-10-14T12:02](streaming-transitions.md#ts-20261014-120258)) takes slot +0x28 when `[r7+0x14]` is non-null and `0x238354(ref)` resolves, else +0x2c. Resolving needs the destination scene to exist and the point to be found in it, so the rule is "the destination point is in a loaded scene". A reference with world level hash 0 resolves in the default scene.
- lvl_00 has four own `scenechange` objects with unqualified bounds and references {world level 0, point hash, 0, flags 2, scene index 0}: `Lobby2Basement` (room1, bound `Lobby2Basement_SceneChange`, point `Basement_Dest_Lobby_P1`), `Basement2Lobby` (room3, `Basement2Lobby_SceneChange`, `Lobby_Dest_Basement_P1`), `Basement2Lineup` (room3, `Bound_Basement2Lineup`, `Lineup_Dest_Basement_P1`), `Lineup2Basement` (room5, `Lineup_to_Basement`, `Basement_Dest_Lineup_P1`). The second reference (hash 0xddccaacc) points at the `_P2` points and is not read. `Basement_Dest_Lobby_P1` lies at (-299.37, 5.7328, 6.0847), yaw 43.604.
- The master level has six more objects whose destination world level is the one their bound is in (qualified references): `SceneChange_BankExToBankIn`, `BankInToBankEx`, `DowntownToTruckChase`, `TruckChaseToDowntown` (all Downtown), `ShackExtToShackInt`, `ShackIntToShackExt` (Fort Meadows). `SceneChange_TruckChaseToDowntown` is the name 0x3038a0 compares.
- The lift of lvl_00 is script-driven: `LIFT_basement` (0x8cc3365c) with `Lobby2Basement_Lift_Leave == 1` disables `Basement2Lobby_SceneChange`, enables `Lobby2Basement_SceneChange` and sets `AIEnable Player1 1` (the player is then in the lift and the bound request fires); `LIFT_Lobby` (0x212cfebb) is the mirror for `Basement2Lobby_SceneChange`. The bound `Lobby2Basement_SceneChange` names room4, which lvl_00 does not have (rooms 1, 3, 5): the room setup clears the active byte only for bounds of scene rooms ([scripts-runtime.md](scripts-runtime.md#ts-20261014-105432)), so such a bound is active in every room; the port treated it as never active.

**Port**: `src/runtime/same_level_entry.{hpp,cpp}` (entries from the level's own objects and the master objects that stay in their world level, `SameLevelEntryFlow`), glue in `src/game_level_update.cpp`, `ScriptRunner::setSceneRooms`. Tests: `same-level-entry-real-data`, `game-same-level-entry`.

Unknowns:
- The delay between the request and 0x303480 and what the pushed node does meanwhile (the fade pair at manager +0x3c / +0x44, byte +0x64, the node mode): not read or measured (TRANS-15).
- The branch where the destination room is not loaded (`0x27e2fc`, `0x2284bc(room)`): the port assumes it is (TRANS-15).
- The meaning of `byte [0x376af8+1]` and of the `_TruckChaseToDowntown` special case (TRANS-15).
- Camera pose, player state and lift door timing after a lift entry were not compared with the oracle (TRANS-16).

# Script natives: census, handlers and work batches

Every native (script function) of the level scripts: handler address, argument count, use in the levels, class and work batch. Script engine itself: [scripts.md](scripts.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-26 to SCRIPT-45). Batch 27 (progress, saves, studs): [script-natives-progress.md](script-natives-progress.md).

<a id="ts-20261014-133339"></a>

## 2026-10-14T13:33:39+02:00 — Census of the 245 natives used by level and class scripts, handler addresses, classes A-D and batches (llvm-objdump on game.arm.elf, C++ level loader; no emulator, no REA session)
Corrected by [script-natives-progress.md §2026-10-14T14:41](script-natives-progress.md#ts-20261014-144155): CountRacesWon is class A (not blocking) and the second argument of EnableStudsByGroup is not an enable flag.
Corrected by [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311): none of the 17 batch 34 natives blocks (all return 1; class C with B? was wrong).
Corrected by [script-natives-flags.md §2026-10-14T14:16](script-natives-flags.md#ts-20261014-141621): the note of PrintDebugString (0x26fde8 is a no-op, not the draw call 0x2665d0) and the classes of the batch 26 rows (what each handler does).

### Method

- **Handlers.** Two sources, which agree for every name found in both. (1) A static table of 16-byte entries {name pointer, handler, info = argument count | types << 8, 0} at 0x359154 (69 entries; the script words and the core natives, for example Wait 0x174604, EnableObject 0x1ddd00, EnableCollision 0x27e9f4). (2) About 290 registration calls `bl 0x23912c` in the startup code (0x15f8f8 and on): `r0 = name string` (pc-relative inline string), `r1 = handler` (literal), `r2` and `r3` = argument count and type byte (they equal the count of `Argument(` lines of `levels/scriptfns.txt` in 247 of 249 comparable cases; exceptions SpawnStuds, SetGravity). 0x23912c itself hashes the name (0x24da28) and stores the handler into the entry of the runtime table whose hash matches (+4), returning -1 when none matches. In the pool layout the handler word sits directly before the aligned name string, which is how the remaining natives were found.
- **Correction.** [Corrects scripts.md §2026-10-13T20:42](scripts.md#ts-20261013-204236): the claim that `scriptfns.txt` lists every native is wrong: it documents 300 of the 377 table entries; the argument types of the others come from the registration values above.
- **Handler ABI** (read in about 25 handlers): `handler(r0 = thread, r1 = array of argument value pointers)`; argument i is `*(r1 + 4 i)`, a float is `*(float*)value`, an object argument is resolved with 0x22dacc (entity pointer; its component pointer is entity+0x7c). Return 1 = done (pc advances), 0 = blocked: the executor calls the handler again every update with thread flag bit 2 set (`ldrh r0,[r0,#20]; tst r0,#4` = "is this a repeat call").
- **Counts.** `Calls` = call sites in the level scripts of the 26 `levels/levels/*.blvl` files that have scripts (30 files in total, 239 distinct names: the number of `progress_report.py`); `lvl_01` = the same in `lvl_01_cherrytreehills_main.blvl`; `1-1` = call sites of the scripts reachable (CallScript / SpawnScript closure) from `startup` plus every trigger script whose name starts with `1-1` in lvl_01 (43 of 314 scripts; static text, not an execution trace); `class files` = call sites in the 40 class `.blvl` files outside `levels/levels/` that have scripts. Six natives occur only there (EffectStart, EffectEnd, EffectAttachParticlesOffset, EffectDisableSpecialObject, ChangeCharTokenTexture, StartFireTruckHose); the progress measure does not count them.
- **Classes.** A = getter / setter / flag on state `chase-game` already has (placed-object state, triggers, bounds, player, save progress, script variables, messages to objects); B = blocking (returns 0 until something ends); C = needs a system that `chase-game` lacks (named in the batch); D = debug only or no call anywhere. The classes come from the name, the argument list of `scriptfns.txt` and a quick look at the handler's size and callees (`n`); only the entries with a text in the Note column had their body read. Everything else is a classification to be confirmed by the agent who implements the batch, and A entries may turn out to need more state than assumed (UnlockCharacter has 104 instructions). A batch agent must read each handler before transcribing it.
- B entries with a note were read in the disassembly; `B?` entries are blocking candidates (a conditional result in the body) not yet confirmed. Natives of batches 28 to 38 listed as C may also block; the executor makes that visible only in the first level that calls them.

### Batches (one agent, one session each; the order is the recommended one)

| Batch | Item | Natives | Class | Calls all / lvl_01 / 1-1 | Needs |
|---|---|---|---|---|---|
| 26 | SCRIPT-26 | object flags and script bookkeeping (22) | A | 1598 / 221 / 37 | - |
| 27 | SCRIPT-27 | progress, save and stud getters / setters (21) | A | 354 / 58 / 12 | - |
| 28 | SCRIPT-28 | DCam core and camera (13) | C, B | 1842 / 317 / 12 | SCRIPT-40 |
| 29 | SCRIPT-29 | DCam cutscenes, credits, camera effects (13) | C, B | 300 / 65 / 10 | SCRIPT-40, MOVIE-2 |
| 30 | SCRIPT-30 | dialogue, tips, mission texts (16) | C | 1985 / 325 / 40 | SCRIPT-41 |
| 31 | SCRIPT-31 | waypoints, objective markers, map, scanner (15) | C | 1232 / 204 / 46 | SCRIPT-41, HUD-11 |
| 32 | SCRIPT-32 | animation and attach (13) | C, B | 1613 / 212 / 23 | ANIM-2 |
| 33 | SCRIPT-33 | sound and music (13) | C | 902 / 110 / 1 | AUDIO-2 |
| 34 | SCRIPT-34 | AI control (17) | C, B | 1051 / 165 / 2 | SCRIPT-42 |
| 35 | SCRIPT-35 | characters, party, weapons, boss hooks (18) | C | 157 / 28 / 5 | SCRIPT-42 |
| 36 | SCRIPT-36 | vehicles and traffic (19) | C | 725 / 91 / 7 | SCRIPT-43 |
| 37 | SCRIPT-37 | platforms and rideables (14) | C, B | 161 / 8 / 0 | SCRIPT-44 |
| 38 | SCRIPT-38 | effects, particles, streaming, semi-transparent lists (20) | C | 553 / 55 / 5 | PART-1 |
| 39 | SCRIPT-39 | hub, shops, minigames (10) | C | 14 / 0 / 0 | SCRIPT-45 |

Sum (at the census): 224 unhandled natives in batches, 20 implemented, 1 stand-in (FadeScreen); batch 26 and 27 changed this, see their entries. The "needs" column names the system whose absence stops the native from doing anything visible; a batch agent can still transcribe the handler's argument handling and state writes, and mark the effect as a stand-in.

### Table (sorted by calls over all levels)

| Native | Handler | Args | Calls all / lvl_01 / 1-1 / class files | Levels | Class | Batch | Note |
|---|---|---|---|---|---|---|---|
| EnableObject | 0x1ddd00 | 2 | 7157 / 710 / 326 / 61 | 18 | done |  |  |
| Wait | 0x174604 | 1 | 3013 / 390 / 25 / 27 | 25 | done |  |  |
| EnableBound | 0x1d0ce8 | 2 | 2111 / 625 / 304 / 13 | 16 | done |  |  |
| TriggerObject | 0x1cddf8 | 2 | 1057 / 88 / 6 / 8 | 16 | done |  |  |
| EnableTrigger | 0x268164 | 2 | 1050 / 160 / 18 / 18 | 14 | done |  |  |
| = | 0x17efc0 | 2 | 856 / 102 / 29 / 4 | 9 | done |  |  |
| StartDCam | 0x194fd8 | 5 | 665 / 121 / 6 / 0 | 11 | done | 28 | handled (path and path command; the other commands are not): [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| PlayCharAnim | 0x1bf9cc | 4 | 625 / 96 / 0 / 0 | 10 | B | 32 | first call starts the animation; returns 1 when 0x2369c4 == 3 (shared with PlayCutscene) |
| ? | 0x16354c | 3 | 554 / 69 / 27 / 4 | 10 | done |  |  |
| ?? | 0x17efb8 | 0 | 554 / 69 / 27 / 4 | 10 | done |  |  |
| SetActiveWayPoint | 0x27bde8 | 2 | 540 / 79 / 23 / 0 | 8 | stand-in | 31 | handled: the waypoint object is shown and a map marker is drawn; a disable always applies (bit 4 test unknown, SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| PrintDebugString | 0x26fde8 | 2 | 514 / 125 / 6 / 6 | 24 | D | 26 | handled: 0x26fde8 is `mov r0,#1; bx lr`, no text is drawn or logged (corrects the old note: 0x2665d0 belongs to the next function 0x26fdf0) |
| PositionObject | 0x1d9e6c | 2 | 498 / 89 / 4 / 0 | 9 | done |  |  |
| CallScript | 0x1c3730 | 1 | 485 / 67 / 64 / 2 | 9 | done |  |  |
| ShowDialogBG | 0x1bfb64 | 1 | 443 / 64 / 4 / 0 | 8 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| WaitForDCam | 0x1883ec | 0 | 429 / 120 / 4 / 0 | 10 | done | 28 | handled, busy test without the cutscene part (SCRIPT-29): [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| SpawnScript | 0x1c3730 | 1 | 424 / 43 / 6 / 0 | 9 | done |  |  |
| ShowDialogueText | 0x26ff84 | 3 | 409 / 25 / 0 / 0 | 6 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| AIEnable | 0x1881d4 | 2 | 404 / 74 / 2 / 0 | 10 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| ShowDialogueTextTimed | 0x2a6d90 | 4 | 400 / 106 / 8 / 0 | 6 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| ToggleUseable | 0x1cdcf8 | 2 | 347 / 26 / 7 / 4 | 8 | A | 26 | handled, flag recorded (stand-in SCRIPT-S17) |
| PlaySound | 0x1b4394 | 2 | 277 / 25 / 0 / 0 | 8 | C | 33 |  |
| EffectFadeObject | 0x288a60 | 5 | 273 / 21 / 0 / 10 | 8 | C | 38 |  |
| PlayAnimation | 0x268240 | 5 | 273 / 19 / 0 / 14 | 11 | C | 32 |  |
| DCamActive | 0x1a2e48 | 2 | 261 / 24 / 0 / 0 | 9 | stand-in | 28 | flag stored, readers unknown |
| subobjectivegotoenable | 0x2adaac | 3 | 260 / 39 / 6 / 0 | 7 | stand-in | 31 | handled: registry transcribed (0x2a62b8), the group effect is not drawn (SCRIPT-71): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| StartDCamTrack | 0x1da810 | 4 | 259 / 31 / 0 / 0 | 10 | stand-in | 28 | handled; look point is the placement of the object, not entity +0x60 with its node matrix |
| ShowTipTextOnce | 0x263850 | 1 | 251 / 31 / 11 / 0 | 8 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| LoadAnimation | 0x2681a0 | 4 | 244 / 18 / 5 / 19 | 10 | C | 32 |  |
| EnableCollision | 0x27e9f4 | 2 | 240 / 33 / 2 / 5 | 9 | done |  |  |
| VehicleSetChaseMaxSpeed | 0x2b4bec | 2 | 208 / 12 / 0 / 0 | 6 | C | 36 |  |
| StopCharAnim | 0x1bfb84 | 1 | 193 / 30 / 0 / 0 | 8 | C | 32 |  |
| AIFaceTarget | 0x1bf570 | 2 | 182 / 18 / 0 / 0 | 8 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| registersound | 0x1cdbdc | 2 | 180 / 22 / 0 / 0 | 8 | C | 33 |  |
| FadeScreen | 0x1a2e8c | 1 | 178 / 39 / 0 / 0 | 8 | stand-in |  |  |
| SubObjectiveEnemyEnable | 0x2b4c3c | 3 | 152 / 37 / 0 / 0 | 5 | done | 31 | exact: handler is `mov r0, #1; bx lr`: [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| ShakeScreen | 0x1b2690 | 3 | 149 / 4 / 0 / 1 | 10 | stand-in | 28 | arguments stored, no shake drawn |
| !? | 0x174428 | 0 | 140 / 14 / 5 / 1 | 8 | done |  |  |
| setmissionsummary | 0x27be98 | 1 | 133 / 20 / 5 / 0 | 8 | stand-in | 30 | handled as a stand-in, key stored at 0x375f98 / 0x375f9c, no reader: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| setmissiontitle | 0x263788 | 1 | 133 / 20 / 5 / 0 | 8 | stand-in | 30 | handled as a stand-in, key stored at 0x375f98 / 0x375f9c, no reader: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| PlaySoundGO | 0x295244 | 3 | 114 / 1 / 0 / 0 | 7 | C | 33 |  |
| ResetObject | 0x1b25f8 | 1 | 107 / 5 / 0 / 0 | 11 | A | 26 | handled: class message 8 queued |
| VehicleSetLocked | 0x26fe78 | 2 | 102 / 13 / 3 / 0 | 8 | C | 36 |  |
| AIActive | 0x1881a4 | 1 | 101 / 16 / 0 / 0 | 6 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AttachObject | 0x1bf68c | 2 | 100 / 15 / 13 / 0 | 8 | C | 32 |  |
| VehicleSetForcedStop | 0x29ea7c | 2 | 99 / 13 / 1 / 0 | 6 | C | 36 |  |
| AIMove | 0x171f7c | 3 | 98 / 19 / 0 / 0 | 8 | stand-in | 34 | handled: starts a straight walk to the target object (path finder not ported) and ends with the AIMove trigger event; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| EnableTargetable | 0x26fb2c | 2 | 97 / 6 / 0 / 0 | 7 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| ShowTipTextTimed | 0x26ffec | 2 | 96 / 45 / 6 / 0 | 8 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| EnableDistanceFade | 0x2a05bc | 2 | 89 / 13 / 9 / 0 | 7 | A | 26 | open: node kind test and child recursion, needs SCRIPT-57 |
| SkipToFinalState | 0x270078 | 1 | 88 / 0 / 0 / 0 | 7 | A | 26 | handled: class message 54 queued |
| revealcollectable | 0x27bdb0 | 1 | 84 / 13 / 13 / 0 | 8 | A | 26 | handled: 0x27bdb0 is `mov r0,#1; bx lr` |
| =+ | 0x17ef30 | 4 | 77 / 7 / 1 / 0 | 9 | done |  |  |
| SubObjectiveArrowGoToEnable | 0x2c57b0 | 2 | 77 / 15 / 4 / 0 | 5 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| OverrideScannable | 0x27bce4 | 2 | 76 / 17 / 11 / 0 | 6 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| enablestudsbygroup | 0x285ed4 | 2 | 74 / 13 / 0 / 0 | 8 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| GiveReward | 0x1a2f18 | 1 | 74 / 9 / 1 / 4 | 14 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| VehicleJumpOut | 0x1da8d8 | 1 | 70 / 5 / 0 / 0 | 8 | C | 36 |  |
| StopMusic | 0x195054 | 1 | 68 / 15 / 0 / 0 | 7 | C | 33 |  |
| AutomaticMusic | 0x1d9c70 | 0 | 67 / 15 / 1 / 0 | 7 | C | 33 |  |
| DestroyProp | 0x1a2fe4 | 1 | 66 / 7 / 0 / 0 | 8 | A | 26 | open: entity destruction 0x23cd80, needs SCRIPT-58 |
| SwapAllegiance | 0x1da88c | 1 | 66 / 2 / 0 / 0 | 8 | C | 35 |  |
| OverrideMusic | 0x1cdb28 | 1 | 65 / 19 / 0 / 0 | 7 | C | 33 |  |
| PanMapToObject | 0x1d9e04 | 4 | 65 / 2 / 0 / 0 | 6 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| SpawnStuds | 0x1a3130 | 2 | 65 / 0 / 0 / 1 | 7 | C | 38 |  |
| UnlockAbility | 0x1cde84 | 1 | 63 / 12 / 7 / 0 | 10 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| AIIgnorePlayer | 0x1d9c38 | 2 | 62 / 0 / 0 / 0 | 4 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| ShowIntroText | 0x1cdc2c | 1 | 58 / 6 / 0 / 0 | 8 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| DCamCutSceneLoad | 0x2889a4 | 1 | 57 / 13 / 2 / 0 | 7 | C | 29 |  |
| DCamCutSceneWait | 0x288a40 | 1 | 57 / 13 / 2 / 0 | 7 | B | 29 | returns !0x2297b0(arg 0) |
| SpawnDebris | 0x1b26f0 | 4 | 57 / 0 / 0 / 1 | 3 | C | 38 |  |
| DCamCutSceneFree | 0x28898c | 1 | 56 / 13 / 2 / 0 | 7 | C | 29 |  |
| DCamCutScenePlay | 0x2889e4 | 5 | 56 / 13 / 2 / 0 | 7 | C | 29 |  |
| WaitForCurrentAnim | 0x2a06a8 | 1 | 53 / 8 / 0 / 1 | 7 | B | 32 | returns 0 until the playing animation frame is within 1 frame of its length |
| ainostate | 0x194e0c | 1 | 50 / 6 / 0 / 0 | 6 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| StopAnim | 0x1883d0 | 1 | 49 / 10 / 0 / 0 | 5 | C | 32 |  |
| StopSound | 0x1b43ec | 3 | 48 / 5 / 0 / 0 | 7 | C | 33 |  |
| vehiclestopsounds | 0x27c148 | 1 | 47 / 6 / 0 / 0 | 7 | C | 33 |  |
| WaitForCurrentCharAnim | 0x263ae4 | 1 | 46 / 3 / 0 / 0 | 8 | B | 32 | same shape as WaitForCurrentAnim |
| CharacterInvulnerable | 0x2a6cfc | 2 | 45 / 6 / 0 / 0 | 7 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| IsRedBrickExtraActive | 0x2a6d34 | 2 | 44 / 12 / 4 / 0 | 6 | done |  |  |
| VehicleStop | 0x1b27e8 | 1 | 43 / 6 / 0 / 0 | 6 | C | 36 |  |
| enableobjectfade | 0x26fac4 | 2 | 42 / 1 / 0 / 0 | 7 | A | 26 | open: message 24 first, then the EnableObject node calls; needs SCRIPT-57 |
| EnableTrafficAndPedestrians | 0x2c5780 | 1 | 42 / 12 / 0 / 0 | 6 | C | 36 |  |
| PositionPlayer | 0x1da0dc | 2 | 42 / 11 / 0 / 0 | 7 | A | 26 | open: PositionObject steps plus a ground probe (needs run-time data, SCRIPT-59) |
| AddToSemiTransNearCameraList | 0x2cb734 | 2 | 41 / 5 / 5 / 0 | 7 | C | 38 |  |
| enabletrafficpath | 0x27bc30 | 2 | 40 / 0 / 0 / 0 | 2 | C | 36 |  |
| StreamRoomIn | 0x1bfba4 | 1 | 40 / 9 / 0 / 0 | 6 | C | 38 |  |
| SnapCamera | 0x1a30fc | 0 | 39 / 15 / 2 / 0 | 7 | stand-in | 28 | follow reset through FollowCamera::reset |
| SetWeaponEnabled | 0x26fea4 | 2 | 36 / 4 / 0 / 0 | 5 | C | 35 |  |
| UnlockCharacter | 0x263918 | 1 | 34 / 1 / 0 / 0 | 8 | A | 27 | handled as stand-in, partial (SCRIPT-S21, SCRIPT-61) |
| AISuperJump | 0x1b2344 | 3 | 31 / 9 / 0 / 0 | 7 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| KillScript | 0x1c3834 | 1 | 30 / 5 / 1 / 0 | 7 | A | 26 | handled: threads of the script except the caller end |
| BulletTime | 0x1a2df0 | 3 | 28 / 6 / 0 / 0 | 9 | C | 29 |  |
| SetPartyCharacter | 0x27beb4 | 3 | 28 / 10 / 3 / 0 | 8 | C | 35 |  |
| SetRaceInProgress | 0x27bff4 | 1 | 28 / 4 / 0 / 0 | 6 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| ShowBossHearts | 0x1da788 | 2 | 27 / 0 / 0 / 0 | 7 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| SceneChange | 0x1b262c | 1 | 25 / 8 / 0 / 0 | 7 | done |  |  |
| AIResetState | 0x1bf650 | 1 | 24 / 11 / 0 / 0 | 6 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| EffectFadeAlpha | 0x27e7d0 | 4 | 24 / 13 / 0 / 5 | 4 | C | 38 |  |
| AIFollowSimplePath | 0x285d18 | 5 | 23 / 2 / 0 / 0 | 4 | stand-in | 34 | handled: walks the path points from the start node, forward or reverse: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| ScaleSoundFrequency | 0x2a8afc | 3 | 23 / 0 / 0 / 0 | 6 | C | 33 |  |
| AddStuds | 0x18838c | 1 | 22 / 3 / 0 / 0 | 7 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| SubObjectiveArrowEnemyEnable | 0x2c87f0 | 2 | 22 / 8 / 0 / 0 | 6 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| VehicleJumpIn | 0x1cdec0 | 2 | 22 / 4 / 0 / 0 | 6 | C | 36 |  |
| RideableIdle | 0x1bfb14 | 1 | 21 / 0 / 0 / 0 | 1 | C | 37 |  |
| StopRiding | 0x1a3180 | 2 | 21 / 0 / 0 / 0 | 1 | C | 37 |  |
| RideableRun | 0x1b2618 | 1 | 20 / 0 / 0 / 0 | 1 | C | 37 |  |
| RideableSetPath | 0x26376c | 2 | 20 / 0 / 0 / 0 | 1 | C | 37 |  |
| ToggleUpdateFlag | 0x270094 | 2 | 20 / 0 / 0 / 0 | 3 | A | 26 | handled; doors honour the flag (reader: update loop 0x1a4098), other readers open (SCRIPT-56, SCRIPT-S18) |
| StopPlayerVehicle | 0x27c040 | 1 | 18 / 8 / 0 / 0 | 4 | C | 36 |  |
| VehicleSetChased | 0x26fe54 | 3 | 18 / 2 / 0 / 0 | 8 | C | 36 |  |
| VehicleSetInvulnerable | 0x2ad904 | 2 | 17 / 2 / 0 / 0 | 6 | C | 36 |  |
| ?! | 0x18a7e4 | 3 | 16 / 7 / 0 / 0 | 7 | done |  |  |
| WarningDialogBox | 0x2700d4 | 1 | 16 / 7 / 0 / 0 | 7 | done |  |  |
| KillObject | 0x1a2fe4 | 1 | 15 / 0 / 0 / 0 | 2 | A | 26 | open: same handler as DestroyProp, needs SCRIPT-58 |
| PlayParticlesGO | 0x27e8b4 | 6 | 15 / 2 / 0 / 4 | 2 | C | 38 |  |
| RideableClearPath | 0x27bdcc | 1 | 14 / 0 / 0 / 0 | 1 | C | 37 |  |
| VehicleSetTopSpeed | 0x2860d4 | 2 | 14 / 2 / 0 / 0 | 6 | C | 36 |  |
| SetDCamEndWait | 0x1da690 | 1 | 13 / 0 / 0 / 0 | 3 | done | 28 | handled |
| WaitForPlatform | 0x263b84 | 1 | 13 / 3 / 0 / 0 | 5 | B | 37 | returns 1 when the platform value is 1.0 or its state is 5 |
| AttachToBone | 0x1ddb90 | 4 | 12 / 1 / 1 / 1 | 6 | C | 32 |  |
| EnableScanner | 0x1cd9dc | 1 | 12 / 1 / 0 / 0 | 7 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| setplatformorigin | 0x27bfbc | 2 | 12 / 2 / 0 / 0 | 3 | C | 37 |  |
| SetPlatformTarget | 0x27bfd8 | 2 | 12 / 2 / 0 / 0 | 3 | C | 37 |  |
| AddToSemiTransListBound | 0x2c08e8 | 3 | 11 / 0 / 0 / 0 | 2 | C | 38 |  |
| ObjectUnshovable | 0x26fd68 | 2 | 11 / 0 / 0 / 0 | 3 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| VehicleAllowJumpOut | 0x27c014 | 2 | 10 / 4 / 1 / 0 | 4 | C | 36 |  |
| VehicleSetUnshovable | 0x29eaa8 | 1 | 10 / 1 / 1 / 0 | 4 | A | 26 | open: blocks while the vehicle state is 6 or 7, SCRIPT-60 |
| EnableSwitch | 0x1bf8a4 | 2 | 9 / 0 / 0 / 0 | 2 | A | 26 | open: kind-dependent state write, needs SCRIPT-57 |
| IsLevelUnlocked | 0x2636e4 | 2 | 9 / 5 / 5 / 0 | 4 | done |  |  |
| PlayCutscene | 0x1bf9d4 | 1 | 9 / 3 / 2 / 0 | 6 | B | 29 | shares the body of PlayCharAnim (r2 = 1) |
| PlayCutsceneNoFadeBackIn | 0x2b9dac | 1 | 9 / 0 / 0 / 0 | 6 | C | 29 |  |
| QuickSave | 0x194f64 | 0 | 9 / 3 / 1 / 0 | 4 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| RoomStreamingWait | 0x27bde0 | 1 | 9 / 0 / 0 / 0 | 3 | B? | 38 |  |
| SetMapDummyLocObj | 0x27be84 | 1 | 9 / 0 / 0 / 0 | 4 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| ShowCharacterIntro | 0x2c2af8 | 1 | 9 / 2 / 0 / 0 | 8 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| clearmapdummylocobj | 0x26f9f4 | 0 | 8 / 0 / 0 / 0 | 4 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EnableShadow | 0x1bf84c | 2 | 8 / 8 / 0 / 0 | 1 | A | 26 | open: written only when 0x21aae0 (object kind) allows, needs SCRIPT-57 |
| GetGameComplete | 0x2635d0 | 1 | 8 / 1 / 1 / 0 | 8 | done |  |  |
| LockCharacter | 0x1cda60 | 1 | 8 / 0 / 0 / 0 | 6 | stand-in | 34 | handled: clears the group bit like UnlockCharacter sets it (party removal not modelled): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| MovePlatform | 0x1bf958 | 3 | 8 / 0 / 0 / 0 | 2 | C | 37 |  |
| SetDCamStartWait | 0x26fdf0 | 1 | 8 / 1 / 0 / 0 | 3 | done | 28 | handled |
| SetIndestructible | 0x27be40 | 2 | 8 / 0 / 0 / 0 | 2 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| ShowRaceStart | 0x1cdc9c | 0 | 8 / 1 / 0 / 0 | 7 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| AIPursuit | 0x17cbf0 | 3 | 7 / 2 / 0 / 0 | 5 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AttachToWorldRoot | 0x2951ec | 1 | 7 / 4 / 0 / 0 | 4 | C | 32 |  |
| AwardRacingCar | 0x1d9c84 | 0 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| ConversationPlayAndWait | 0x2ab364 | 5 | 7 / 2 / 0 / 0 | 2 | B | 29 | first call 0x25d974, repeat calls return 1 when the byte at thread+0x48 pointer is 0 |
| CountRacesWon | 0x1cd910 | 1 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled, exact; not blocking (corrects the B? class) |
| IsRacingCarAwarded | 0x285f64 | 1 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled, exact (batch 27 entry) |
| SetPlatformTime | 0x2637a4 | 2 | 7 / 0 / 0 / 0 | 1 | C | 37 |  |
| SetRaceWon | 0x1a30a8 | 1 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| SpawnDebris2Colour | 0x2860f0 | 7 | 7 / 4 / 0 / 0 | 3 | C | 38 |  |
| VehicleSetRaceTrim | 0x286080 | 2 | 7 / 1 / 0 / 0 | 6 | C | 36 |  |
| AICantAttack | 0x1bf528 | 2 | 6 / 0 / 0 / 0 | 1 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| OverrideFog | 0x1b24f8 | 2 | 6 / 0 / 0 / 0 | 2 | C | 29 |  |
| Set3DFocusObject | 0x288aa8 | 2 | 6 / 0 / 0 / 0 | 1 | stand-in | 28 | stereoscopic focus only, no effect on the mono picture |
| UnlockLevel | 0x1b276c | 1 | 6 / 1 / 0 / 0 | 6 | A | 27 | handled, exact (batch 27 entry) |
| VehicleEnableDrivingMusic | 0x2bf2b4 | 2 | 6 / 0 / 0 / 0 | 3 | C | 33 |  |
| AttachToBoneRot | 0x1bf718 | 6 | 5 / 4 / 4 / 0 | 2 | C | 32 |  |
| InformationBox | 0x1d9d4c | 1 | 5 / 4 / 0 / 0 | 2 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| SetBackgroundColour | 0x291624 | 3 | 5 / 0 / 0 / 0 | 1 | C | 29 |  |
| SetForcedStopOnMyVehicle | 0x2b9df0 | 2 | 5 / 4 / 2 / 0 | 2 | C | 36 |  |
| ShowLevelText | 0x1cdc54 | 0 | 5 / 1 / 1 / 0 | 5 | stand-in | 30 | heading box drawn, start time a stand-in: [script-text.md §2026-10-14T16:51](script-text.md#ts-20261014-165143) |
| ShowTally | 0x194f78 | 5 | 5 / 0 / 0 / 0 | 1 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| VehicleSetAI | 0x1bfb50 | 1 | 5 / 1 / 0 / 0 | 5 | C | 36 |  |
| VehicleSetDCamPathFollow | 0x27be1c | 3 | 5 / 0 / 0 / 0 | 3 | C | 28 | not read |
| activeduringdcam | 0x26f978 | 2 | 4 / 0 / 0 / 0 | 3 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| AIStalkPlayer | 0x1cd88c | 2 | 4 / 0 / 0 / 0 | 1 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| CharSetAllowVehicleEntry | 0x29ea3c | 2 | 4 / 4 / 0 / 0 | 1 | C | 35 |  |
| DCamCutsceneFadeDisable | 0x2c09ec | 0 | 4 / 2 / 0 / 0 | 2 | C | 29 |  |
| DisableSuperBuiltItSave | 0x2b4b78 | 2 | 4 / 2 / 1 / 0 | 2 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| enablescannerbutton | 0x2915ac | 1 | 4 / 2 / 0 / 0 | 2 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EndDCam | 0x17cbd4 | 0 | 4 / 0 / 0 / 0 | 2 | done | 28 | handled |
| OverrideZClip | 0x1cdb54 | 2 | 4 / 0 / 0 / 0 | 2 | C | 29 |  |
| RideableJump | 0x1bfb28 | 1 | 4 / 0 / 0 / 0 | 1 | C | 37 |  |
| setenergyguncolour | 0x285ffc | 2 | 4 / 0 / 0 / 0 | 1 | C | 38 |  |
| SetParachuteDeployHeight | 0x286040 | 1 | 4 / 1 / 1 / 0 | 4 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| SetPlatformSpeed | 0x26fe08 | 2 | 4 / 1 / 0 / 0 | 3 | C | 37 |  |
| SmokeyElectrifyTile | 0x2916b8 | 2 | 4 / 0 / 0 / 0 | 1 | C | 38 |  |
| StartMiniGame | 0x1cdcac | 1 | 4 / 0 / 0 / 0 | 2 | C | 39 |  |
| SuperFreeplaySwap | 0x27c0ac | 2 | 4 / 1 / 0 / 0 | 3 | C | 35 |  |
| AIMillAbout | 0x1b22e4 | 2 | 3 / 0 / 0 / 0 | 1 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AttachToRoom | 0x1ddc88 | 1 | 3 / 1 / 0 / 0 | 2 | C | 32 |  |
| CameraFollow | 0x1bf7c8 | 0 | 3 / 1 / 0 / 0 | 3 | C | 28 | read, not handled: [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| DistanceFadeMid | 0x27e798 | 1 | 3 / 0 / 0 / 0 | 1 | A | 26 | open: node kind test, needs SCRIPT-57 |
| PlatformSwapPath | 0x26fdc4 | 2 | 3 / 0 / 0 / 0 | 1 | C | 37 |  |
| RelockChasePlainClothes | 0x2b4bac | 0 | 3 / 3 / 1 / 0 | 1 | C | 35 |  |
| ResetCarriedThing | 0x27bd64 | 2 | 3 / 0 / 0 / 0 | 2 | A | 26 | open: acts on the carried thing of the player, SCRIPT-60 |
| SetSoundVolume | 0x272dcc | 3 | 3 / 1 / 0 / 0 | 3 | C | 33 |  |
| StopSoundGO | 0x1d0d28 | 3 | 3 / 1 / 0 / 0 | 2 | C | 33 |  |
| UnloadAnimation | 0x27e9c4 | 2 | 3 / 3 / 0 / 2 | 1 | C | 32 |  |
| Walnuts_Hit | 0x1b2828 | 1 | 3 / 0 / 0 / 0 | 1 | C | 35 |  |
| 1-8_RejoinFight | 0x26353c | 1 | 2 / 2 / 0 / 0 | 1 | C | 35 |  |
| AddToSemiTransList | 0x2a059c | 2 | 2 / 0 / 0 / 0 | 1 | C | 38 |  |
| AIMoveNoAvoid | 0x1cd798 | 3 | 2 / 2 / 0 / 0 | 1 | stand-in | 34 | handled: as AIMove, records the no-avoid flag: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| BlackoutMap | 0x1b23f4 | 1 | 2 / 0 / 0 / 0 | 1 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EnableMinimapWipe | 0x27bbe8 | 1 | 2 / 2 / 2 / 0 | 1 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| getinmech | 0x194e7c | 2 | 2 / 0 / 0 / 0 | 1 | C | 35 |  |
| ifplayercharacter | 0x27bc58 | 2 | 2 / 0 / 0 / 0 | 1 | C | 39 |  |
| LockAbility | 0x1b24ac | 1 | 2 / 0 / 0 / 0 | 2 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| PlayCredits | 0x1b25c0 | 0 | 2 / 0 / 0 / 0 | 2 | B | 29 | first call starts 0x213184; returns 1 when byte +0x25 of a global is 3 |
| PositionLastPlayerVehicleIfNearby | 0x2cd564 | 1 | 2 / 0 / 0 / 0 | 1 | C | 36 |  |
| removescannermapmarker | 0x2ad8f0 | 0 | 2 / 1 / 0 / 0 | 2 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| RideableGetOn | 0x1cdc10 | 2 | 2 / 0 / 0 / 0 | 1 | C | 37 |  |
| settrafficmaxvehicles | 0x2a6d70 | 1 | 2 / 2 / 0 / 0 | 1 | C | 36 |  |
| ShowPortraitOverVehicle | 0x2b4c1c | 2 | 2 / 0 / 0 / 0 | 1 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| 1-8_runfromarrest | 0x27bb4c | 1 | 1 / 1 / 0 / 0 | 1 | C | 35 |  |
| AIRunAway | 0x17cc30 | 1 | 1 / 0 / 0 / 0 | 1 | stand-in | 34 | handled: flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AwardViewer | 0x1b23e4 | 0 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| CameraRotFixedSet | 0x27bb9c | 4 | 1 / 0 / 0 / 0 | 1 | B? | 28 | read, not handled: [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| CharacterShop | 0x1cd8e4 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| CharSetSkydiveMode | 0x1da6e4 | 2 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| DisableIcon | 0x1b2414 | 2 | 1 / 1 / 0 / 0 | 1 | stand-in | 31 | handled as a stand-in that records its call (SCRIPT-S26): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EnableSuperBricks | 0x27bc08 | 1 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| FireBeamWeapon | 0x1d9cb4 | 4 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| GetCharacterUnlocked | 0x29e2c0 | 2 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled, exact (batch 27 entry) |
| getlanguage | 0x1b241c | 1 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S22) |
| GetOutMech | 0x1a2f04 | 1 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| GetPlayerNumber | 0x263618 | 1 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S22) |
| HACK_PlayDowntownIntro | 0x2ad87c | 1 | 1 / 1 / 1 / 0 | 1 | C | 35 |  |
| HubCustomisation | 0x26fc88 | 0 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| HubShop | 0x1b2450 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| IsRedbrickBought | 0x26fd30 | 2 | 1 / 0 / 0 / 0 | 1 | A | 27 | handled, exact (batch 27 entry) |
| PlayScriptOnStartup | 0x2915d4 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| PlaySoundAtObject | 0x27bd48 | 2 | 1 / 0 / 0 / 0 | 1 | C | 33 |  |
| PositionBound | 0x268390 | 2 | 1 / 0 / 0 / 0 | 1 | A | 26 | open: needs the object world position, only in lvl_test_playground_mike, SCRIPT-59 |
| RedBrickShop | 0x1bfafc | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| Rex_AxeSmash_Done | 0x27bdb8 | 1 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| Rex_GrapplePull_Done | 0x29ea28 | 1 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| SetPropRocking | 0x1da6a8 | 2 | 1 / 1 / 0 / 0 | 1 | C | 38 |  |
| showinfopointer | 0x263848 | 2 | 1 / 0 / 0 / 0 | 1 | stand-in | 30 | handled as a stand-in, call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| StudMagnetScene | 0x2638e8 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| UnlockScanner | 0x1cdeb0 | 0 | 1 / 1 / 1 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| UsePet | 0x172184 | 2 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| VehicleSetThrowObjects | 0x2ad940 | 2 | 1 / 0 / 0 / 0 | 1 | C | 36 |  |
| VehicleShop | 0x1b278c | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| walnuts_startphase2 | 0x291730 | 1 | 1 / 0 / 0 / 0 | 1 | C | 35 |  |
| ChangeCharTokenTexture | 0x2ad7c0 | 1 | 0 / 0 / 0 / 7 | 0 | C | 38 |  |
| EffectAttachParticlesOffset | 0x2ca970 | 5 | 0 / 0 / 0 / 13 | 0 | C | 38 |  |
| EffectDisableSpecialObject | 0x2c8f6c | 0 | 0 / 0 / 0 / 7 | 0 | C | 38 |  |
| EffectEnd | 0x1b438c | 1 | 0 / 0 / 0 / 6 | 0 | C | 38 |  |
| EffectStart | 0x1d0cd4 | 0 | 0 / 0 / 0 / 7 | 0 | C | 38 |  |
| StartFireTruckHose | 0x286214 | 1 | 0 / 0 / 0 / 2 | 0 | C | 38 |  |


### Natives with no call in any level or class script (class D)

These table entries have a handler (or at least a name) and no call site in the files above; they need no work until a script that uses them is found (the class `.blvl` files outside `levels/levels/` were included; mission and cutscene logic written in C++ may still call the handlers directly). The extraction of names from the startup code produced three garbled strings that are left out.

AIRemoveRangeBound, ActivateObject, Active, AddCharacterToParty, AddCharacterToPartyEnum, AddCharacterToPartyFade, AddFerryMapMarker, AddToSemiTransListx2, AddTreasureHunterObject, AlertMiB, AttachHeadParticles, BendSound, BlockLadderAtTop, BossStunForArrest, CarryResetLoc, ConversationPlay, DetatchFromBone, DuckMusicFor, EffectAttachParticles, EffectAttachParticlesToPath, EffectFadeAlphaThreshold, EffectLinkPathToObject, EffectPlayParticles, ElectrifyObject, EnableObjectWithFX, EnableSubobject, FireProjectile, FireTruckSpray, FlashEffectLight, FlushAnimations, FlushSounds, GetOnFireTruckHose, GetRandomNumber, GetSuperCounter, HideTally, HitObject, HitObjectInBound, HubCharSelect, IncreaseSuperCounter, IsAtWayPoint, IsInBound, IsObjectEnabled, IsRaceWon, IsStealthed, KillPet, MPSync, MakeDynamic, MakeStatic, MakeTransparent, NoCameraFollow, OverrideMusicMood, PanelCamera, PauseAllOtherScripts, PauseMusic, PauseScript, PauseTallyTimer, PickupObject, PlaceObjectFX, PlayCharAnimWithGravity, PlayCharacterSound, PlayCurrentAnimToEnd, PlayEffect, PlayParticles, PositionObjectOffset, PropFaceTarget, RemoveCharacterFromParty, RemoveTreasureHunterObject, ResetAnimation, Return, RideableWalk, SavedOk, ScaleAnimCharNode, ScaleSoundFrequencyGO, Set3DFocusDepth, Set3DFocusPath, Set3DFocusSnap, SetAnimationFrame, SetAnimationPaused, SetAutoOpen, SetCullFlags, SetFOV, SetFog, SetGravity, SetMapDummyLocPath, SetPlatformLooped, SetPlayerSkydiving, SetPropRotating, SetRespawnPoints, SetSilverBossHearts, SetSoundDistances, SetSoundFrequency, SetSoundVolumeGO, SetToppleTarget, ShowTallyTimer, SpawnDebrisTransparent, StartDCamTrackAuto, StartNodelessDCamTrack, StartRain, StartSimplePathCam, Stop, StopEffect, StopFireExtinguisher, TallyTimerWin, TransitionScreenIn, TransitionScreenOut, UnLoadAnim, UseFireExtinguisher, WaitForPlayingAnimFrame, Walnuts_StartPhase3, WhichPlayerInBound

### Unknowns

- Which A entries really are plain flag or state writes (only the handlers with a Note were read); which C entries block. A batch agent confirms each handler body before transcribing it.
- The meaning of the type byte (r3 / info >> 8) beyond the argument count; the runtime table builder that creates the 377 entries before 0x23912c binds the handlers (not located).
- Whether natives are also called by game code outside the script executor (messages to the mission logic); the call-site census only sees scripts.
- The 1-1 closure is static: scripts reachable by trigger events other than the name prefix `1-1` and by class startup scripts (SCRIPT-6) are not in it.

Moved entries:

- [script-natives-flags.md §2026-10-14T14:16](script-natives-flags.md#ts-20261014-141621): batch 26 (SCRIPT-26), the 22 handlers read.

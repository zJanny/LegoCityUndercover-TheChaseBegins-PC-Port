# Script natives: census table

Every native (script function) of the level and class scripts with its handler, argument count, calls, levels, class, work batch and current state. Method, classes A-D and the batches: [script-natives.md §2026-10-14T13:33](script-natives.md#ts-20261014-133339). Items: [script-system-natives.md](../plans/features/script-system-natives.md).

This file has no dated entries: the table is edited in place. When a native is read or implemented, update its row (CLAUDE.md "Script natives in batches"). The note of a row holds only its state (for example `handled`, `stand-in`, `open`), item IDs and links to the findings; addresses and details go into the linked docs/re entry. When this file passes 40 KB, split the table by batch into `script-natives-census-<first>-<last>.md`, one index row each.

## Current state

- The table lists every native with handler, argument count, call counts, levels, class (A to D, done, stand-in), batch and a state note; it has no dated entries and is edited in place.
- A row's note holds only its state, item IDs and links to the findings; natives are read and implemented in batches, and each row is updated when its handler is read.

## Table (sorted by calls over all levels)

| Native | Handler | Args | Calls all / lvl_01 / 1-1 / class files | Levels | Class | Batch | Note |
|---|---|---|---|---|---|---|---|
| EnableObject | 0x1ddd00 | 2 | 7157 / 710 / 326 / 61 | 18 | done |  |  |
| Wait | 0x174604 | 1 | 3013 / 390 / 25 / 27 | 25 | done |  |  |
| EnableBound | 0x1d0ce8 | 2 | 2111 / 625 / 304 / 13 | 16 | done |  |  |
| TriggerObject | 0x1cddf8 | 2 | 1057 / 88 / 6 / 8 | 16 | done |  |  |
| EnableTrigger | 0x268164 | 2 | 1050 / 160 / 18 / 18 | 14 | done |  |  |
| = | 0x17efc0 | 2 | 856 / 102 / 29 / 4 | 9 | done |  |  |
| StartDCam | 0x194fd8 | 5 | 665 / 121 / 6 / 0 | 11 | done | 28 | handled (path and path command; the other commands are not): [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| PlayCharAnim | 0x1bf9cc | 4 | 625 / 96 / 0 / 0 | 10 | stand-in | 32 | stand-in: plays on level characters and on Player1, records the rest (SCRIPT-S110, SCRIPT-S262, SCRIPT-S272, SCRIPT-273, SCRIPT-319, ANIM-S11, ANIM-S12): [script-natives-animation.md §2026-10-19T12:55](script-natives-animation.md#ts-20261019-125503) |
| ? | 0x16354c | 3 | 554 / 69 / 27 / 4 | 10 | done |  |  |
| ?? | 0x17efb8 | 0 | 554 / 69 / 27 / 4 | 10 | done |  |  |
| SetActiveWayPoint | 0x27bde8 | 2 | 540 / 79 / 23 / 0 | 8 | stand-in | 31 | handled: show and hide chain with map slot and disable fade (SCRIPT-74 closed); stand-in SCRIPT-S269: [objective-hud.md §2026-10-19T15:00](objective-hud.md#ts-20261019-150002) |
| PrintDebugString | 0x26fde8 | 2 | 514 / 125 / 6 / 6 | 24 | D | 26 | handled: 0x26fde8 is `mov r0,#1; bx lr`, no text is drawn or logged (corrects the old note: 0x2665d0 belongs to the next function 0x26fdf0) |
| PositionObject | 0x1d9e6c | 2 | 498 / 89 / 4 / 0 | 9 | done |  | player, vehicles and level characters (SCRIPT-287); props SCRIPT-288, no physics body (SCRIPT-S255 closed) |
| CallScript | 0x1c3730 | 1 | 485 / 67 / 64 / 2 | 9 | done |  | handled; a missing script stops the runner (SCRIPT-S5 closed): [scripts.md §2026-10-19T15:12:09](scripts.md#ts-20261019-151209) |
| ShowDialogBG | 0x1bfb64 | 1 | 443 / 64 / 4 / 0 | 8 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| WaitForDCam | 0x1883ec | 0 | 429 / 120 / 4 / 0 | 10 | done | 28 | handled, busy test without the cutscene part (SCRIPT-29): [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| SpawnScript | 0x1c3730 | 1 | 424 / 43 / 6 / 0 | 9 | done |  | handled; a missing script stops the runner (SCRIPT-S5 closed): [scripts.md §2026-10-19T15:12:09](scripts.md#ts-20261019-151209) |
| ShowDialogueText | 0x26ff84 | 3 | 409 / 25 / 0 / 0 | 6 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| AIEnable | 0x1881d4 | 2 | 404 / 74 / 2 / 0 | 10 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| ShowDialogueTextTimed | 0x2a6d90 | 4 | 400 / 106 / 8 / 0 | 6 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| ToggleUseable | 0x1cdcf8 | 2 | 347 / 26 / 7 / 4 | 8 | A | 26 | stand-in: kind 71 group case, flag recorded only for kinds with a use zone (SCRIPT-S239, SCRIPT-294, SCRIPT-296) |
| PlaySound | 0x1b4394 | 2 | 277 / 25 / 0 / 0 | 8 | stand-in | 33 | stand-in (AUDIO-S30) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| EffectFadeObject | 0x288a60 | 5 | 273 / 21 / 0 / 10 | 8 | stand-in | 38 | stand-in: node tween on the object's instances, flag bit 2 and particle nodes not ported (SCRIPT-S232, SCRIPT-281, SCRIPT-38): [script-natives-world.md §2026-10-18T11:01:40](script-natives-world.md#ts-20261018-110140) |
| PlayAnimation | 0x268240 | 5 | 273 / 19 / 0 / 14 | 11 | stand-in | 32 | stand-in: plays a loaded animation on level characters and on skinned placed props with AnimationOffset, records the rest (SCRIPT-S110, SCRIPT-273): [script-natives-animation.md §2026-10-15T20:04](script-natives-animation.md#ts-20261015-200440) |
| DCamActive | 0x1a2e48 | 2 | 261 / 24 / 0 / 0 | 9 | stand-in | 28 | flag stored, readers unknown |
| subobjectivegotoenable | 0x2adaac | 3 | 260 / 39 / 6 / 0 | 7 | done | 31 | exact: registry, record update and map slot drawn (SCRIPT-71 closed; update stand-in SCRIPT-S270): [objective-hud.md §2026-10-19T15:00](objective-hud.md#ts-20261019-150002) |
| StartDCamTrack | 0x1da810 | 4 | 259 / 31 / 0 / 0 | 10 | stand-in | 28 | handled; look point is the placement of the object, not entity +0x60 with its node matrix |
| ShowTipTextOnce | 0x263850 | 1 | 251 / 31 / 11 / 0 | 8 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| LoadAnimation | 0x2681a0 | 4 | 244 / 18 / 5 / 19 | 10 | stand-in | 32 | stand-in: name and class recorded as loaded (SCRIPT-S110, SCRIPT-274): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| EnableCollision | 0x27e9f4 | 2 | 240 / 33 / 2 / 5 | 9 | done |  |  |
| VehicleSetChaseMaxSpeed | 0x2b4bec | 2 | 208 / 12 / 0 / 0 | 6 | C | 36 |  |
| StopCharAnim | 0x1bfb84 | 1 | 193 / 30 / 0 / 0 | 8 | stand-in | 32 | stand-in: stops level characters and Player1 (SCRIPT-S110, SCRIPT-S262): [script-natives-animation.md §2026-10-19T12:55](script-natives-animation.md#ts-20261019-125503) |
| AIFaceTarget | 0x1bf570 | 2 | 182 / 18 / 0 / 0 | 8 | stand-in | 34 | state 10 (0x2c5bc4) ported: heading toward the target each tick, x/z pinned to home; returns 1 ([ai-states.md §2026-10-14T21:21](ai-states.md#ts-20261014-212158)): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| registersound | 0x1cdbdc | 2 | 180 / 22 / 0 / 0 | 8 | stand-in | 33 | stand-in, records only (AUDIO-S32) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| FadeScreen | 0x1a2e8c | 1 | 178 / 39 / 0 / 0 | 8 | stand-in |  | top screen only (SCRIPT-S253) |
| SubObjectiveEnemyEnable | 0x2b4c3c | 3 | 152 / 37 / 0 / 0 | 5 | done | 31 | exact: handler is `mov r0, #1; bx lr`: [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| ShakeScreen | 0x1b2690 | 3 | 149 / 4 / 0 / 1 | 10 | done | 28 | SCRIPT-S271, SCRIPT-332, CAMERA-S51, CAMERA-54 |
| !? | 0x174428 | 0 | 140 / 14 / 5 / 1 | 8 | done |  |  |
| setmissionsummary | 0x27be98 | 1 | 133 / 20 / 5 / 0 | 8 | stand-in | 30 | handled as a stand-in, key stored at 0x375f98 / 0x375f9c, no reader: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| setmissiontitle | 0x263788 | 1 | 133 / 20 / 5 / 0 | 8 | stand-in | 30 | handled as a stand-in, key stored at 0x375f98 / 0x375f9c, no reader: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| PlaySoundGO | 0x295244 | 3 | 114 / 1 / 0 / 0 | 7 | stand-in | 33 | stand-in (AUDIO-S30) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| ResetObject | 0x1b25f8 | 1 | 107 / 5 / 0 / 0 | 11 | A | 26 | handled: class message 8 queued; kind 132 restores placement, enables, clears destroyed bit; kind 64 enables only (SCRIPT-307, SCRIPT-313; rest SCRIPT-317, SCRIPT-318): [script-natives-world-reset.md §2026-10-19T11:06:52](script-natives-world-reset.md#ts-20261019-110652) |
| VehicleSetLocked | 0x26fe78 | 2 | 102 / 13 / 3 / 0 | 8 | done | 36 | record +0x311 bit 1; src/runtime/vehicle.cpp: [vehicles-system.md §2026-10-14T23:59:31](vehicles-system.md#ts-20261014-235931) |
| AIActive | 0x1881a4 | 1 | 101 / 16 / 0 / 0 | 6 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AttachObject | 0x1bf68c | 2 | 100 / 15 / 13 / 0 | 8 | stand-in | 32 | stand-in: parent of the second object recorded (SCRIPT-S110, SCRIPT-272): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| VehicleSetForcedStop | 0x29ea7c | 2 | 99 / 13 / 1 / 0 | 6 | done | 36 | record +0x310: [vehicles-system.md §2026-10-14T23:59:31](vehicles-system.md#ts-20261014-235931) |
| AIMove | 0x171f7c | 3 | 98 / 19 / 0 / 0 | 8 | stand-in | 34 | handled: starts a straight walk to the target object (path finder not ported) and ends with the AIMove trigger event; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| EnableTargetable | 0x26fb2c | 2 | 97 / 6 / 0 / 0 | 7 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| ShowTipTextTimed | 0x26ffec | 2 | 96 / 45 / 6 / 0 | 8 | stand-in | 30 | handled as a stand-in, drawn with fitted layout: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| EnableDistanceFade | 0x2a05bc | 2 | 89 / 13 / 9 / 0 | 7 | stand-in | 26 | stand-in: flag recorded, nothing draws the fade (SCRIPT-S233, SCRIPT-282, RENDER-66, SCRIPT-57): [script-natives-world.md §2026-10-18T11:01:40](script-natives-world.md#ts-20261018-110140) |
| SkipToFinalState | 0x270078 | 1 | 88 / 0 / 0 / 0 | 7 | A | 26 | handled: class message 54 queued |
| revealcollectable | 0x27bdb0 | 1 | 84 / 13 / 13 / 0 | 8 | A | 26 | handled: 0x27bdb0 is `mov r0,#1; bx lr` |
| =+ | 0x17ef30 | 4 | 77 / 7 / 1 / 0 | 9 | done |  |  |
| SubObjectiveArrowGoToEnable | 0x2c57b0 | 2 | 77 / 15 / 4 / 0 | 5 | done | 31 | exact: arrow list 0, drawn (SCRIPT-72 closed; draw stand-in SCRIPT-S270, SCRIPT-329): [objective-hud.md §2026-10-19T15:00](objective-hud.md#ts-20261019-150002) |
| OverrideScannable | 0x27bce4 | 2 | 76 / 17 / 11 / 0 | 6 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| enablestudsbygroup | 0x285ed4 | 2 | 74 / 13 / 0 / 0 | 8 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| GiveReward | 0x1a2f18 | 1 | 74 / 9 / 1 / 4 | 14 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| VehicleJumpOut | 0x1da8d8 | 1 | 70 / 5 / 0 / 0 | 8 | done | 36 | player: [vehicles-waypoints.md §2026-10-15T11:35](vehicles-waypoints.md#ts-20261015-113500) |
| StopMusic | 0x195054 | 1 | 68 / 15 / 0 / 0 | 7 | stand-in | 33 | block ramp (AUDIO-S20) [§2026-10-15T15:10](script-natives-audio.md#ts-20261015-151038) |
| AutomaticMusic | 0x1d9c70 | 0 | 67 / 15 / 1 / 0 | 7 | stand-in | 33 | clears 0x376b31; mood step needs combat [§2026-10-15T15:10](script-natives-audio.md#ts-20261015-151038) |
| DestroyProp | 0x1a2fe4 | 1 | 66 / 7 / 0 / 0 | 8 | stand-in | 26 | stand-in: effects of kinds 67, 204, 71, 64, 132 and the character death of 27, 121, 242 ported; SCRIPT-S245, SCRIPT-S246, SCRIPT-S250..S252, SCRIPT-S264, SCRIPT-S265, SCRIPT-306, SCRIPT-307, SCRIPT-321..323, SCRIPT-58: [script-natives-world.md §2026-10-19T10:02:26](script-natives-world.md#ts-20261019-100226), [script-natives-world-death.md §2026-10-19T13:21:05](script-natives-world-death.md#ts-20261019-132105) |
| SwapAllegiance | 0x1da88c | 1 | 66 / 2 / 0 / 0 | 8 | stand-in | 35 | toggles recorded, lists and grid not ported (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| OverrideMusic | 0x1cdb28 | 1 | 65 / 19 / 0 / 0 | 7 | stand-in | 33 | through the block queue and ramp (AUDIO-S20) [§2026-10-15T15:10](script-natives-audio.md#ts-20261015-151038) |
| PanMapToObject | 0x1d9e04 | 4 | 65 / 2 / 0 / 0 | 6 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| SpawnStuds | 0x1a3130 | 2 | 65 / 0 / 0 / 1 | 7 | C | 38 |  |
| UnlockAbility | 0x1cde84 | 1 | 63 / 12 / 7 / 0 | 10 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| AIIgnorePlayer | 0x1d9c38 | 2 | 62 / 0 / 0 / 0 | 4 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| ShowIntroText | 0x1cdc2c | 1 | 58 / 6 / 0 / 0 | 8 | stand-in | 30 | heading object with a hint text key: [script-text.md §2026-10-14T18:57](script-text.md#ts-20261014-185739) |
| DCamCutSceneLoad | 0x2889a4 | 1 | 57 / 13 / 2 / 0 | 7 | stand-in | 29 | slot table, loader 0x196390; the retry condition of the native is open: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929) |
| DCamCutSceneWait | 0x288a40 | 1 | 57 / 13 / 2 / 0 | 7 | stand-in | 29 | returns !0x2297b0(name): [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929) |
| SpawnDebris | 0x1b26f0 | 4 | 57 / 0 / 0 / 1 | 3 | C | 38 |  |
| DCamCutSceneFree | 0x28898c | 1 | 56 / 13 / 2 / 0 | 7 | stand-in | 29 | 0x196144: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929) |
| DCamCutScenePlay | 0x26fa18 | 5 | 56 / 13 / 2 / 0 | 7 | stand-in | 29 | fades handled (SCRIPT-S242, SCRIPT-S243); flags b c d open: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929), [cutscenes.md §2026-10-19T09:12:06](cutscenes.md#ts-20261019-091206) |
| WaitForCurrentAnim | 0x2a06a8 | 1 | 53 / 8 / 0 / 1 | 7 | stand-in | 32 | stand-in: blocks on level characters and skinned placed props as the original, completes at once otherwise (SCRIPT-S110): [script-natives-animation.md §2026-10-15T20:04](script-natives-animation.md#ts-20261015-200440) |
| ainostate | 0x194e0c | 1 | 50 / 6 / 0 / 0 | 6 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| StopAnim | 0x1883d0 | 1 | 49 / 10 / 0 / 0 | 5 | stand-in | 32 | stand-in: stops level characters and skinned placed props, records the rest (SCRIPT-S110): [script-natives-animation.md §2026-10-15T20:04](script-natives-animation.md#ts-20261015-200440) |
| StopSound | 0x1b43ec | 3 | 48 / 5 / 0 / 0 | 7 | done | 33 | handled [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| vehiclestopsounds | 0x27c148 | 1 | 47 / 6 / 0 / 0 | 7 | stand-in | 33 | stand-in, records only (AUDIO-S32) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| WaitForCurrentCharAnim | 0x263ae4 | 1 | 46 / 3 / 0 / 0 | 8 | stand-in | 32 | stand-in: blocks on level characters and Player1 as the original, completes at once otherwise (SCRIPT-S110, SCRIPT-S263): [script-natives-animation.md §2026-10-19T12:55](script-natives-animation.md#ts-20261019-125503) |
| CharacterInvulnerable | 0x2a6cfc | 2 | 45 / 6 / 0 / 0 | 7 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| IsRedBrickExtraActive | 0x2a6d34 | 2 | 44 / 12 / 4 / 0 | 6 | done |  |  |
| VehicleStop | 0x1b27e8 | 1 | 43 / 6 / 0 / 0 | 6 | C | 36 |  |
| enableobjectfade | 0x26fac4 | 2 | 42 / 1 / 0 / 0 | 7 | A | 26 | stand-in: ends like EnableObject unless the kind answers message 24 (SCRIPT-S241, SCRIPT-295) |
| EnableTrafficAndPedestrians | 0x2c5780 | 1 | 42 / 12 / 0 / 0 | 6 | stand-in | 36 | flag only, no traffic [§2026-10-15T15:10](script-natives-audio.md#ts-20261015-151038) |
| PositionPlayer | 0x1da0dc | 2 | 42 / 11 / 0 / 0 | 7 | stand-in | 26 | handled like PositionObject for the player (fall reset, message 8 and bits recorded SCRIPT-S274, SCRIPT-S275; a seated player moves the vehicle SCRIPT-S276); ground probe SCRIPT-59: [script-natives-flags.md §2026-10-19T15:12:32](script-natives-flags.md#ts-20261019-151232) |
| AddToSemiTransNearCameraList | 0x2cb734 | 2 | 41 / 5 / 5 / 0 | 7 | stand-in | 38 | stand-in: entries recorded, not drawn (SCRIPT-S233, RENDER-67, SCRIPT-38): [script-natives-world.md §2026-10-18T11:01:40](script-natives-world.md#ts-20261018-110140) |
| enabletrafficpath | 0x27bc30 | 2 | 40 / 0 / 0 / 0 | 2 | C | 36 |  |
| StreamRoomIn | 0x1bfba4 | 1 | 40 / 9 / 0 / 0 | 6 | done | 38 | handled: requests the room, blocks until loaded (SCRIPT-S235, WSTREAM-33, SCRIPT-38): [script-natives-world.md §2026-10-18T11:01:40](script-natives-world.md#ts-20261018-110140) |
| SnapCamera | 0x1a30fc | 0 | 39 / 15 / 2 / 0 | 7 | stand-in | 28 | follow reset through FollowCamera::reset |
| SetWeaponEnabled | 0x26fea4 | 2 | 36 / 4 / 0 / 0 | 5 | stand-in | 35 | hand choice and player bytes recorded (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| UnlockCharacter | 0x263918 | 1 | 34 / 1 / 0 / 0 | 8 | A | 27 | handled as stand-in, partial (SCRIPT-S21, SCRIPT-61) |
| AISuperJump | 0x1b2344 | 3 | 31 / 9 / 0 / 0 | 7 | stand-in | 34 | set-up 0x213300 and state 11 ported; the flight is a stand-in (SCRIPT-103); returns 1 ([ai-states.md §2026-10-14T21:21](ai-states.md#ts-20261014-212158)): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| KillScript | 0x1c3834 | 1 | 30 / 5 / 1 / 0 | 7 | A | 26 | handled: threads of the script except the caller end |
| BulletTime | 0x1a2df0 | 3 | 28 / 6 / 0 / 0 | 9 | C | 29 |  |
| SetPartyCharacter | 0x27beb4 | 3 | 28 / 10 / 3 / 0 | 8 | stand-in | 35 | stand-in: a swap to another character is not ported (SCRIPT-S234, SCRIPT-279): [script-natives-world.md §2026-10-18T11:01:40](script-natives-world.md#ts-20261018-110140) |
| SetRaceInProgress | 0x27bff4 | 1 | 28 / 4 / 0 / 0 | 6 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| ShowBossHearts | 0x1da788 | 2 | 27 / 0 / 0 / 0 | 7 | stand-in | 30 | call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| SceneChange | 0x1b262c | 1 | 25 / 8 / 0 / 0 | 7 | done |  |  |
| AIResetState | 0x1bf650 | 1 | 24 / 11 / 0 / 0 | 6 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| EffectFadeAlpha | 0x27e7d0 | 4 | 24 / 13 / 0 / 5 | 4 | C | 38 |  |
| AIFollowSimplePath | 0x285d18 | 5 | 23 / 2 / 0 / 0 | 4 | stand-in | 34 | handled: walks the path points from the start node, forward or reverse: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| ScaleSoundFrequency | 0x2a8afc | 3 | 23 / 0 / 0 / 0 | 6 | stand-in | 33 | stand-in (AUDIO-S31) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| AddStuds | 0x18838c | 1 | 22 / 3 / 0 / 0 | 7 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| SubObjectiveArrowEnemyEnable | 0x2c87f0 | 2 | 22 / 8 / 0 / 0 | 6 | done | 31 | exact: arrow list 1, drawn (SCRIPT-72 closed; draw stand-in SCRIPT-S270, SCRIPT-329): [objective-hud.md §2026-10-19T15:00](objective-hud.md#ts-20261019-150002) |
| VehicleJumpIn | 0x1cdec0 | 2 | 22 / 4 / 0 / 0 | 6 | done | 36 | player: [vehicles-waypoints.md §2026-10-15T11:35](vehicles-waypoints.md#ts-20261015-113500) |
| RideableIdle | 0x1bfb14 | 1 | 21 / 0 / 0 / 0 | 1 | C | 37 |  |
| StopRiding | 0x1a3180 | 2 | 21 / 0 / 0 / 0 | 1 | C | 37 |  |
| RideableRun | 0x1b2618 | 1 | 20 / 0 / 0 / 0 | 1 | C | 37 |  |
| RideableSetPath | 0x26376c | 2 | 20 / 0 / 0 / 0 | 1 | C | 37 |  |
| ToggleUpdateFlag | 0x270094 | 2 | 20 / 0 / 0 / 0 | 3 | A | 26 | handled; doors honour the flag (reader: update loop 0x1a4098), other readers open (SCRIPT-56, SCRIPT-S18) |
| StopPlayerVehicle | 0x27c040 | 1 | 18 / 8 / 0 / 0 | 4 | done | 36 | [vehicles-waypoints.md §2026-10-15T11:35](vehicles-waypoints.md#ts-20261015-113500) |
| VehicleSetChased | 0x26fe54 | 3 | 18 / 2 / 0 / 0 | 8 | C | 36 |  |
| VehicleSetInvulnerable | 0x2ad904 | 2 | 17 / 2 / 0 / 0 | 6 | C | 36 |  |
| ?! | 0x18a7e4 | 3 | 16 / 7 / 0 / 0 | 7 | done |  |  |
| WarningDialogBox | 0x2700d4 | 1 | 16 / 7 / 0 / 0 | 7 | done |  |  |
| KillObject | 0x1a2fe4 | 1 | 15 / 0 / 0 / 0 | 2 | stand-in | 26 | stand-in: same handler as DestroyProp (SCRIPT-S245, SCRIPT-S246, SCRIPT-S264, SCRIPT-S265, SCRIPT-306, SCRIPT-321..323): [script-natives-world.md §2026-10-19T10:02:26](script-natives-world.md#ts-20261019-100226), [script-natives-world-death.md §2026-10-19T13:21:05](script-natives-world-death.md#ts-20261019-132105) |
| PlayParticlesGO | 0x27e8b4 | 6 | 15 / 2 / 0 / 4 | 2 | C | 38 |  |
| RideableClearPath | 0x27bdcc | 1 | 14 / 0 / 0 / 0 | 1 | C | 37 |  |
| VehicleSetTopSpeed | 0x2860d4 | 2 | 14 / 2 / 0 / 0 | 6 | done | 36 | [vehicles-waypoints.md §2026-10-15T11:35](vehicles-waypoints.md#ts-20261015-113500) |
| SetDCamEndWait | 0x1da690 | 1 | 13 / 0 / 0 / 0 | 3 | done | 28 | handled |
| WaitForPlatform | 0x263b84 | 1 | 13 / 3 / 0 / 0 | 5 | B | 37 | returns 1 when the platform value is 1.0 or its state is 5 |
| AttachToBone | 0x1ddb90 | 4 | 12 / 1 / 1 / 1 | 6 | stand-in | 32 | stand-in: parent, its bone and the child bone recorded (SCRIPT-S110, SCRIPT-272): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| EnableScanner | 0x1cd9dc | 1 | 12 / 1 / 0 / 0 | 7 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| setplatformorigin | 0x27bfbc | 2 | 12 / 2 / 0 / 0 | 3 | C | 37 |  |
| SetPlatformTarget | 0x27bfd8 | 2 | 12 / 2 / 0 / 0 | 3 | C | 37 |  |
| AddToSemiTransListBound | 0x2c08e8 | 3 | 11 / 0 / 0 / 0 | 2 | C | 38 |  |
| ObjectUnshovable | 0x26fd68 | 2 | 11 / 0 / 0 / 0 | 3 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| VehicleAllowJumpOut | 0x27c014 | 2 | 10 / 4 / 1 / 0 | 4 | done | 36 | record +0x311 bit 6: [vehicles-system.md §2026-10-14T23:59:31](vehicles-system.md#ts-20261014-235931) |
| VehicleSetUnshovable | 0x29eaa8 | 1 | 10 / 1 / 1 / 0 | 4 | done | 36 | +4 = 10, returns 0 (the thread repeats it) while +2 is 6 or 7: [vehicles-system.md §2026-10-14T23:59:31](vehicles-system.md#ts-20261014-235931) |
| EnableSwitch | 0x1bf8a4 | 2 | 9 / 0 / 0 / 0 | 2 | A | 26 | stand-in: message 8, use flag (registered kinds) and switch state recorded (SCRIPT-S238, SCRIPT-S239) |
| IsLevelUnlocked | 0x2636e4 | 2 | 9 / 5 / 5 / 0 | 4 | done |  |  |
| PlayCutscene | 0x1bf9d4 | 1 | 9 / 3 / 2 / 0 | 6 | stand-in | 29 | own body 0x1bf9dc, blocks until the movie ends: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929) |
| PlayCutsceneNoFadeBackIn | 0x2b9dac | 1 | 9 / 0 / 0 / 0 | 6 | stand-in | 29 | sets byte 0x5163e8+0x184, then PlayCutscene with fade-back-in 0: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929) |
| QuickSave | 0x194f64 | 0 | 9 / 3 / 1 / 0 | 4 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| RoomStreamingWait | 0x27bde0 | 1 | 9 / 0 / 0 / 0 | 3 | B? | 38 |  |
| SetMapDummyLocObj | 0x27be84 | 1 | 9 / 0 / 0 / 0 | 4 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| ShowCharacterIntro | 0x2c2af8 | 1 | 9 / 2 / 0 / 0 | 8 | stand-in | 30 | dialogue box with the character name; HUD portrait swap missing (HUD-25): [script-text.md §2026-10-14T18:57](script-text.md#ts-20261014-185739) |
| clearmapdummylocobj | 0x26f9f4 | 0 | 8 / 0 / 0 / 0 | 4 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EnableShadow | 0x1bf84c | 2 | 8 / 8 / 0 / 0 | 1 | A | 26 | stand-in: recorded for kinds 27, 149, 121, 159, 242 (SCRIPT-S238, SCRIPT-S240, SCRIPT-297) |
| GetGameComplete | 0x2635d0 | 1 | 8 / 1 / 1 / 0 | 8 | done |  |  |
| LockCharacter | 0x1cda60 | 1 | 8 / 0 / 0 / 0 | 6 | stand-in | 34 | handled: clears the group bit like UnlockCharacter sets it (party removal not modelled): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| MovePlatform | 0x1bf958 | 3 | 8 / 0 / 0 / 0 | 2 | C | 37 |  |
| SetDCamStartWait | 0x26fdf0 | 1 | 8 / 1 / 0 / 0 | 3 | done | 28 | handled |
| SetIndestructible | 0x27be40 | 2 | 8 / 0 / 0 / 0 | 2 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| ShowRaceStart | 0x1cdc9c | 0 | 8 / 1 / 0 / 0 | 7 | stand-in | 30 | call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| AIPursuit | 0x17cbf0 | 3 | 7 / 2 / 0 / 0 | 5 | stand-in | 34 | state numbers 35/36 recorded; update 0x2ab6cc not ported (SCRIPT-104); returns 1 ([ai-states.md §2026-10-14T21:21](ai-states.md#ts-20261014-212158)): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AttachToWorldRoot | 0x2951ec | 1 | 7 / 4 / 0 / 0 | 4 | stand-in | 32 | stand-in: recorded (SCRIPT-S110, SCRIPT-272): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| AwardRacingCar | 0x1d9c84 | 0 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| ConversationPlayAndWait | 0x2ab364 | 5 | 7 / 2 / 0 / 0 | 2 | B | 29 | first call 0x25d974, repeat calls return 1 when the byte at thread+0x48 pointer is 0 |
| CountRacesWon | 0x1cd910 | 1 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled, exact; not blocking (corrects the B? class) |
| IsRacingCarAwarded | 0x285f64 | 1 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled, exact (batch 27 entry) |
| SetPlatformTime | 0x2637a4 | 2 | 7 / 0 / 0 / 0 | 1 | C | 37 |  |
| SetRaceWon | 0x1a30a8 | 1 | 7 / 1 / 0 / 0 | 6 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| SpawnDebris2Colour | 0x2860f0 | 7 | 7 / 4 / 0 / 0 | 3 | C | 38 |  |
| VehicleSetRaceTrim | 0x286080 | 2 | 7 / 1 / 0 / 0 | 6 | C | 36 |  |
| AICantAttack | 0x1bf528 | 2 | 6 / 0 / 0 / 0 | 1 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| OverrideFog | 0x1b24f8 | 2 | 6 / 0 / 0 / 0 | 2 | C | 29 |  |
| Set3DFocusObject | 0x288aa8 | 2 | 6 / 0 / 0 / 0 | 1 | stand-in | 28 | stereoscopic focus only, no effect on the mono picture |
| UnlockLevel | 0x1b276c | 1 | 6 / 1 / 0 / 0 | 6 | A | 27 | handled, exact (batch 27 entry) |
| VehicleEnableDrivingMusic | 0x2bf2b4 | 2 | 6 / 0 / 0 / 0 | 3 | stand-in | 33 | stand-in, records only (AUDIO-S32) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| AttachToBoneRot | 0x1bf718 | 6 | 5 / 4 / 4 / 0 | 2 | stand-in | 32 | stand-in: parent, its bone and the rotation recorded (SCRIPT-S110, SCRIPT-272): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| InformationBox | 0x1d9d4c | 1 | 5 / 4 / 0 / 0 | 2 | stand-in | 30 | dialog mode like WarningDialogBox, empty second button: [script-text.md §2026-10-14T18:57](script-text.md#ts-20261014-185739) |
| SetBackgroundColour | 0x291624 | 3 | 5 / 0 / 0 / 0 | 1 | C | 29 |  |
| SetForcedStopOnMyVehicle | 0x2b9df0 | 2 | 5 / 4 / 2 / 0 | 2 | done | 36 | first argument is a character; no effect while it is on foot (always, until VEH-4): [vehicles-system.md §2026-10-14T23:59:31](vehicles-system.md#ts-20261014-235931) |
| ShowLevelText | 0x1cdc54 | 0 | 5 / 1 / 1 / 0 | 5 | stand-in | 30 | heading box drawn, start time a stand-in: [script-text.md §2026-10-14T16:51](script-text.md#ts-20261014-165143) |
| ShowTally | 0x194f78 | 5 | 5 / 0 / 0 / 0 | 1 | stand-in | 30 | call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| VehicleSetAI | 0x1bfb50 | 1 | 5 / 1 / 0 / 0 | 5 | C | 36 |  |
| VehicleSetDCamPathFollow | 0x27be1c | 3 | 5 / 0 / 0 / 0 | 3 | C | 28 | not read |
| activeduringdcam | 0x26f978 | 2 | 4 / 0 / 0 / 0 | 3 | A | 26 | handled, flag recorded (SCRIPT-S18) |
| AIStalkPlayer | 0x1cd88c | 2 | 4 / 0 / 0 / 0 | 1 | stand-in | 34 | state 27 recorded; update 0x2c1c90 not ported (SCRIPT-104); returns 1 ([ai-states.md §2026-10-14T21:21](ai-states.md#ts-20261014-212158)): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| CharSetAllowVehicleEntry | 0x29ea3c | 2 | 4 / 4 / 0 / 0 | 1 | stand-in | 35 | flag recorded, boarding gate not wired (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| DCamCutsceneFadeDisable | 0x2c09ec | 0 | 4 / 2 / 0 / 0 | 2 | stand-in | 29 | sets byte 0x529e30+0x39d, nothing reads it yet: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929) |
| DisableSuperBuiltItSave | 0x2b4b78 | 2 | 4 / 2 / 1 / 0 | 2 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| enablescannerbutton | 0x2915ac | 1 | 4 / 2 / 0 / 0 | 2 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EndDCam | 0x17cbd4 | 0 | 4 / 0 / 0 / 0 | 2 | done | 28 | handled |
| OverrideZClip | 0x1cdb54 | 2 | 4 / 0 / 0 / 0 | 2 | C | 29 |  |
| RideableJump | 0x1bfb28 | 1 | 4 / 0 / 0 / 0 | 1 | C | 37 |  |
| setenergyguncolour | 0x285ffc | 2 | 4 / 0 / 0 / 0 | 1 | C | 38 |  |
| SetParachuteDeployHeight | 0x286040 | 1 | 4 / 1 / 1 / 0 | 4 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| SetPlatformSpeed | 0x26fe08 | 2 | 4 / 1 / 0 / 0 | 3 | C | 37 |  |
| SmokeyElectrifyTile | 0x2916b8 | 2 | 4 / 0 / 0 / 0 | 1 | C | 38 |  |
| StartMiniGame | 0x1cdcac | 1 | 4 / 0 / 0 / 0 | 2 | C | 39 |  |
| SuperFreeplaySwap | 0x27c0ac | 2 | 4 / 1 / 0 / 0 | 3 | stand-in | 35 | swap record recorded (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| AIMillAbout | 0x1b22e4 | 2 | 3 / 0 / 0 / 0 | 1 | stand-in | 34 | flag or requested state recorded; returns 1: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AttachToRoom | 0x1ddc88 | 1 | 3 / 1 / 0 / 0 | 2 | stand-in | 32 | stand-in: recorded (SCRIPT-S110, SCRIPT-272): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| CameraFollow | 0x1bf7c8 | 0 | 3 / 1 / 0 / 0 | 3 | stand-in | 28 | camera switch request (manager +0x28), no-op stand-in: [script-natives-audio.md §2026-10-15T15:10](script-natives-audio.md#ts-20261015-151038) |
| DistanceFadeMid | 0x27e798 | 1 | 3 / 0 / 0 / 0 | 1 | A | 26 | open: node kind test, SCRIPT-282 |
| PlatformSwapPath | 0x26fdc4 | 2 | 3 / 0 / 0 / 0 | 1 | C | 37 |  |
| RelockChasePlainClothes | 0x2b4bac | 0 | 3 / 3 / 1 / 0 | 1 | stand-in | 35 | party removal and swap record recorded (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| ResetCarriedThing | 0x27bd64 | 2 | 3 / 0 / 0 / 0 | 2 | A | 26 | open: acts on the carried thing of the player, SCRIPT-60 |
| SetSoundVolume | 0x272dcc | 3 | 3 / 1 / 0 / 0 | 3 | stand-in | 33 | stand-in (AUDIO-S31) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| StopSoundGO | 0x1d0d28 | 3 | 3 / 1 / 0 / 0 | 2 | stand-in | 33 | stand-in (AUDIO-S30, AUDIO-33) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| UnloadAnimation | 0x27e9c4 | 2 | 3 / 3 / 0 / 2 | 1 | stand-in | 32 | stand-in: name removed from the loaded set (SCRIPT-S110, SCRIPT-274): [script-natives-animation.md §2026-10-15T18:33](script-natives-animation.md#ts-20261015-183336) |
| Walnuts_Hit | 0x1b2828 | 1 | 3 / 0 / 0 / 0 | 1 | stand-in | 35 | boss requested state 5 recorded (SCRIPT-S278, SCRIPT-339): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| 1-8_RejoinFight | 0x26353c | 1 | 2 / 2 / 0 / 0 | 1 | stand-in | 35 | boss requested state 6 recorded (SCRIPT-S278, SCRIPT-339): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| AddToSemiTransList | 0x2a059c | 2 | 2 / 0 / 0 / 0 | 1 | C | 38 |  |
| AIMoveNoAvoid | 0x1cd798 | 3 | 2 / 2 / 0 / 0 | 1 | stand-in | 34 | handled: as AIMove, records the no-avoid flag: [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| BlackoutMap | 0x1b23f4 | 1 | 2 / 0 / 0 / 0 | 1 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EnableMinimapWipe | 0x27bbe8 | 1 | 2 / 2 / 2 / 0 | 1 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| getinmech | 0x194e7c | 2 | 2 / 0 / 0 / 0 | 1 | stand-in | 35 | pilot pair recorded (SCRIPT-S279, SCRIPT-340): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| ifplayercharacter | 0x27bc58 | 2 | 2 / 0 / 0 / 0 | 1 | C | 39 |  |
| LockAbility | 0x1b24ac | 1 | 2 / 0 / 0 / 0 | 2 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| PlayCredits | 0x1b25c0 | 0 | 2 / 0 / 0 / 0 | 2 | B | 29 | first call starts 0x213184; returns 1 when byte +0x25 of a global is 3 |
| PositionLastPlayerVehicleIfNearby | 0x2cd564 | 1 | 2 / 0 / 0 / 0 | 1 | C | 36 |  |
| removescannermapmarker | 0x2ad8f0 | 0 | 2 / 1 / 0 / 0 | 2 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| RideableGetOn | 0x1cdc10 | 2 | 2 / 0 / 0 / 0 | 1 | C | 37 |  |
| settrafficmaxvehicles | 0x2a6d70 | 1 | 2 / 2 / 0 / 0 | 1 | C | 36 |  |
| ShowPortraitOverVehicle | 0x2b4c1c | 2 | 2 / 0 / 0 / 0 | 1 | stand-in | 30 | call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| 1-8_runfromarrest | 0x27bb4c | 1 | 1 / 1 / 0 / 0 | 1 | stand-in | 35 | boss requested state 2 recorded (SCRIPT-S278, SCRIPT-339): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| AIRunAway | 0x17cc30 | 1 | 1 / 0 / 0 / 0 | 1 | stand-in | 34 | state 25 recorded; update 0x28b7a0 not ported (SCRIPT-104); returns 1 ([ai-states.md §2026-10-14T21:21](ai-states.md#ts-20261014-212158)): [ai-natives.md §2026-10-14T16:33](ai-natives.md#ts-20261014-163311) |
| AwardViewer | 0x1b23e4 | 0 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| CameraRotFixedSet | 0x27bb9c | 4 | 1 / 0 / 0 / 0 | 1 | B? | 28 | read, not handled: [dcam.md §2026-10-14T14:36](dcam.md#ts-20261014-143600) |
| CharacterShop | 0x1cd8e4 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| CharSetSkydiveMode | 0x1da6e4 | 2 | 1 / 0 / 0 / 0 | 1 | stand-in | 35 | state 129 / 1 and 0x376910 value recorded (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| DisableIcon | 0x1b2414 | 2 | 1 / 1 / 0 / 0 | 1 | stand-in | 31 | records its call (SCRIPT-S27): [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919) |
| EnableSuperBricks | 0x27bc08 | 1 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| FireBeamWeapon | 0x1d9cb4 | 4 | 1 / 0 / 0 / 0 | 1 | stand-in | 35 | beam arguments recorded (SCRIPT-S277, SCRIPT-338): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| GetCharacterUnlocked | 0x29e2c0 | 2 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled, exact (batch 27 entry) |
| getlanguage | 0x1b241c | 1 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S22) |
| GetOutMech | 0x1a2f04 | 1 | 1 / 0 / 0 / 0 | 1 | stand-in | 35 | pilot cleared (SCRIPT-S279, SCRIPT-340): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| GetPlayerNumber | 0x263618 | 1 | 1 / 1 / 0 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S22) |
| HACK_PlayDowntownIntro | 0x2ad87c | 1 | 1 / 1 / 1 / 0 | 1 | done | 35 | level 8 unlock, StoryProgress and the session byte (SCRIPT-S280): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| HubCustomisation | 0x26fc88 | 0 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| HubShop | 0x1b2450 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| IsRedbrickBought | 0x26fd30 | 2 | 1 / 0 / 0 / 0 | 1 | A | 27 | handled, exact (batch 27 entry) |
| PlayScriptOnStartup | 0x2915d4 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| PlaySoundAtObject | 0x27bd48 | 2 | 1 / 0 / 0 / 0 | 1 | stand-in | 33 | stand-in (AUDIO-S30) [§2026-10-15T17:27](script-natives-audio.md#ts-20261015-172705) |
| PositionBound | 0x268390 | 2 | 1 / 0 / 0 / 0 | 1 | A | 26 | open: needs the object world position, only in lvl_test_playground_mike, SCRIPT-59 |
| RedBrickShop | 0x1bfafc | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| Rex_AxeSmash_Done | 0x27bdb8 | 1 | 1 / 0 / 0 / 0 | 1 | stand-in | 35 | boss requested state 6 recorded (SCRIPT-S278, SCRIPT-339): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| Rex_GrapplePull_Done | 0x29ea28 | 1 | 1 / 0 / 0 / 0 | 1 | stand-in | 35 | boss requested state 8 recorded (SCRIPT-S278, SCRIPT-339): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| SetPropRocking | 0x1da6a8 | 2 | 1 / 1 / 0 / 0 | 1 | C | 38 |  |
| showinfopointer | 0x263848 | 2 | 1 / 0 / 0 / 0 | 1 | stand-in | 30 | call recorded only: [script-text.md §2026-10-14T15:25](script-text.md#ts-20261014-152500) |
| StudMagnetScene | 0x2638e8 | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| UnlockScanner | 0x1cdeb0 | 0 | 1 / 1 / 1 / 0 | 1 | A | 27 | handled as stand-in (SCRIPT-S20, batch 27 entry) |
| UsePet | 0x172184 | 2 | 1 / 0 / 0 / 0 | 1 | done | 35 | empty handler, returns 1: [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| VehicleSetThrowObjects | 0x2ad940 | 2 | 1 / 0 / 0 / 0 | 1 | C | 36 |  |
| VehicleShop | 0x1b278c | 1 | 1 / 0 / 0 / 0 | 1 | C | 39 |  |
| walnuts_startphase2 | 0x291730 | 1 | 1 / 0 / 0 / 0 | 1 | stand-in | 35 | boss requested state 2 recorded (SCRIPT-S278, SCRIPT-339): [script-natives-party.md §2026-10-19T15:53:02](script-natives-party.md#ts-20261019-155302) |
| ChangeCharTokenTexture | 0x2ad7c0 | 1 | 0 / 0 / 0 / 7 | 0 | C | 38 |  |
| EffectAttachParticlesOffset | 0x2ca970 | 5 | 0 / 0 / 0 / 13 | 0 | C | 38 |  |
| EffectDisableSpecialObject | 0x2c8f6c | 0 | 0 / 0 / 0 / 7 | 0 | C | 38 |  |
| EffectEnd | 0x1b438c | 1 | 0 / 0 / 0 / 6 | 0 | C | 38 |  |
| EffectStart | 0x1d0cd4 | 0 | 0 / 0 / 0 / 7 | 0 | C | 38 |  |
| StartFireTruckHose | 0x286214 | 1 | 0 / 0 / 0 / 2 | 0 | C | 38 |  |

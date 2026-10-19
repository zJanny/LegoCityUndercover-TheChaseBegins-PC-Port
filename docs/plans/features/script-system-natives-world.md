# Script system: mission 1-2 world natives

## 1. Scope

The six natives that mission 1-2 called without a handler: DestroyProp and KillObject (SCRIPT-58, same handler), EffectFadeObject, AddToSemiTransNearCameraList and the rest of batch 13 (SCRIPT-38), EnableDistanceFade (SCRIPT-57), SetPartyCharacter (SCRIPT-35) and StreamRoomIn (SCRIPT-38). The census and the other batches are in [script-system-natives.md](script-system-natives.md), room streaming in [world-streaming.md](world-streaming.md), the draw side in [rendering.md](rendering.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T11:03:52+02:00. The six natives have handlers read from the original and the port run of mission 1-2 no longer reports them: destroy and the node tween act on the objects, StreamRoomIn requests the room from the streamer and waits for it, the distance fade flag, the semi-transparent list and the party request are recorded. The original draws a distance fade on every mesh node by default (found while reading EnableDistanceFade), which the port does not (RENDER-66).

## 3. Verified behaviour

- `1-2_MissionStart` runs `DestroyProp(1-4_Police_Car)`, `EnableObject`, `EffectFadeObject(1-4_Police_Car, 0, 1, 1.2, 1)`: in the port the car is disabled at that point (startup state; not checked in the original), so the destroy is a no-op, and the fade raises it from hidden to opaque in 1.2 s. Pinned by `natives-world-real-data`. evidence: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- `1-2_Cage_Key_Target_Script` waits at `StreamRoomIn("Room40")` until the room is loaded; in `chase-game` the cutscene `1_2_c` starts after the load (9 updates, WSTREAM-S30) and the 1.5 s wait. Pinned by `natives-world-real-data` and `game-stream-room-in`.
- A scripted room request is a wanted room with priority 0 that unloads when the request stops. Pinned by `room-streaming-real-data`.
- The startup script's `SetPartyCharacter(Player1, 9, 1)` calls change nothing (character 9 is the player). Pinned by `natives-world-real-data`.

## 4. Stand-ins

- SCRIPT-S231 DestroyProp and KillObject destroy every enabled entity (hidden, no collision, `PlacedObjectState::destroyed`, EnableObject does not bring it back) except vehicles, whose class takes message 1 as its break; the class table of message 1 for the other kinds and EnableObject on a destroyed entity are not read. needs: SCRIPT-280. evidence: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- SCRIPT-S232 EffectFadeObject drives `Scene::setInstanceFade` and the instance visibility of placed objects (and their wheels) with the tween value; flag bit 2, particle nodes and the linked object are not ported, level characters are not faded, and the fade is a multiplier of the finished fragment (HUD-S70). needs: SCRIPT-281, HUD-70. evidence: same entry.
- SCRIPT-S233 EnableDistanceFade and AddToSemiTransNearCameraList only record their arguments; every mesh stays opaque at every distance. needs: SCRIPT-282, RENDER-66, RENDER-67. evidence: same entry.
- SCRIPT-S234 SetPartyCharacter with a character other than the player's logs "swap not ported" and changes nothing. needs: SCRIPT-279. evidence: same entry.
- SCRIPT-S235 The room request of StreamRoomIn lives for the update it is made in (the original clears the list at a time not read), the room is found by its number in the streamer's rooms, and loading takes the fixed 9 updates. needs: WSTREAM-33, WSTREAM-30. evidence: same entry.
- SCRIPT-S239 ToggleUseable and EnableSwitch write the use flag only for instances whose kind registers a use zone (`objectKindRegistersUseZone`); the conditions inside the registering functions are ignored (kinds 72 and 146 need a node, 225 the parameter `Carryable`, the post-link of 21 and 98 a clear halfword bit 7), an instance of an unknown kind records the flag, and the initial flag (`UseStartActive`) is not applied. needs: SCRIPT-296. uses: ToggleUseable, EnableSwitch. evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
- SCRIPT-S240 EnableShadow acts on kinds 27, 149, 121, 159 and 242: the shadow kind list is taken as filled for every placed instance at level start, and the callers of the character setup 0x24166c that could set bit 14 for other kinds are not traced. needs: SCRIPT-297. uses: EnableShadow. evidence: same entry.
- SCRIPT-S241 enableobjectfade does nothing for instances of kinds 27 and 102 (their handlers answer message 24, not read) and ends like EnableObject for every other kind; that is read for the 13 kinds of the main-level calls and rests on a scan for a compare with 24 for the rest. needs: SCRIPT-295. uses: enableobjectfade. evidence: same entry.

## 5. Open items

- [ ] SCRIPT-279 — The party swap 0x245258: the party members `Player<1|2>.Player%02d`, the table at 0x51b24c in the mission states, the character that id 16 stands for, the carry hand-over and the member's `startup` run. blocks: SCRIPT-S234, SCRIPT-35. evidence: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- [ ] SCRIPT-280 — Which classes handle message 1 of the destroy 0x23cd80 (table 0x245004 + kind x 40, +0x14), what the broadcasts 0x23cee8 over 0x3775c0 / 0x3775c4 do, and EnableObject (0x1ddd00) on a destroyed entity. blocks: SCRIPT-S231. evidence: same entry.
- [ ] SCRIPT-281 — EffectFadeObject: the third node property (0x23f540, flag bit 2), the particle node case, the clock of 0x23f74c (real time or update time) and fading level characters and other non-placed nodes. needs: HUD-70. blocks: SCRIPT-S232. evidence: same entry.
- [ ] SCRIPT-282 — EnableDistanceFade: the node kind gate (global 0x37674c) and the child recursion 0x2b65dc per placed class, DistanceFadeMid, and the AI target scorers 0x2df0dc / 0x2ee214 that scale their ranges with the tier of 0x28781c. needs: RENDER-66. blocks: SCRIPT-S233, SCRIPT-57. evidence: same entry.
- [ ] SCRIPT-283 — AddToSemiTransList 0x2a059c and AddToSemiTransListBound 0x2c08e8 (13 calls, the other callers of the list entry creator 0x1f40d8) are not read. needs: RENDER-67. blocks: SCRIPT-38. evidence: same entry.
- [ ] SCRIPT-294 — The hint registry 0x2219bc searches (records at `*(0x58e9a4 + 0x24)`, singleton "LEGOHintBounds"): who adds records and what the byte +0xa gates; the port's `usable` is the use-zone bit only. blocks: SCRIPT-S239. uses: ToggleUseable. evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
- [ ] SCRIPT-295 — Message 24 in the handlers of kinds 27 (0x18e4c8) and 102 (0x18c100), and a table-driven dispatch in kinds outside the 13 read by hand. blocks: SCRIPT-S241. uses: enableobjectfade. evidence: same entry.
- [ ] SCRIPT-296 — The registration conditions of the use zone: what sets halfword `[entity + 0x10]` bit 7 (post-link of kinds 21 and 98), the node pointer test of kinds 72 and 146, `Carryable` of kind 225, and `UseStartActive` as the initial flag. blocks: SCRIPT-S239. uses: ToggleUseable. evidence: same entry.
- [ ] SCRIPT-297 — The classes that reach the character setup 0x24166c through 0x22b79c (callers 0x175ac4, 0x224f54, 0x29ee04) and so may set bit 14 of `+0xc`; nothing reads the shadow flag yet. blocks: SCRIPT-S240. uses: EnableShadow. evidence: same entry.

## 6. Findings summary

- Handlers, the distance fade of mesh nodes with measured thresholds, the tween rules and the request list: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- The node tween registry first read for the waypoint column: [objective-hud.md §2026-10-15T19:58](../../re/objective-hud.md#ts-20261015-195841).
- Census rows: [script-natives-census.md](../../re/script-natives-census.md).

## 7. Code and tests

- `src/runtime/natives_effects.*` (`EffectNativeState`), `src/runtime/natives_world.*` (`PartyRequest`, `partyOutcome`), `ObjectNativeState::destroy` in `src/runtime/natives_objects.*`, `PlacedObjectState::destroyed` (`src/assets/object_collision.*`), `ScriptRunner::destroyNative` / `effectNative` / `worldNative` (`src/runtime/script_runner_natives.cpp`), `RoomStreamer::update(..., scripted)`, glue `src/game_level_natives.cpp` (`applyScriptNatives`, `applyObjectFades`, `installScriptHooks`).
- CTest: `natives-world`, `natives-world-real-data`, `natives-objects`, `room-streaming-real-data`, `game-stream-room-in`.

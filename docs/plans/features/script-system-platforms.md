# Script system: platforms and attached nodes

## 1. Scope

The platform class (ObjectType 64, `props/prop_platform`, `leprops/leprop_platform`): its record, the trigger messages, the travel and the node move, and the node hierarchy of AttachObject that carries attached objects with a moving node. The natives MovePlatform, SetPlatform* and Rideable* are batch SCRIPT-37 in [script-system-natives.md](script-system-natives.md); WaitForPlatform and the generator scene of mission 1-3 are in [script-system-missions-1-3.md](script-system-missions-1-3.md).

## 2. Status

IN_PROGRESS, updated 2026-10-26T22:58:23+01:00. The create, the post-link (`ObjectTo`), the messages 0xff, 0xfe and 43, the request 0x2150bc and the update 0x2807c8 are read and ported for platforms without a path, `orient` or type 7: the node moves on the lerp from its placement to `ObjectTo` with the shaped value, ends by type (types 0 to 6) and carries the nodes attached below it (translation). The crane platform of mission 1-3 goes down 10.63 in 102 node moves with the girder; the director camera, PlayParticlesGO and the AI read the carried positions. Nothing rides on a moving platform yet (SCRIPT-378).

## 3. Verified behaviour

- The crane platform's travel against the oracle (RPC 20261025-garage-C-1): trigger at u90, node moves u91..u192 (0.105 per update, y 16.6251 to 5.9968), state 5 at u192, the wait ends at u193; the girder goes down with it (y 27.481 to 16.852); CTests `platform-node-move`, `platform-node-move-real-data`, `prop-rocking-real-data`, `game-generator-1-3`. evidence: [props-rocking-platforms.md §2026-10-26T22:54:42](../../re/props-rocking-platforms.md#ts-20261026-225442).

## 4. Stand-ins

- SCRIPT-S301 Parts of the platform class are not ported: a platform with a `Platform_Path` steps its value but its node does not move, `orient` and type 7 do not turn the node, the shape curves 1..5 and the named curves return the value unchanged, the lift timers of types 5 and 6, state 1, the speed counters, the sounds and the messages 41 to controlled platforms are missing. needs: SCRIPT-377. evidence: [props-rocking-platforms.md §2026-10-26T22:54:42](../../re/props-rocking-platforms.md#ts-20261026-225442).
- SCRIPT-S302 Carrying: attached nodes take only the translation of a moving node (no ported platform turns), the platforms update in the order of the level's objects (the entity order of the original is not read), and carried props keep the rotation the rocking or their placement gives them. needs: SCRIPT-377. evidence: [props-rocking-platforms.md §2026-10-26T22:54:42](../../re/props-rocking-platforms.md#ts-20261026-225442).

## 5. Open items

- [ ] SCRIPT-377 — The rest of the platform class: path platforms (the spline of `Platform_Path`, the lawnmowers of mission 1-3, which the user saw move wrong in the port), `orient` and type 7 rotations, the shape curves, the lift timers and state 1, the speed counters, sounds and messages 41. blocks: SCRIPT-S301, SCRIPT-S302. uses: props/prop_platform, leprops/leprop_platform. evidence: [props-rocking-platforms.md §2026-10-26T22:54:42](../../re/props-rocking-platforms.md#ts-20261026-225442).
- [ ] SCRIPT-378 — Nothing rides on a moving platform: the entity move's sweep (push, squash, HardTarget) and the character's ground entity are not read, so the player stays behind when `1-3_Ledge_Platform` rises and a platform's collision moves through characters. blocks: SCRIPT-372. evidence: [props-rocking-platforms.md §2026-10-26T22:54:42](../../re/props-rocking-platforms.md#ts-20261026-225442).

## 6. Findings summary

- The platform class (create, post-link, messages, request, update, node move) and AttachObject's node hierarchy: [props-rocking-platforms.md §2026-10-26T22:54:42](../../re/props-rocking-platforms.md#ts-20261026-225442).
- The oracle trace of the crane platform: [props-rocking.md §2026-10-25T16:46:40](../../re/props-rocking.md#ts-20261025-164640).

## 7. Code and tests

- `src/runtime/platform_node_move.*` (record, messages, update, `NodeCarry`), `src/runtime/platform_wait.*` (`PlatformSet`: `carriedOffset`, `attachedTo`, `nodeParent`, arrivals), ScriptRunner glue `src/runtime/game_platform_glue.cpp`, scene glue `src/game_platform_nodes.cpp` (log line `platform NAME: travel ended in state S at X Y Z after N node moves at update U`).
- CTests `platform-node-move`, `platform-node-move-real-data`, `game-generator-1-3` (slow).
